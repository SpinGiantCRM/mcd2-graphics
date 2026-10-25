'use strict';
const fs=require('node:fs/promises');const path=require('node:path');const {readContract,mapPath,planDeployment,safeRelative}=require('./contract');const io=require('./filesystem');const {MARKER}=require('./configuration');const {guardHardlink}=require('./guarded-hardlink');const {checkOwnership}=require('./ownership');
const GAME='minecraftdungeons2',TYPE='mcd2-graphics-root',HARDLINK='hardlink_activator';
function cleanFiles(files){const result=files.filter(f=>!/[\\/]$/.test(f)).map(f=>f.replaceAll('\\','/'));const seen=new Set();for(const f of result){safeRelative(f);if(seen.has(f.toLowerCase()))throw Error('Duplicate archive member');seen.add(f.toLowerCase());}return result;}
function savedContracts(state){return Object.values(state.persistent?.mods?.[GAME]??{}).filter(m=>m.attributes?.mcd2InstallContract).map(m=>readContract(JSON.stringify(m.attributes.mcd2InstallContract)));}
async function editionFor(root,c){const found=[];for(const [name,p] of Object.entries(c.editionProfiles)){const file=await io.target(root,p.shipping);if(await io.exists(file))found.push(name);}if(found.length!==1)throw Error('Choose an installation containing one supported Steam or Game Pass executable layout');return found[0];}
function init(context,runtime=null){const api=context.api;const {util,selectors,actions}=runtime??require('vortex-api');let ready=false;
 const root=()=>api.getState().settings.gameMode.discovered[GAME]?.path;
 const ensureHardlink=()=>{if(!ready)throw Error('The Vortex hardlink safety adapter has not loaded. Restart Vortex before installing this package.');const state=api.getState(),current=util.getCurrentActivator(state,GAME,true);if(current?.id!==HARDLINK)throw Error('Select Hardlink Deployment in Vortex Settings → Mods, then retry. MCD2 Graphics does not support symlink or copy deployment.');if(state.settings.mods.activator[GAME]===undefined)api.store.dispatch(actions.setActivator(GAME,HARDLINK));};
 async function contextForPath(dataPath,removing=false,purgeStaging=null){const state=api.getState(),gameRoot=root();if(!gameRoot)return null;if(purgeStaging&&path.resolve(purgeStaging)!==path.resolve(selectors.installPathForGame(state,GAME)))return null;if(dataPath!==undefined&&(process.platform==='win32'?path.resolve(dataPath).toLowerCase()!==path.resolve(gameRoot).toLowerCase():path.resolve(dataPath)!==path.resolve(gameRoot)))return null;const profile=selectors.activeProfile(state);if(profile?.gameId!==GAME)return null;
  const mods=state.persistent.mods[GAME]??{};const selected=Object.values(mods).filter(m=>profile.modState[m.id]?.enabled&&m.attributes?.mcd2PackageRole==='payload');
  if(selected.length>1)throw Error('Enable only one MCD2 Graphics package per profile');
  if(removing||selected.length===0){
   // Purge/profile change must use the recorded old profile, not a new active one.
   for(const c of savedContracts(state))for(const edition of Object.keys(c.editionProfiles)){const b=await io.read(gameRoot,mapPath(c,edition,MARKER));if(!b)continue;const r=JSON.parse(b.toString());if(r.owner!=='vortex')continue;return {root:gameRoot,contract:readContract(JSON.stringify(r.contract)),edition:r.edition,profileId:r.profileId,remove:true};}
   return null;
  }
  const c=readContract(JSON.stringify(selected[0].attributes.mcd2InstallContract));const edition=selected[0].attributes.mcd2Edition;if(await editionFor(gameRoot,c)!==edition)throw Error('Installation edition changed; reinstall the Vortex package');
  const stagedFiles=new Map(),expectedFiles=new Map(),staging=selectors.installPathForGame(state,GAME);
  for(const mod of Object.values(mods).filter(m=>profile.modState[m.id]?.enabled&&m.type===TYPE)){
   const mc=readContract(JSON.stringify(mod.attributes.mcd2InstallContract));if(mc.packageVersion!==c.packageVersion||mod.attributes.mcd2Edition!==edition)throw Error('Mixed MCD2 package versions/editions; reinstall matching requirements');
   const files=mod.attributes.mcd2Files;const folder=await io.target(staging,mod.installationPath);
   for(const [relative,h] of Object.entries(files??{})){const destination=mapPath(c,edition,relative);if(stagedFiles.has(destination))throw Error('Two enabled packages own the same destination');stagedFiles.set(destination,await io.target(folder,destination));expectedFiles.set(destination,h);}
  }
  return {root:gameRoot,contract:c,edition,profileId:profile.id,stagedFiles,expectedFiles};
 }
 context.once(()=>{const stock=util.getActivator(HARDLINK);if(!stock)throw Error('Stock Vortex hardlink deployment is unavailable');guardHardlink(stock,contextForPath);ready=true;});
 context.registerModType(TYPE,20,id=>id===GAME,()=>{const state=api.getState();if(savedContracts(state).length&&state.settings.mods.activator[GAME]!==HARDLINK)throw Error('Use Hardlink Deployment for MCD2 Graphics; purge/remove it before changing deployment methods');return root();},instructions=>Promise.resolve(instructions.some(i=>i.type==='attribute'&&i.key==='mcd2PackageRole')),{name:'MCD2 Graphics (game root)'});
 context.registerInstaller('mcd2-graphics-contract',21,async(files,gameId)=>{
  if(gameId!==GAME)return {supported:false,requiredFiles:[]};
  const candidates=files.map(f=>f.replaceAll('\\','/'));
  const contractFile=candidates.find(f=>f.split('/').at(-1)==='mcd2-install.json');
  const depNames=savedContracts(api.getState()).flatMap(c=>c.dependencies.flatMap(d=>Object.keys(d.files).map(p=>path.posix.basename(p).toLowerCase())));
  return {supported:!!contractFile||candidates.some(f=>depNames.includes(path.posix.basename(f).toLowerCase())),requiredFiles:contractFile?[contractFile]:[]};
 },async(files,destinationPath,gameId)=>{
  if(gameId!==GAME)throw Error('Wrong game');ensureHardlink();const list=cleanFiles(files),manifest=list.filter(f=>path.posix.basename(f)==='mcd2-install.json');let c,edition,owned,instructions,role;
  if(manifest.length===1){const bytes=await io.read(destinationPath,manifest[0]);if(!bytes)throw Error('Missing package contract');c=readContract(bytes.toString());edition=await editionFor(root(),c);const prefix=manifest[0].slice(0,-'mcd2-install.json'.length);owned=c.payload;instructions=planDeployment(c,edition);for(const i of instructions){i.source=prefix+i.source;if(!list.includes(i.source)||await io.digest(await io.target(destinationPath,i.source))!==owned[i.source.slice(prefix.length+'payload/'.length)])throw Error('Changed or missing payload member');}role='payload';
  }else if(manifest.length>1)throw Error('Duplicate package contract');
  else{
   const contracts=savedContracts(api.getState());if(contracts.length===0)throw Error('Install the MCD2 Graphics package into Vortex first, then import its requirement downloads');
   const candidates=new Map();for(const x of contracts)candidates.set(require('./contract').identity(x),x);
   if(candidates.size!==1)throw Error('Remove outdated MCD2 candidate packages from Vortex before importing requirements');c=[...candidates.values()][0];edition=await editionFor(root(),c);const matches=[];
   for(const dep of c.dependencies.filter(d=>d.editions.includes(edition)))for(const release of dep.releases){const copies=[];let valid=true;for(const [relative,h] of Object.entries(release.files)){const names=list.filter(f=>dep.archiveMembers?.[relative]?f===dep.archiveMembers[relative]:path.posix.basename(f).toLowerCase()===path.posix.basename(relative).toLowerCase());const verified=[];for(const source of names)if(await io.digest(await io.target(destinationPath,source))===h)verified.push(source);if(verified.length!==1){valid=false;break;}copies.push({type:'copy',source:verified[0],destination:mapPath(c,edition,relative)});}if(valid)matches.push({dep,release,copies});}
   const unique=new Map(matches.map(x=>[JSON.stringify(x.release.files),x]));if(unique.size!==1)throw Error('This download is not one complete supported requirement release');const match=[...unique.values()][0];owned=match.release.files;instructions=match.copies;role=match.dep.id;
  }
  const ownership=await checkOwnership({root:root(),contract:c,edition});if(ownership.owner==='standalone'||ownership.owner==='conflict')throw Error(ownership.next);
  return {instructions:[{type:'setmodtype',value:TYPE},{type:'attribute',key:'mcd2PackageRole',value:role},{type:'attribute',key:'mcd2InstallContract',value:c},{type:'attribute',key:'mcd2Edition',value:edition},{type:'attribute',key:'mcd2Files',value:owned},...instructions]};
 });
 return true;
}
module.exports={default:init,init,checkOwnership,cleanFiles,editionFor};
