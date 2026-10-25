'use strict';
const {test}=require('node:test');const assert=require('node:assert/strict');const fs=require('node:fs/promises');const os=require('node:os');const path=require('node:path');const crypto=require('node:crypto');
const {readContract,planDeployment}=require('../src/contract');
const {applyOwnedConfiguration,restoreOwnedConfiguration}=require('../src/configuration');
const {preflightDeployment}=require('../src/deployment');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function completeStock(stock){stock.externalChanges??=async()=>[];stock.prePurge??=async()=>{};return stock;}
function contract(){return {schemaVersion:1,kind:'mcd2-graphics',packageVersion:'candidate.test',editionProfiles:{Steam:{binaryDirectory:'Dungeons/Binaries/Win64',shipping:'Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe',exeSHA256:hash('game')}},payload:{'Dungeons/Binaries/Win64/own.addon64':hash('payload')},dependencies:[],configuration:{startup:{path:'Dungeons/Config/UserEngine.ini',section:'SystemSettings',keys:{'r.AllowHDR':'1','r.AntiAliasingMethod':'2'}},hdrActivation:{path:'Dungeons/Binaries/Win64/ReShade.ini',lease:'Dungeons/Binaries/Win64/MCD2Graphics/HdrAddonActivation.bin'},frameworkPreviousSHA256:null},migration:{previousInstalls:{}}};}
async function fixture(t){const root=await fs.mkdtemp(path.join(os.tmpdir(),'mcd2-vortex-'));t.after(()=>fs.rm(root,{recursive:true,force:true}));return root;}
async function put(root,relative,data){const p=path.join(root,relative);await fs.mkdir(path.dirname(p),{recursive:true});await fs.writeFile(p,data);}

test('package mapping preserves real root paths and maps WinGDK once',()=>{
 const c=contract();c.editionProfiles.WinGDK={binaryDirectory:'Dungeons/Binaries/WinGDK',shipping:'Dungeons/Binaries/WinGDK/Dungeons-WinGDK-Shipping.exe',exeSHA256:hash('game')};
 const instructions=planDeployment(readContract(JSON.stringify(c)),'WinGDK');
 assert.deepEqual(instructions,[{type:'copy',source:'payload/Dungeons/Binaries/Win64/own.addon64',destination:'Dungeons/Binaries/WinGDK/own.addon64'}]);
 for(const relative of ['../bad','Dungeons/Binaries/Win64/sl.reflex.dll','a\\b']){const bad=contract();bad.payload={[relative]:hash('x')};assert.throws(()=>readContract(JSON.stringify(bad)));}
 const bad=contract();bad.schemaVersion=2;assert.throws(()=>readContract(JSON.stringify(bad)));
});
test('configuration restores original keys and preserves later player edits',async t=>{
 const root=await fixture(t);const c=contract();await put(root,c.configuration.startup.path,'[SystemSettings]\nr.AllowHDR=0\nother=42\n');
 const r=await applyOwnedConfiguration({root,contract:c,edition:'Steam',profileId:'private-profile'});
 assert.match(await fs.readFile(path.join(root,c.configuration.startup.path),'utf8'),/r.AntiAliasingMethod=2/);
 await fs.appendFile(path.join(root,c.configuration.startup.path),'player=1\n');
 await restoreOwnedConfiguration({root,contract:c,edition:'Steam',profileId:'private-profile'},r);
 assert.equal(await fs.readFile(path.join(root,c.configuration.startup.path),'utf8'),'[SystemSettings]\nr.AllowHDR=0\nother=42\nplayer=1\n');
});
test('configuration rejects duplicate keys, linked paths and wrong profile',async t=>{
 const root=await fixture(t);const c=contract();await put(root,c.configuration.startup.path,'[SystemSettings]\nr.AllowHDR=0\nr.AllowHDR=1\n');
 await assert.rejects(applyOwnedConfiguration({root,contract:c,edition:'Steam',profileId:'a'}));
 await put(root,c.configuration.startup.path,'');const r=await applyOwnedConfiguration({root,contract:c,edition:'Steam',profileId:'a'});
 await assert.rejects(restoreOwnedConfiguration({root,contract:c,edition:'Steam',profileId:'b'},r));
});
test('preflight catches changed game, competing manager and staged hash mismatch without writes',async t=>{
 const root=await fixture(t);const stage=await fixture(t);const c=contract();await put(root,c.editionProfiles.Steam.shipping,'game');await put(stage,'Dungeons/Binaries/Win64/own.addon64','payload');
 const ctx={root,contract:c,edition:'Steam',profileId:'a',stagedFiles:new Map([['Dungeons/Binaries/Win64/own.addon64',path.join(stage,'Dungeons/Binaries/Win64/own.addon64')]]),requireClosed:async()=>{}};
 await preflightDeployment(ctx);await put(root,c.editionProfiles.Steam.shipping,'changed');await assert.rejects(preflightDeployment(ctx));
 await put(root,c.editionProfiles.Steam.shipping,'game');await put(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json',JSON.stringify({version:'test',owner:'standalone',schemaVersion:1,files:c.payload}));await assert.rejects(preflightDeployment(ctx));
 await fs.rm(path.join(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json'));await put(stage,'Dungeons/Binaries/Win64/own.addon64','wrong');await assert.rejects(preflightDeployment(ctx));
 assert.equal(await fs.readFile(path.join(stage,'Dungeons/Binaries/Win64/own.addon64'),'utf8'),'wrong');
});

test('both manager transitions give removal steps and reject unknown receipts',async t=>{
 const {checkOwnership}=require('../src/ownership');const root=await fixture(t),c=contract();const ctx={root,contract:c,edition:'Steam'};
 await put(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json',JSON.stringify({version:c.packageVersion,files:c.payload}));
 let result=await checkOwnership(ctx);assert.equal(result.owner,'standalone');assert.match(result.next,/Uninstall.*keeps shared requirements/);
 await put(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json',JSON.stringify({schemaVersion:1,owner:'vortex'}));
 result=await checkOwnership(ctx);assert.equal(result.owner,'vortex');assert.match(result.next,/Purge.*preferences/);
 await put(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json','{}');assert.equal((await checkOwnership(ctx)).owner,'conflict');
});

test('guarded stock hardlink calls refuse writes when preflight fails and preserve stock identity',async t=>{
 const root=await fixture(t),stage=await fixture(t),c=contract();await put(root,c.editionProfiles.Steam.shipping,'wrong game');await put(stage,'own','payload');
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>{await fs.link(path.join(stage,'own'),path.join(root,'deployed'));return [];},prePurge:async()=>{},purge:async()=>{await fs.rm(path.join(root,'keep'));},postPurge:async()=>{}};
 const ctx={root,contract:c,edition:'Steam',profileId:'a',stagedFiles:new Map([['Dungeons/Binaries/Win64/own.addon64',path.join(stage,'own')]]),requireClosed:async()=>{}};
 require('../src/guarded-hardlink').guardHardlink(completeStock(stock),async data=>data===root?ctx:null);
 assert.equal(stock.id,'hardlink_activator');await assert.rejects(stock.prepare(root,true,[],x=>x));
 assert.equal(await fs.stat(path.join(root,'deployed')).then(()=>true,()=>false),false);
 await put(root,c.editionProfiles.Steam.shipping,'game');await put(root,'keep','keep');
 await put(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json','{"schemaVersion":1,"owner":"standalone"}');
 await assert.rejects(stock.purge(stage,root,'minecraftdungeons2'));
 assert.equal(await fs.readFile(path.join(root,'keep'),'utf8'),'keep');
});

test('real hardlink lifecycle deploys, disables, re-enables and releases ownership for standalone',async t=>{
 const root=await fixture(t),stage=await fixture(t),c=contract();await put(root,c.editionProfiles.Steam.shipping,'game');await put(stage,'Dungeons/Binaries/Win64/own.addon64','payload');
 const destination='Dungeons/Binaries/Win64/own.addon64';let enabled=true;
 const ctx={root,contract:c,edition:'Steam',profileId:'a',expectedFiles:new Map([[destination,hash('payload')]]),stagedFiles:new Map([[destination,path.join(stage,destination)]]),requireClosed:async()=>{}};
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>{const p=path.join(root,destination);if(enabled){await fs.rm(p,{force:true});await fs.link(path.join(stage,destination),p);}else await fs.rm(p,{force:true});return [];},prePurge:async()=>{},purge:async()=>{await fs.rm(path.join(root,destination),{force:true});},postPurge:async()=>{}};
 require('../src/guarded-hardlink').guardHardlink(completeStock(stock),async data=>data===root?{...ctx,remove:!enabled}:null);
 await stock.prepare(root,true,[],x=>x);await stock.finalize('minecraftdungeons2',root,stage);
 assert.equal((await fs.stat(path.join(root,destination))).ino,(await fs.stat(path.join(stage,destination))).ino);
 let r=JSON.parse(await fs.readFile(path.join(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json')));assert.equal(r.phase,'active');
 await fs.appendFile(path.join(root,c.configuration.startup.path),'player=keep\n');enabled=false;
 await stock.prepare(root,true,[],x=>x);await stock.finalize('minecraftdungeons2',root,stage);
 assert.equal((await require('../src/ownership').checkOwnership(ctx)).owner,'unmanaged');assert.match(await fs.readFile(path.join(root,c.configuration.startup.path),'utf8'),/player=keep/);
 enabled=true;await stock.prepare(root,true,[],x=>x);await stock.finalize('minecraftdungeons2',root,stage);
 await stock.purge(stage,root,'minecraftdungeons2');await stock.postPurge();assert.equal((await require('../src/ownership').checkOwnership(ctx)).owner,'unmanaged');
 assert.equal(await fs.readFile(path.join(stage,destination),'utf8'),'payload');
});
test('real package installer recognises only its contract and verifies before returning managed instructions',async t=>{
 const root=await fixture(t),extracted=await fixture(t),c=contract();await put(root,c.editionProfiles.Steam.shipping,'game');await put(extracted,'mcd2-install.json',JSON.stringify(c));await put(extracted,'payload/Dungeons/Binaries/Win64/own.addon64','payload');
 let install,once,type;const state={settings:{gameMode:{discovered:{minecraftdungeons2:{path:root}}},mods:{activator:{minecraftdungeons2:'hardlink_activator'}}},persistent:{mods:{minecraftdungeons2:{}}}};
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>[],purge:async()=>{},postPurge:async()=>{}};
 require('../src/index').init({api:{getState:()=>state,store:{dispatch:()=>{}}},once:f=>{once=f;},registerInstaller:(...args)=>{install=args;},registerModType:(...args)=>{type=args;}},{util:{getActivator:()=>completeStock(stock),getCurrentActivator:()=>stock},selectors:{},actions:{}});once();
 assert.equal((await install[2](['mod.pak'],'minecraftdungeons2')).supported,false);assert.equal((await install[2](['mcd2-install.json'],'minecraftdungeons2')).supported,true);
 const result=await install[3](['mcd2-install.json','payload/Dungeons/Binaries/Win64/own.addon64'],extracted,'minecraftdungeons2');
 assert.equal(result.instructions.find(i=>i.type==='setmodtype').value,'mcd2-graphics-root');assert.equal(result.instructions.find(i=>i.type==='copy').destination,'Dungeons/Binaries/Win64/own.addon64');
 assert.equal(type[3](),root);state.settings.mods.activator.minecraftdungeons2='symlink_activator';state.persistent.mods.minecraftdungeons2.mod={attributes:{mcd2InstallContract:c}};assert.throws(type[3],/Hardlink/);
 state.settings.mods.activator.minecraftdungeons2='hardlink_activator';await put(extracted,'payload/Dungeons/Binaries/Win64/own.addon64','changed');await assert.rejects(install[3](['mcd2-install.json','payload/Dungeons/Binaries/Win64/own.addon64'],extracted,'minecraftdungeons2'));
});

test('interrupted candidate upgrade retries safely against journaled previous payload',async t=>{
 const root=await fixture(t),stage=await fixture(t),old=contract();const destination='Dungeons/Binaries/Win64/own.addon64';await put(root,old.editionProfiles.Steam.shipping,'game');await put(root,destination,'payload');
 const oldCtx={root,contract:old,edition:'Steam',profileId:'a',requireClosed:async()=>{}};await applyOwnedConfiguration(oldCtx);
 const next=contract();next.packageVersion='candidate.next';next.payload[destination]=hash('next payload');next.migration.previousInstalls={[old.packageVersion]:old.payload};await put(stage,destination,'next payload');
 const ctx={...oldCtx,contract:next,stagedFiles:new Map([[destination,path.join(stage,destination)]]),deployed:false};
 const {beforeDeploy}=require('../src/deployment');await beforeDeploy(ctx);await beforeDeploy(ctx);
 assert.equal(await fs.readFile(path.join(root,destination),'utf8'),'payload');
});

test('prePurge guard runs before stock preparation or external-change handling',async t=>{
 const root=await fixture(t),c=contract();await put(root,'keep','keep');await put(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json','{"schemaVersion":1,"owner":"standalone"}');
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>[],prePurge:async()=>{await fs.rm(path.join(root,'keep'));},purge:async()=>{},postPurge:async()=>{}};
 require('../src/guarded-hardlink').guardHardlink(completeStock(stock),async()=>({root,contract:c,edition:'Steam',profileId:'a',requireClosed:async()=>{}}));
 await assert.rejects(stock.prePurge('staging'));assert.equal(await fs.readFile(path.join(root,'keep'),'utf8'),'keep');
});

test('active configuration repair yields to external edits even if they equal the original value',async t=>{
 const root=await fixture(t),c=contract(),ctx={root,contract:c,edition:'Steam',profileId:'a'};
 await put(root,c.configuration.startup.path,'[SystemSettings]\nr.AllowHDR=0\n');await applyOwnedConfiguration(ctx);
 const p=path.join(root,c.configuration.startup.path);await fs.writeFile(p,(await fs.readFile(p,'utf8')).replace('r.AllowHDR=1','r.AllowHDR=0'));
 await assert.rejects(applyOwnedConfiguration(ctx));assert.match(await fs.readFile(p,'utf8'),/r.AllowHDR=0/);
});

test('purging MCD2 while another game is active still restores owned configuration',async t=>{
 const root=await fixture(t),stage=await fixture(t),c=contract(),destination='Dungeons/Binaries/Win64/own.addon64';await put(root,c.editionProfiles.Steam.shipping,'game');await put(root,destination,'payload');await put(root,c.configuration.startup.path,'[SystemSettings]\nr.AllowHDR=0\n');
 const ctx={root,contract:c,edition:'Steam',profileId:'mcd2-profile'};await applyOwnedConfiguration(ctx);
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>[],prePurge:async()=>{},purge:async()=>{await fs.rm(path.join(root,destination),{force:true});},postPurge:async()=>{}};
 const state={settings:{gameMode:{discovered:{minecraftdungeons2:{path:root}}},mods:{activator:{minecraftdungeons2:'hardlink_activator'}}},persistent:{mods:{minecraftdungeons2:{main:{attributes:{mcd2InstallContract:c,mcd2PackageRole:'payload'}}}}}};
 let once;require('../src/index').init({api:{getState:()=>state,store:{dispatch:()=>{}}},once:f=>{once=f;},registerInstaller:()=>{},registerModType:()=>{}},{util:{getActivator:()=>completeStock(stock)},selectors:{activeProfile:()=>({id:'other-profile',gameId:'other-game'}),installPathForGame:()=>stage},actions:{}});once();
 await stock.prePurge(stage);await stock.purge(stage,root,'minecraftdungeons2');await stock.postPurge();
 assert.match(await fs.readFile(path.join(root,c.configuration.startup.path),'utf8'),/r.AllowHDR=0/);
 assert.equal(await fs.stat(path.join(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json')).then(()=>true,()=>false),false);
});

test('early external-change boundary refuses unsafe deployment before reconciliation writes',async t=>{
 const root=await fixture(t),stage=await fixture(t),c=contract();await put(root,c.editionProfiles.Steam.shipping,'wrong game');await put(root,'keep','keep');
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>[],prePurge:async()=>{},purge:async()=>{},postPurge:async()=>{},externalChanges:async()=>{await fs.rm(path.join(root,'keep'));return [];}};
 require('../src/guarded-hardlink').guardHardlink(completeStock(stock),async()=>({root,contract:c,edition:'Steam',profileId:'a',stagedFiles:new Map(),expectedFiles:new Map(),requireClosed:async()=>{}}));
 await assert.rejects(stock.externalChanges('minecraftdungeons2',stage,root,[]));assert.equal(await fs.readFile(path.join(root,'keep'),'utf8'),'keep');
});
test('post-finalize configuration failure preserves activation list for Vortex manifest commit',async t=>{
 const root=await fixture(t),stage=await fixture(t),c=contract(),destination='Dungeons/Binaries/Win64/own.addon64';await put(root,c.editionProfiles.Steam.shipping,'game');await put(stage,destination,'payload');
 const activation=[{relPath:destination,source:'main',target:'',time:1}],errors=[];
 const stock={id:'hardlink_activator',prepare:async()=>{},finalize:async()=>{await fs.link(path.join(stage,destination),path.join(root,destination));await fs.writeFile(path.join(root,c.configuration.startup.path),'[SystemSettings]\nr.AllowHDR=9\n');return activation;},prePurge:async()=>{},purge:async()=>{},postPurge:async()=>{}};
 const ctx={root,contract:c,edition:'Steam',profileId:'a',stagedFiles:new Map([[destination,path.join(stage,destination)]]),expectedFiles:new Map([[destination,hash('payload')]]),requireClosed:async()=>{}};
 require('../src/guarded-hardlink').guardHardlink(completeStock(stock),async()=>ctx,e=>errors.push(e));await stock.prepare(root,true,[],x=>x);
 assert.deepEqual(await stock.finalize('minecraftdungeons2',root,stage),activation);assert.equal(errors.length,1);
 const r=JSON.parse(await fs.readFile(path.join(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json')));assert.equal(r.phase,'prepared');
});

test('partial purge retaining only a fresh proxy cannot release ownership',async t=>{
 const root=await fixture(t),c=contract();c.payload={'Dungeons/Binaries/Win64/dxgi.dll':hash('proxy')};await put(root,'Dungeons/Binaries/Win64/dxgi.dll','proxy');
 const ctx={root,contract:c,edition:'Steam',profileId:'a',framework:{hadFile:false,previousHash:null},requireClosed:async()=>{}};
 await applyOwnedConfiguration(ctx);const {beforeRemove,afterRemove}=require('../src/deployment');await beforeRemove(ctx);
 await assert.rejects(afterRemove(ctx));assert.equal(await fs.stat(path.join(root,'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json')).then(()=>true,()=>false),true);
});

test('standalone handoff waits until Vortex releases its private DLSS requirement',async t=>{
 const root=await fixture(t),c=contract(),runtime='Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll';
 c.dependencies=[{id:'DLSSRuntime',role:'common',editions:['Steam'],files:{[runtime]:hash('runtime')},releases:[{version:'1',files:{[runtime]:hash('runtime')}}],acquisition:{method:'browser',url:'https://example.com/dlss'}}];
 await put(root,'Dungeons/Binaries/Win64/own.addon64','payload');await put(root,runtime,'runtime');const ctx={root,contract:c,edition:'Steam',profileId:'a',privateRuntimeManaged:true,requireClosed:async()=>{}};
 await applyOwnedConfiguration(ctx);const {beforeRemove,afterRemove}=require('../src/deployment');await beforeRemove(ctx);await fs.rm(path.join(root,'Dungeons/Binaries/Win64/own.addon64'));await afterRemove(ctx);
 assert.equal((await require('../src/ownership').checkOwnership(ctx)).owner,'vortex');assert.equal(await fs.readFile(path.join(root,runtime),'utf8'),'runtime');
 await fs.rm(path.join(root,runtime));await afterRemove(ctx);assert.equal((await require('../src/ownership').checkOwnership(ctx)).owner,'unmanaged');
});
