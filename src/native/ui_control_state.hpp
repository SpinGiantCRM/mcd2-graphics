#pragma once
#include "settings_bridge.hpp"
struct UiControlState {
 mcd2ui::Mailbox queue; mcd2ui::Settings desired;
 bool pending=false, observing=false, fallback=false; unsigned fallback_error=1, phase=0, applied_revision=0, invalid_reads=0;
 unsigned width=0,height=0,outwidth=0,outheight=0; uint64_t dimension_frame=0;
 uint32_t session=0; ULONGLONG wrong_ratio_since=0, queued_at=0; fs::path saves; std::ofstream receipt;
} ui_control;
static void ui_observe_source(a::command_list *cmd){
 if(!ui_control.observing)return;
 for(const auto &slot:resolve(cmd,true))if(slot.space==0 && slot.reg==0 && slot.binding.type==a::descriptor_type::constant_buffer){
  std::array<float,84> pc{};if(!read_upload(slot,sizeof(pc),pc.data()))return;
  for(unsigned i:{36u,37u,44u,45u})if(!std::isfinite(pc[i]) || pc[i]<1 || pc[i]>8192 || pc[i]!=std::floor(pc[i]))return;
  ui_control.width=unsigned(pc[36]);ui_control.height=unsigned(pc[37]);ui_control.outwidth=unsigned(pc[44]);ui_control.outheight=unsigned(pc[45]);ui_control.dimension_frame=frame;return;
 }
}
