'use strict';
const {mapPath}=require('./contract');const io=require('./filesystem');const {MARKER}=require('./configuration');
async function checkOwnership(ctx){const b=await io.read(ctx.root,mapPath(ctx.contract,ctx.edition,MARKER));if(!b)return {owner:'unmanaged',next:'Ready for this manager after preflight.'};let r;try{r=JSON.parse(b.toString());}catch{return {owner:'conflict',next:'Keep these files and use the original installer or restore its receipt.'};}
 if(r.owner==='vortex'&&r.schemaVersion===1)return {owner:'vortex',next:'Disable MCD2 Graphics and its private DLSSRuntime (nvngx_dlss.dll) requirement in Vortex, then Deploy, or Purge. Other shared requirements and saved preferences can stay. Then open standalone and Check again.'};
 const known=r.version===ctx.contract.packageVersion?ctx.contract.payload:ctx.contract.migration?.previousInstalls?.[r.version];
 if((r.schemaVersion===undefined||r.schemaVersion===0||r.schemaVersion===1&&r.owner==='standalone')&&known&&r.files&&Object.entries(known).every(([p,h])=>r.files[p]===h))return {owner:'standalone',next:'Open your existing standalone installer and choose Uninstall. It keeps shared requirements, preferences and saves. Then return to Vortex and Deploy.'};
 return {owner:'conflict',next:'The receipt is not a recognised standalone or Vortex install. Keep the files and use its original installer; do not delete DLLs manually.'};}
module.exports={checkOwnership};
