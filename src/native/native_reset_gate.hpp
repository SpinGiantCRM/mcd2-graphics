// One-shot independence proof; completely absent unless explicitly requested.
// Vary the finite history texture while all current inputs and reset constants
// stay fixed. Restore the original history bytes before leaving the AA boundary.
static bool native_reset_pair(a::command_list *cmd,const Cmd &saved,uint32_t x,uint32_t y,uint32_t z){
 if(!lean.reset_pair_requested)return false;
 lean.reset_pair_requested=false;auto &c=*live_fixture;auto slots=resolve(cmd,true);
 const Slot *history=nullptr,*out=nullptr;
 for(const auto &s:slots)if(s.space==0){if(s.reg==5 && s.binding.type==a::descriptor_type::shader_resource_view)history=&s;if(s.reg==0 && s.binding.type==a::descriptor_type::unordered_access_view)out=&s;}
 if(!history || !out)return false;
 auto *h=reinterpret_cast<ID3D12Resource*>(from_view(cmd->get_device(),history->binding.view).handle),*o=reinterpret_cast<ID3D12Resource*>(from_view(cmd->get_device(),out->binding.view).handle);
 auto hd=h->GetDesc();if(h==o || hd.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || hd.Width!=3840 || hd.Height!=2160 || hd.MipLevels!=1 || hd.DepthOrArraySize!=1)return false;
 const auto k=reinterpret_cast<uint64_t>(h);auto it=saved.states.find(k);D3D12_RESOURCE_STATES state;
 if(it!=saved.states.end())state=native_state(it->second);else{auto prior=submitted_states.find(k);if(prior==submitted_states.end())return false;state=native_state(prior->second);}
 if(!(state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))return false;
 auto *native=reinterpret_cast<ID3D12GraphicsCommandList*>(cmd->get_native());
 auto *backup=eval_texture(c.resource_device,c.owned,3840,2160,hd.Format,false);
 auto *zero=eval_texture(c.resource_device,c.owned,3840,2160,hd.Format,false);
 auto *bright=eval_texture(c.resource_device,c.owned,3840,2160,hd.Format,false);
 if(!backup || !zero || !bright)return false;
 auto seed=[&](ID3D12Resource *r,const std::array<uint16_t,4> &value){
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT rows=0;UINT64 rowbytes=0,total=0;c.resource_device->GetCopyableFootprints(&hd,0,1,0,&fp,&rows,&rowbytes,&total);
  auto *upload=eval_buffer(c.resource_device,c.owned,total,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);if(!upload)return false;
  void *p=nullptr;D3D12_RANGE empty{0,0};if(FAILED(upload->Map(0,&empty,&p)))return false;
  for(unsigned row=0;row<rows;row++)for(unsigned col=0;col<3840;col++)memcpy(static_cast<char*>(p)+fp.Offset+row*fp.Footprint.RowPitch+col*8,value.data(),8);
  upload->Unmap(0,nullptr);D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload;src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=fp;dst.pResource=r;dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;native->CopyTextureRegion(&dst,0,0,0,&src,nullptr);eval_transition(native,r,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);return true;
 };
 if(!seed(zero,{0,0,0,0}) || !seed(bright,{0x5640,0x5240,0x4900,0x3c00}))return false;
 auto capture=[&](ID3D12Resource *r,const char *suffix,D3D12_RESOURCE_STATES prior){
  eval_transition(native,r,prior,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  auto path=root/label/(std::to_string(frame)+"-"+suffix+".bin");auto read=dense_readback(c.resource_device,c.owned,native,r,path);
  if(read.buffer){read.buffer->AddRef();copies.push_back({read.buffer,cmd,nullptr,read.total,read.rows,read.fp.Footprint.RowPitch,static_cast<uint32_t>(read.rowbytes),path});}
  eval_transition(native,r,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,prior);
 };
 eval_transition(native,h,state,D3D12_RESOURCE_STATE_COPY_SOURCE);native->CopyResource(backup,h);eval_transition(native,backup,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);eval_transition(native,h,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
 capture(backup,"history-before",D3D12_RESOURCE_STATE_COPY_SOURCE);
 native->CopyResource(h,zero);eval_transition(native,h,D3D12_RESOURCE_STATE_COPY_DEST,state);native->Dispatch(x,y,z);D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=o;native->ResourceBarrier(1,&order);capture(o,"native-reset-zero",D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 eval_transition(native,h,state,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(h,bright);eval_transition(native,h,D3D12_RESOURCE_STATE_COPY_DEST,state);native->Dispatch(x,y,z);native->ResourceBarrier(1,&order);capture(o,"native-reset-bright",D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 eval_transition(native,h,state,D3D12_RESOURCE_STATE_COPY_DEST);native->CopyResource(h,backup);eval_transition(native,h,D3D12_RESOURCE_STATE_COPY_DEST,state);capture(h,"history-after",state);
 lean.log<<"{\"kind\":\"native_reset_pair_recorded\",\"frame\":"<<frame<<",\"finiteHistoryVariants\":2,\"resetPassByteOffset\":48,\"originalHistoryRestorationRecorded\":true}\n";
 return true;
}
