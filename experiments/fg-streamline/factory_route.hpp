// Synthetic, single-factory route only. No executable code or vendor DLL patch.
// Route the native factory's HWND creation through SL before ReShade wraps
// the returned swapchain. The object's standard COM vtable is shadowed and
// restored before releasing it. This is not yet a game integration.
#include <array>
#include <cstring>
struct FactoryRoute {
 using Create=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,HWND,const DXGI_SWAP_CHAIN_DESC1*,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,IDXGISwapChain1**);
 inline static FactoryRoute* active=nullptr;
 inline static thread_local bool entering=false;
 IDXGIFactory7* native=nullptr;IDXGIFactory2* proxy=nullptr;
 void** original=nullptr;std::array<void*,32> table{};unsigned routed=0;
 static HRESULT STDMETHODCALLTYPE create(IDXGIFactory2* factory,IUnknown* queue,HWND window,const DXGI_SWAP_CHAIN_DESC1* desc,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen,IDXGIOutput* output,IDXGISwapChain1** swap){
  auto self=active;if(!self||factory!=self->native)return E_UNEXPECTED;
  auto originalCreate=reinterpret_cast<Create>(self->original[15]);
  if(entering)return originalCreate(factory,queue,window,desc,fullscreen,output,swap);
  entering=true;++self->routed;
  auto result=self->proxy->CreateSwapChainForHwnd(queue,window,desc,fullscreen,output,swap);
  entering=false;return result;
 }
 bool attach(IDXGIFactory4* outer){
  if(active||native)return false;
  constexpr GUID unwrap={0x7f2c9a11,0x3b4e,0x4d6a,{0x81,0x2f,0x5e,0x9c,0xd3,0x7a,0x1b,0x42}};
  IDXGIFactory* raw=nullptr;
  if(FAILED(outer->QueryInterface(unwrap,reinterpret_cast<void**>(&raw)))||!raw)return false;
  auto hr=raw->QueryInterface(IID_PPV_ARGS(&native));raw->Release();if(FAILED(hr)||!native)return false;
  if(FAILED(native->QueryInterface(IID_PPV_ARGS(&proxy)))){detach();return false;}
  if(mcd2_fg_upgrade(reinterpret_cast<void**>(&proxy))){detach();return false;}
  original=*reinterpret_cast<void***>(native);
  std::memcpy(table.data(),original,sizeof(table));table[15]=reinterpret_cast<void*>(create);active=this;
  InterlockedExchangePointer(reinterpret_cast<void* volatile*>(native),table.data());return true;
 }
 void detach(){
  if(original&&native){InterlockedExchangePointer(reinterpret_cast<void* volatile*>(native),original);original=nullptr;}
  if(active==this)active=nullptr;
  if(proxy){proxy->Release();proxy=nullptr;}if(native){native->Release();native=nullptr;}
 }
 ~FactoryRoute(){detach();}
};
