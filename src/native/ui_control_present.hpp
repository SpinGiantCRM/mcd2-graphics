#pragma once
#include "upscaler_support.hpp"
static bool ui_dlss_adapter_candidate(a::command_queue *queue,unsigned &vendor){
 auto *device=reinterpret_cast<ID3D12Device*>(queue->get_device()->get_native());
 IDXGIFactory4 *factory=nullptr;IDXGIAdapter1 *adapter=nullptr;DXGI_ADAPTER_DESC1 desc{};
 if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))return false;
 auto result=factory->EnumAdapterByLuid(device->GetAdapterLuid(),IID_PPV_ARGS(&adapter));
 factory->Release();
 if(FAILED(result))return false;
 result=adapter->GetDesc1(&desc);adapter->Release();
 if(FAILED(result))return false;
 vendor=desc.VendorId;return mcd2ui::dlss_adapter_candidate(vendor);
}
static std::vector<uint8_t> ui_read_file(const fs::path &p){
 std::ifstream in(p,std::ios::binary|std::ios::ate);if(!in)return {};auto n=in.tellg();if(n<64||n>8192)return {};
 std::vector<uint8_t> b(size_t(n),0);in.seekg(0);in.read(reinterpret_cast<char*>(b.data()),n);if(!in)return {};return b;
}
static bool ui_runtime_ack(unsigned phase,unsigned error=0){
 auto bytes=ui_read_file(ui_control.saves/L"MCD2GraphicsRuntime.sav");if(bytes.empty())return false;
 mcd2ui::RuntimeState current;if(!mcd2ui::decode_runtime(bytes,current))return false;
 std::vector<uint8_t> out(bytes.begin(),bytes.begin()+current.headerBytes);
 auto u32=[&](uint32_t v){for(unsigned i=0;i<4;i++)out.push_back(uint8_t(v>>(i*8)));};
 auto str=[&](const std::string &s){u32(uint32_t(s.size()+1));out.insert(out.end(),s.begin(),s.end());out.push_back(0);};
 auto value=[&](const char *name,uint32_t v){if(!v)return;str(name);str("IntProperty");u32(0);u32(4);out.push_back(0);u32(v);};
 value("SchemaVersion",1);value("RequestedRevision",ui_control.desired.revision);value("SessionId",ui_control.session);value("Phase",phase);value("ErrorCode",error);str("None");u32(0);
 auto target=ui_control.saves/L"MCD2GraphicsRuntime.sav",temporary=ui_control.saves/L"MCD2GraphicsRuntime.pending";
 {std::ofstream f(temporary,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(out.data()),out.size());f.flush();if(!f)return false;}
 if(!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;
 ui_control.receipt<<"{\"event\":\"runtime_ack\",\"revision\":"<<ui_control.desired.revision<<",\"phase\":"<<phase<<",\"session\":"<<ui_control.session<<",\"error\":"<<error<<",\"frame\":"<<frame<<"}\n";ui_control.receipt.flush();return true;
}
static bool ui_poll_intent(){
 if(ui_control.saves.empty()){
  // UE render threads can have a small Windows stack. Keep the 64 KiB path
  // buffer on the heap; even an initialization-only local reserves its frame
  // on every call to this present callback.
  std::vector<wchar_t> local(32768);auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.data(),DWORD(local.size()));if(!n||n>=local.size())return false;
  ui_control.session=uint32_t((GetTickCount64() ^ (uint64_t(GetCurrentProcessId())<<12)) & 0x7fffffff);if(!ui_control.session)ui_control.session=1;
  ui_control.saves=fs::path(local.data())/L"Dungeons2"/L"Saved"/L"SaveGames";
  root=fs::path(local.data())/L"Dungeons2"/L"Saved"/L"MCD2Graphics";
  std::error_code directory_error;fs::create_directories(root,directory_error);if(directory_error){ui_control.saves.clear();return false;}
  ui_control.receipt.open(root/L"MCD2GraphicsNativeReceipt.jsonl",std::ios::app);
 }
 mcd2ui::Settings incoming;auto bytes=ui_read_file(ui_control.saves/L"MCD2GraphicsSettings.sav");
 const bool legacyDecoded=mcd2ui::decode(bytes,incoming);
 const int shared=provider_menu_sr_intent(incoming);
 if(shared<0 || (!shared&&!legacyDecoded)){
  // Invalid persisted intent may never activate an NGX feature.
  if(++ui_control.invalid_reads<3)return false;
  if(live_fixture && lean.wanted){lean.wanted=false;ui_control.fallback=true;ui_control.pending=true;ui_control.phase=0;}
  return ui_control.fallback;
 }
 ui_control.invalid_reads=0;
 // Only the current game process can authorize SR in a gameplay world.
 // Old saves and the rendered frontend remain Native while preserving UI intent.
 if(incoming.contextReady!=1 || incoming.contextSession!=ui_control.session){incoming.mode=0;incoming.scale=10000;}
 if(ui_control.queue.offer(incoming)){
  ui_control.queue.take(ui_control.desired);ui_control.pending=true;ui_control.phase=0;ui_control.observing=false;ui_control.fallback=false;ui_control.fallback_error=1;ui_control.queued_at=GetTickCount64();ui_control.width=ui_control.height=ui_control.outwidth=ui_control.outheight=0;ui_control.dimension_frame=0;ui_control.wrong_ratio_since=0;
  ui_control.receipt<<"{\"event\":\"intent_queued\",\"revision\":"<<incoming.revision<<",\"mode\":"<<incoming.mode<<",\"frame\":"<<frame<<"}\n";ui_control.receipt.flush();
 }else if(incoming.revision==ui_control.desired.revision && incoming.mode==ui_control.desired.mode && incoming.scale==ui_control.desired.scale && incoming.preset==ui_control.desired.preset && incoming.custom==ui_control.desired.custom && incoming.schema==ui_control.desired.schema){
  ui_control.desired.sourceRevision=incoming.sourceRevision;ui_control.desired.sourceSession=incoming.sourceSession;
 }
 return true;
}
static void ui_apply_queued(a::command_queue *q,std::unique_lock<std::recursive_mutex> &guard){
 if(ui_control.phase==2 && live_fixture && lean.rebuild && !mcd2ui::source_ratio(ui_control.desired,lean.next_width,lean.next_height,lean.next_outwidth,lean.next_outheight)){
  // A native settings reapply is different from an output-resolution change.
  // Retire/reset first, then ask the game thread to restore persisted source scale.
  ui_control.pending=true;ui_control.phase=0;ui_control.fallback=false;ui_control.wrong_ratio_since=0;ui_control.queued_at=GetTickCount64();
  ui_control.receipt<<"{\"event\":\"source_scale_reapply\",\"revision\":"<<ui_control.desired.revision<<",\"frame\":"<<frame<<"}\n";ui_control.receipt.flush();
 }
 if(ui_control.phase==2 && live_fixture && (lean.failed || (!lean.wanted && !lean.rebuild))){
  ui_control.fallback=true;ui_control.pending=true;ui_control.phase=0;
 }
 if(!ui_control.pending)return;
 if(ui_control.phase==0){
  lean.wanted=false;lean.rebuild=false;
  // Existing gate restores native history; cleanup checks recordings and a fresh GPU fence.
  if(live_fixture){if(lean.pending || lean.history_dirty || !lean.borrow.recordings.empty() || lean.borrow.blocked)return;if(!lean_cleanup(q,guard))return;lean_write_status();}
  // Decline unsupported rendering adapters before asking the game to reduce
  // source resolution. Shader observations may never arrive on these systems.
  unsigned vendor=0;
  if(!ui_control.fallback && ui_control.desired.mode!=0 && !ui_dlss_adapter_candidate(q,vendor)){
   ui_control.fallback=true;ui_control.fallback_error=1;
   ui_control.receipt<<"{\"event\":\"adapter_declined\",\"revision\":"<<ui_control.desired.revision<<",\"vendor\":"<<vendor<<"}\n";ui_control.receipt.flush();
  }
  if(ui_control.fallback){if(!ui_runtime_ack(3,ui_control.fallback_error))return;ui_control.pending=false;ui_control.observing=false;return;}
  if(!ui_runtime_ack(1))return;ui_control.phase=1;ui_control.observing=true;ui_control.queued_at=GetTickCount64();return;
 }
 if(ui_control.phase==1){
  if(ui_control.desired.mode!=0 && mcd2ui::source_wait_expired(GetTickCount64()-ui_control.queued_at)){
   ui_control.fallback=true;ui_control.fallback_error=1;ui_control.phase=0;
   ui_control.receipt<<"{\"event\":\"source_wait_timeout\",\"revision\":"<<ui_control.desired.revision<<"}\n";ui_control.receipt.flush();return;
  }
  if(ui_control.desired.sourceRevision!=ui_control.desired.revision || ui_control.desired.sourceSession!=ui_control.session)return;
  if(ui_control.desired.mode==0){if(!ui_runtime_ack(2))return;ui_control.applied_revision=ui_control.desired.revision;ui_control.pending=false;ui_control.observing=false;ui_control.phase=2;return;}
  auto &c=ui_control;
  const bool fresh=c.dimension_frame && c.dimension_frame+3>=frame;
  // UI bounds normally prevent these; reject malformed or engine-clamped sources as well.
  if(fresh && (c.width<640 || c.height<360)){c.fallback=true;c.fallback_error=2;c.phase=0;return;}
  const bool dimensions=c.dimension_frame+3>=frame && c.width>=640 && c.height>=360 && c.outwidth>=c.width && c.outheight>=c.height && c.outwidth<=3840 && c.outheight<=2160;
  bool ratio=dimensions && mcd2ui::source_ratio(c.desired,c.width,c.height,c.outwidth,c.outheight);
  if(!dimensions || !ratio){if(dimensions){if(!c.wrong_ratio_since)c.wrong_ratio_since=GetTickCount64();if(GetTickCount64()-c.wrong_ratio_since>15000){c.fallback=true;c.fallback_error=2;c.phase=0;}}else c.wrong_ratio_since=0;return;}
  c.wrong_ratio_since=0;
  // A destroyed queue or exhausted retained reset ledger requires Native/restart.
  if(native_reset_borrows.blocked || native_reset_borrows.entries.size()>=256){c.fallback=true;c.phase=0;return;}
  lean_width=c.width;lean_height=c.height;lean_outwidth=c.outwidth;lean_outheight=c.outheight;
  label="ui02-live-"+std::to_string(ui_control.session)+"-r"+std::to_string(c.desired.revision);fs::create_directories(root/label);lean=LeanState{};lean.log.open(root/label/"lean-events.jsonl");
  if(!init_live_saved(q,&guard)){c.fallback=true;c.phase=0;lean.failed=true;return;}
  lean.wanted=true;lean.last_wanted=true;c.observing=false;c.phase=4;c.queued_at=GetTickCount64();
  c.receipt<<"{\"event\":\"feature_created\",\"revision\":"<<c.desired.revision<<",\"input\":["<<c.width<<","<<c.height<<"],\"output\":["<<c.outwidth<<","<<c.outheight<<"]}\n";c.receipt.flush();ui_runtime_ack(4);return;
 }
 if(ui_control.phase==4){
  if(lean.failed || !lean.wanted){ui_control.fallback=true;ui_control.phase=0;return;}
  if(lean.evaluations>0){if(!ui_runtime_ack(2))return;ui_control.applied_revision=ui_control.desired.revision;ui_control.pending=false;ui_control.phase=2;return;}
  if(GetTickCount64()-ui_control.queued_at>15000){lean.wanted=false;ui_control.fallback=true;ui_control.phase=0;}
 }
}
