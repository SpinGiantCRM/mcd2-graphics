'use strict';
const {beforeDeploy,verifyDeployed,beforeRemove,afterRemove,preflightDeployment,preflightRemoval,requireClosed}=require('./deployment');
const guarded=new WeakSet();

// Vortex 2.9.0's emitAndAwait swallows event errors. The public stock activator
// methods, in contrast, are awaited directly by deploy/purge. Decorate those
// transaction boundaries, without replacing hardlinks or their manifests.
function guardHardlink(stock,contextForPath,report=()=>{}){
 if(stock.id!=='hardlink_activator')throw Error('MCD2 Graphics requires Vortex hardlink deployment');
 if(guarded.has(stock))return;guarded.add(stock);
 if(typeof stock.externalChanges!=='function')throw Error('Unsupported Vortex hardlink transaction API');
 const pending=new Map(),purged=new Map();
 const externalChanges=stock.externalChanges.bind(stock);
 stock.externalChanges=async(gameId,installationPath,...args)=>{
  if(gameId==='minecraftdungeons2'){
   const ctx=await contextForPath(undefined,false,installationPath);
   if(ctx){if(ctx.remove)await preflightRemoval(ctx);else await preflightDeployment(ctx);}
  }
  return externalChanges(gameId,installationPath,...args);
 };
 const prepare=stock.prepare.bind(stock),finalize=stock.finalize.bind(stock),purge=stock.purge.bind(stock),prePurge=stock.prePurge?.bind(stock),postPurge=stock.postPurge.bind(stock);
 stock.prepare=async(dataPath,...args)=>{
  const ctx=await contextForPath(dataPath);if(ctx){await (ctx.requireClosed??requireClosed)();if(ctx.remove)await beforeRemove(ctx);else await beforeDeploy({...ctx,deployed:false});pending.set(dataPath,ctx);}
  return prepare(dataPath,...args);
 };
 stock.finalize=async(gameId,dataPath,...args)=>{
  const ctx=pending.get(dataPath)??await contextForPath(dataPath);
  if(ctx){await (ctx.requireClosed??requireClosed)();if(!ctx.remove){for(const [destination,source] of ctx.stagedFiles){const expected=ctx.expectedFiles.get(destination);if(expected&&await require('./filesystem').digest(source)!==expected)throw Error('Staging changed during deployment');}}}
  const result=await finalize(gameId,dataPath,...args);
  if(ctx){try{if(ctx.remove)await afterRemove(ctx);else await verifyDeployed(ctx);}catch(error){
   // Links have changed. Always return activation results so Vortex can commit
   // its manifest; the private journal stays prepared/removing for recovery.
   try{const notification=report(error,ctx);if(notification?.catch)notification.catch(()=>{});}catch{}
  }}pending.delete(dataPath);return result;
 };
 stock.prePurge=async(installationPath,...args)=>{const ctx=await contextForPath(undefined,true,installationPath);if(ctx)await beforeRemove(ctx);return prePurge?.(installationPath,...args);};
 stock.purge=async(installationPath,dataPath,...args)=>{
  const ctx=await contextForPath(dataPath,true);if(ctx)await beforeRemove(ctx);
  const result=await purge(installationPath,dataPath,...args);if(ctx)purged.set(dataPath,ctx);return result;
 };
 stock.postPurge=async(...args)=>{const result=await postPurge(...args);for(const [p,ctx] of purged){await afterRemove(ctx);purged.delete(p);}return result;};
}
module.exports={guardHardlink};
