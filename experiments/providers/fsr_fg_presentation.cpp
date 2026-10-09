// Standalone SDK experiment. Owns every resource; never opens a game or save.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include "framegeneration/include/dx12/ffx_api_framegeneration_dx12.h"
#include "api/include/dx12/ffx_api_dx12.h"
#include "framegeneration/include/ffx_framegeneration.h"

constexpr unsigned Width = 1280, Height = 720;
constexpr auto Read = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
template<class T> void release(T*& p) { if (p) { p->Release(); p = nullptr; } }
struct Log {
    FILE* file = nullptr;
    ~Log() { if (file) fclose(file); }
    void result(const char* stage, long code) {
        fprintf(file, "{\"stage\":\"%s\",\"result\":%ld}\n", stage, code); fflush(file);
    }
};
static Log* outputLog;
static std::atomic<unsigned> errors{0}, warnings{0};
void message(uint32_t type, const wchar_t*) {
    // Keep SDK text private; the evidence contains only severity counts.
    if (type == FFX_API_MESSAGE_TYPE_ERROR) ++errors; else ++warnings;
}
struct Session {
    HMODULE sdk = nullptr;
    PfnFfxCreateContext create = nullptr;
    PfnFfxDestroyContext destroy = nullptr;
    PfnFfxQuery query = nullptr;
    PfnFfxDispatch dispatch = nullptr;
    PfnFfxConfigure configure = nullptr;
    ffxContext context = nullptr;
    ffxContext swapContext = nullptr;IDXGISwapChain4* swap=nullptr;HWND window=nullptr;HANDLE waitable=nullptr;
    std::atomic<unsigned> realCallbacks{0},generatedCallbacks{0},callbackErrors{0};
    IDXGIFactory4* factory = nullptr;
    ID3D12Device* device = nullptr;
    ID3D12CommandQueue* queue = nullptr;
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList* cmd = nullptr;
    ID3D12Fence* fence = nullptr;
    HANDLE event = nullptr;
    uint64_t value = 0;
    bool submitted = false, drained = true, retirementFailed = false;
    ID3D12DescriptorHeap* rtv = nullptr;
    ID3D12Resource* images[7]{};
    ID3D12Resource* readback = nullptr;
    bool wait() {
        auto next = ++value;
        drained = SUCCEEDED(queue->Signal(fence, next)) &&
            SUCCEEDED(fence->SetEventOnCompletion(next, event)) && WaitForSingleObject(event, 30000) == WAIT_OBJECT_0;
        return drained;
    }
    bool retire() {
        if (retirementFailed) return false;
        if (submitted && !wait()) { retirementFailed = true; return false; }
        if(swapContext){ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12 wait{};wait.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12;
            auto result=dispatch(&swapContext,&wait.header);outputLog->result("wait-for-presents",result);if(result){retirementFailed=true;return false;}}
        if (cmd) {
            release(cmd);
            outputLog->result("invalidate-owned-recording", 0);
            if (!wait()) { retirementFailed = true; return false; }
            outputLog->result("post-invalidation-fence", 0);
        }
        if (context) {
            // Stop generation on the one owned presenter before destroying contexts.
            ffxConfigureDescFrameGeneration off{};off.header.type=FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
            off.swapChain=swap;
            auto disabled=configure(&context,&off.header);outputLog->result("disable-before-destroy",disabled);if(disabled){retirementFailed=true;return false;}
            auto code = destroy(&context, nullptr);
            outputLog->result("destroy-context-after-fence", code);
            if (code) { retirementFailed = true; return false; }
            outputLog->result("destroy-returned-context-nonnull", context ? 1 : 0);
            // Successful destruction consumes the handle. Some runtime versions
            // leave the caller's pointer unchanged; never destroy it a second time.
            context = nullptr;
        }
        if(swapContext){auto result=destroy(&swapContext,nullptr);outputLog->result("destroy-swapchain-after-presents",result);if(result){retirementFailed=true;return false;}swapContext=nullptr;
            release(swap);if(waitable){CloseHandle(waitable);waitable=nullptr;}}
        for (unsigned i=0; i<7; ++i) {
            if(images[i]) outputLog->result("release-owned-image", i);
            release(images[i]);
        }
        if(readback) outputLog->result("release-readback", 0);
        release(readback);
        if(rtv) outputLog->result("release-RTV-heap", 0);
        release(rtv);
        outputLog->result("retired-owned-resources", 0);
        return true;
    }
    ~Session() {
        // Never release resources/context/module while GPU work may reference them.
        // On fence failure the process exit is the containment boundary.
        if (!retire()) return;
        release(cmd); release(allocator); release(fence); release(queue); release(device); release(factory);
        if (event) CloseHandle(event);
        if (window) DestroyWindow(window);
        if (sdk) FreeLibrary(sdk);
    }
};
void transition(ID3D12GraphicsCommandList* cmd, ID3D12Resource* image,
                D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    D3D12_RESOURCE_BARRIER b{}; b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition = {image, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, from, to}; cmd->ResourceBarrier(1, &b);
}
float half(uint16_t value) {
    const unsigned e = (value >> 10) & 31, m = value & 1023;
    float result = e == 0 ? std::ldexp(float(m), -24) : e == 31 ? INFINITY : std::ldexp(1.f + float(m)/1024, int(e)-15);
    return value & 32768 ? -result : result;
}
bool resources(Session& s, unsigned w, unsigned h) {
    D3D12_DESCRIPTOR_HEAP_DESC views{}; views.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; views.NumDescriptors = 7;
    if (FAILED(s.device->CreateDescriptorHeap(&views, IID_PPV_ARGS(&s.rtv)))) return false;
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    const DXGI_FORMAT formats[] = {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R16G16B16A16_FLOAT};
    auto handle = s.rtv->GetCPUDescriptorHandleForHeapStart();
    auto stride = s.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (unsigned i=0; i<7; ++i) {
        D3D12_RESOURCE_DESC d{}; d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width = i == 3 ? 1 : i == 6 ? Width : w; d.Height = i == 3 ? 1 : i == 6 ? Height : h;
        d.DepthOrArraySize = 1; d.MipLevels = 1; d.SampleDesc.Count = 1; d.Format = formats[i];
        d.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (i == 6) d.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (FAILED(s.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &d,
            D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&s.images[i])))) return false;
        s.device->CreateRenderTargetView(s.images[i], nullptr, handle); handle.ptr += stride;
    }
    D3D12_RESOURCE_DESC buffer{}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = uint64_t(Width) * Height * 8; buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    return SUCCEEDED(s.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&s.readback)));
}
bool checkOutput(Session& s, bool hdr, unsigned frame) {
    D3D12_RANGE range{0, size_t(Width)*Height*8}; void* data = nullptr;
    if (FAILED(s.readback->Map(0, &range, &data))) return false;
    const auto values = static_cast<uint16_t*>(data);
    bool valid = true; float minimum = INFINITY, maximum = -INFINITY;
    for (size_t i=0; i<size_t(Width)*Height; ++i) for (unsigned c=0; c<3; ++c) {
        float v = half(values[i*4+c]); valid &= std::isfinite(v) && v >= -.05f && v < 10;
        minimum = (std::min)(minimum, v); maximum = (std::max)(maximum, v);
    }
    // Two flat regions far from their edge establish actual, correctly scaled output,
    // rather than accepting an untouched sentinel or a black dispatch.
    for (unsigned side=0; side<2; ++side) for (unsigned c=0; c<3; ++c) {
        auto i = size_t(Height/2)*Width + (side ? Width*3/4 : Width/4);
        float expected = side ? (hdr ? float(2u << c) : float(1u << c)/8) : float(1u << c)/4;
        valid &= std::abs(half(values[i*4+c])-expected) < (std::max)(.025f, expected*.05f);
    }
    D3D12_RANGE written{0,0}; s.readback->Unmap(0, &written);
    fprintf(outputLog->file, "{\"stage\":\"readback\",\"frame\":%u,\"HDR\":%s,\"valid\":%s,\"minimum\":%.6f,\"maximum\":%.6f}\n",
        frame, hdr?"true":"false", valid?"true":"false", minimum, maximum); fflush(outputLog->file);
    return valid;
}

static D3D12_RESOURCE_STATES nativeState(uint32_t state){
    D3D12_RESOURCE_STATES result=D3D12_RESOURCE_STATE_COMMON;
    if(state&FFX_API_RESOURCE_STATE_UNORDERED_ACCESS)result|=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if(state&FFX_API_RESOURCE_STATE_COMPUTE_READ)result|=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if(state&FFX_API_RESOURCE_STATE_PIXEL_READ)result|=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if(state&FFX_API_RESOURCE_STATE_COPY_SRC)result|=D3D12_RESOURCE_STATE_COPY_SOURCE;
    if(state&FFX_API_RESOURCE_STATE_COPY_DEST)result|=D3D12_RESOURCE_STATE_COPY_DEST;
    if(state&FFX_API_RESOURCE_STATE_RENDER_TARGET)result|=D3D12_RESOURCE_STATE_RENDER_TARGET;
    return result;
}
static ffxReturnCode_t compose(ffxCallbackDescFrameGenerationPresent* p,void* user){
    auto& s=*static_cast<Session*>(user);
    if(!p||!p->commandList||!p->currentBackBuffer.resource||!p->outputSwapChainBuffer.resource){++s.callbackErrors;return FFX_API_RETURN_ERROR_PARAMETER;}
    auto* cmd=static_cast<ID3D12GraphicsCommandList*>(p->commandList);auto* src=static_cast<ID3D12Resource*>(p->currentBackBuffer.resource);auto* dst=static_cast<ID3D12Resource*>(p->outputSwapChainBuffer.resource);
    const auto beforeSrc=nativeState(p->currentBackBuffer.state),beforeDst=nativeState(p->outputSwapChainBuffer.state);
    if(src!=dst){transition(cmd,src,beforeSrc,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(cmd,dst,beforeDst,D3D12_RESOURCE_STATE_COPY_DEST);cmd->CopyResource(dst,src);transition(cmd,dst,D3D12_RESOURCE_STATE_COPY_DEST,beforeDst);transition(cmd,src,D3D12_RESOURCE_STATE_COPY_SOURCE,beforeSrc);}
    if(p->isGeneratedFrame)++s.generatedCallbacks;else ++s.realCallbacks;
    return FFX_API_RETURN_OK;
}
static ffxReturnCode_t generate(ffxDispatchDescFrameGeneration* p,void* user){auto& s=*static_cast<Session*>(user);return s.dispatch(&s.context,&p->header);}
int main() {
    Log log; fopen_s(&log.file, "fsr-fg-presentation.jsonl", "w"); if (!log.file) return 1; outputLog=&log;
    Session s;
    auto path=std::make_unique<wchar_t[]>(32768);
    auto size=GetCurrentDirectoryW(32768,path.get());if(!size||size>32660)return 2;
    wcscat_s(path.get(),32768,L"\\amd_fidelityfx_framegeneration_dx12.dll");
    s.sdk=LoadLibraryExW(path.get(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    log.result("load-pinned-framegeneration",s.sdk?0:GetLastError());if(!s.sdk)return 3;
    s.create=reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(s.sdk,"ffxCreateContext"));
    s.destroy=reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(s.sdk,"ffxDestroyContext"));
    s.query=reinterpret_cast<PfnFfxQuery>(GetProcAddress(s.sdk,"ffxQuery"));
    s.dispatch=reinterpret_cast<PfnFfxDispatch>(GetProcAddress(s.sdk,"ffxDispatch"));
    s.configure=reinterpret_cast<PfnFfxConfigure>(GetProcAddress(s.sdk,"ffxConfigure"));
    if(!s.create||!s.destroy||!s.query||!s.dispatch||!s.configure)return 4;
    const bool expectFramework=GetEnvironmentVariableW(L"MCD2_FSR_EXPECT_RESHADE",nullptr,0)!=0;
    const auto dxgi=GetModuleHandleW(L"dxgi.dll");const bool frameworkExports=dxgi&&GetProcAddress(dxgi,"ReShadeRegisterAddon");
    auto hr=CreateDXGIFactory1(IID_PPV_ARGS(&s.factory));log.result("factory",hr);if(FAILED(hr))return 5;
    hr=D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&s.device));log.result("device",hr);if(FAILED(hr))return 6;
    HMODULE deviceOwner=nullptr;auto vtable=*reinterpret_cast<void***>(s.device);
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(vtable[0]),&deviceOwner);
    const bool wrapped=frameworkExports&&deviceOwner==dxgi;
    fprintf(log.file,"{\"stage\":\"framework\",\"requested\":%s,\"exports\":%s,\"deviceWrapped\":%s}\n",expectFramework?"true":"false",frameworkExports?"true":"false",wrapped?"true":"false");fflush(log.file);
    if(expectFramework&&!wrapped)return 40;
    IDXGIAdapter1* adapter=nullptr;hr=s.factory->EnumAdapterByLuid(s.device->GetAdapterLuid(),IID_PPV_ARGS(&adapter));if(FAILED(hr))return 7;
    DXGI_ADAPTER_DESC1 info{};hr=adapter->GetDesc1(&info);release(adapter);if(FAILED(hr))return 7;
    fprintf(log.file,"{\"stage\":\"rendering-adapter\",\"vendor\":%u,\"device\":%u,\"software\":%s}\n",info.VendorId,info.DeviceId,(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)?"true":"false");fflush(log.file);
    if(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)return 7;
    ffxQueryDescGetVersions versions{};versions.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    versions.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;versions.device=s.device;
    uint64_t count=0;versions.outputCount=&count;auto code=s.query(nullptr,&versions.header);log.result("query-provider-count",code);if(code||!count||count>32)return 8;
    std::vector<uint64_t> ids(count);std::vector<const char*> names(count);versions.versionIds=ids.data();versions.versionNames=names.data();
    code=s.query(nullptr,&versions.header);log.result("query-provider-versions",code);if(code||count>ids.size())return 9;
    uint64_t selected=0;
    for(size_t i=0;i<count;++i){if(!names[i])return 9;std::string name(names[i]);
        fprintf(log.file,"{\"stage\":\"provider\",\"id\":%llu,\"name\":\"%s\"}\n",ids[i],name.c_str());
        if(name=="3.1.6" && ids[i]==17726168133342859270ull)selected=ids[i];}
    fflush(log.file);if(!selected)return 10;
    D3D12_COMMAND_QUEUE_DESC queue{};
    if(FAILED(s.device->CreateCommandQueue(&queue,IID_PPV_ARGS(&s.queue)))||FAILED(s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&s.allocator)))||
       FAILED(s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,s.allocator,nullptr,IID_PPV_ARGS(&s.cmd)))||FAILED(s.cmd->Close())||
       FAILED(s.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s.fence))))return 11;
    s.event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!s.event)return 11;
    for(unsigned trial=0;trial<1;++trial){
        const bool hdr=true;
        ffxOverrideVersion version{};version.header.type=FFX_API_DESC_TYPE_OVERRIDE_VERSION;version.versionId=selected;
        ffxCreateBackendDX12Desc backend{};backend.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;backend.device=s.device;backend.header.pNext=&version.header;
        ffxCreateContextDescFrameGenerationVersion apiVersion{};apiVersion.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION;
        apiVersion.version=FFX_FRAMEGENERATION_VERSION;apiVersion.header.pNext=&backend.header;
        ffxCreateContextDescFrameGeneration create{};create.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;create.header.pNext=&apiVersion.header;
        create.displaySize={Width,Height};create.maxRenderSize={Width,Height};create.backBufferFormat=FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        create.flags=FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING|(hdr?FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE:0);
        code=s.create(&s.context,&create.header,nullptr);log.result("create-context",code);if(code||!s.context)return 12;
        ffxQueryGetProviderVersion active{};active.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
        code=s.query(&s.context,&active.header);log.result("confirm-active-provider",code);if(code||active.versionId!=selected)return 13;
        fprintf(log.file,"{\"stage\":\"active-provider\",\"id\":%llu}\n",active.versionId);fflush(log.file);
        ffxConfigureDescGlobalDebug1 debug{};debug.header.type=FFX_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG1;debug.fpMessage=message;
        code=s.configure(&s.context,&debug.header);log.result("configure-debug-callback",code);if(code)return 14;
        if(!s.cmd && (FAILED(s.allocator->Reset()) ||
            FAILED(s.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,s.allocator,nullptr,IID_PPV_ARGS(&s.cmd))) ||
            FAILED(s.cmd->Close())))return 11;
        if(!resources(s,Width,Height))return 15;

        WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCD2FsrFgOwnedPresentation";
        RegisterClassW(&wc);s.window=CreateWindowW(wc.lpszClassName,L"MCD2 isolated FSR frame generation",WS_OVERLAPPEDWINDOW,0,0,Width,Height,nullptr,nullptr,wc.hInstance,nullptr);if(!s.window)return 25;
        ShowWindow(s.window,SW_SHOW);SetForegroundWindow(s.window);
        ffxCreateContextDescFrameGenerationSwapChainVersionDX12 swapVersion{};swapVersion.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;swapVersion.version=FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;
        DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=Width;desc.Height=Height;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;desc.BufferCount=3;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;desc.Flags=DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 swapCreate{};swapCreate.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;swapCreate.header.pNext=&swapVersion.header;swapCreate.swapchain=&s.swap;swapCreate.hwnd=s.window;swapCreate.desc=&desc;swapCreate.dxgiFactory=s.factory;swapCreate.gameQueue=s.queue;
        code=s.create(&s.swapContext,&swapCreate.header,nullptr);log.result("create-owned-swapchain",code);if(code||!s.swap||!s.swapContext)return 26;
        ffxQueryGetProviderVersion swapActive{};swapActive.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;code=s.query(&s.swapContext,&swapActive.header);log.result("confirm-swapchain-provider",code);if(code||swapActive.versionId!=17752306900579389447ull||!swapActive.versionName||strcmp(swapActive.versionName,"3.1.7"))return 27;
        fprintf(log.file,"{\"stage\":\"swapchain-provider\",\"id\":%llu,\"name\":\"%s\"}\n",swapActive.versionId,swapActive.versionName);fflush(log.file);
        s.waitable=s.swap->GetFrameLatencyWaitableObject();if(!s.waitable)return 28;
        hr=s.swap->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);log.result("set-scRGB-output",hr);if(FAILED(hr))return 29;
        unsigned identity=0;
        for(unsigned phase=0;phase<3;++phase){bool enabled=phase==1;unsigned frames=enabled?60:30;
        const auto beforeReal=s.realCallbacks.load(),beforeGenerated=s.generatedCallbacks.load();UINT beforePresents=0;hr=s.swap->GetLastPresentCount(&beforePresents);log.result("query-before-present-count",hr);if(FAILED(hr))return 34;
        for(unsigned n=0;n<frames;++n){const auto frame=identity++;
            MSG m{};while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}
            if(WaitForSingleObject(s.waitable,30000)!=WAIT_OBJECT_0)return 30;

            if(FAILED(s.allocator->Reset())||FAILED(s.cmd->Reset(s.allocator,nullptr)))return 16;
            auto handle=s.rtv->GetCPUDescriptorHandleForHeapStart();auto stride=s.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            const float colors[7][4]={{.25f,.5f,1,1},{.5f,0,0,0},{0,0,0,0},{1,0,0,0},{0,0,0,0},{0,0,0,0},{11,13,17,1}};
            for(unsigned i=0;i<7;++i){if(frame)transition(s.cmd,s.images[i],i==6?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:Read,D3D12_RESOURCE_STATE_RENDER_TARGET);
                s.cmd->ClearRenderTargetView(handle,colors[i],0,nullptr);
                if(i==0){D3D12_RECT rect{LONG(Width/2),0,LONG(Width),LONG(Height)};const float bright[]={hdr?2.f:.125f,hdr?4.f:.25f,hdr?8.f:.5f,1};s.cmd->ClearRenderTargetView(handle,bright,1,&rect);}
                transition(s.cmd,s.images[i],D3D12_RESOURCE_STATE_RENDER_TARGET,i==6?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:Read);handle.ptr+=stride;}
            ffxConfigureDescFrameGeneration config{};config.header.type=FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
            config.frameGenerationEnabled=enabled;config.swapChain=s.swap;config.presentCallback=compose;config.presentCallbackUserContext=&s;config.frameGenerationCallback=generate;config.frameGenerationCallbackUserContext=&s;config.generationRect={0,0,Width,Height};config.frameID=frame+1;
            code=s.configure(&s.context,&config.header);log.result("configure-frame",code);if(code)return 17;
            ffxDispatchDescFrameGenerationPrepareV2 prepare{};prepare.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2;
            prepare.commandList=s.cmd;prepare.frameID=config.frameID;prepare.renderSize={Width,Height};prepare.motionVectorScale={float(Width),float(Height)};
            prepare.frameTimeDelta=16.667f;prepare.reset=n==0;prepare.cameraNear=.1f;prepare.cameraFar=1000;prepare.cameraFovAngleVertical=1.04719755f;
            prepare.viewSpaceToMetersFactor=1;prepare.depth=ffxApiGetResourceDX12(s.images[1],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            prepare.motionVectors=ffxApiGetResourceDX12(s.images[2],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            // Real camera definition for this owned test scene, not invented game data.
            prepare.cameraUp[1]=1;prepare.cameraRight[0]=1;prepare.cameraForward[2]=1;

            if(enabled){code=s.dispatch(&s.context,&prepare.header);log.result("prepare-owned-inputs",code);if(code)return 18;}
            ID3D12Resource* back=nullptr;hr=s.swap->GetBuffer(s.swap->GetCurrentBackBufferIndex(),IID_PPV_ARGS(&back));if(FAILED(hr))return 31;
            transition(s.cmd,s.images[0],Read,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(s.cmd,back,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_COPY_DEST);s.cmd->CopyResource(back,s.images[0]);transition(s.cmd,back,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PRESENT);transition(s.cmd,s.images[0],D3D12_RESOURCE_STATE_COPY_SOURCE,Read);back->Release();
            if(FAILED(s.cmd->Close()))return 20;ID3D12CommandList* lists[]={s.cmd};s.submitted=true;s.drained=false;s.queue->ExecuteCommandLists(1,lists);
            hr=s.swap->Present(0,0);log.result("present-real-frame",hr);if(FAILED(hr))return 32;
            if(!s.wait())return 21;if(errors||FAILED(s.device->GetDeviceRemovedReason()))return 23;Sleep(16);
        }
        ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12 wait{};wait.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12;
        code=s.dispatch(&s.swapContext,&wait.header);log.result("wait-phase-presents",code);if(code)return 33;
        UINT afterPresents=0;hr=s.swap->GetLastPresentCount(&afterPresents);log.result("query-present-count",hr);if(FAILED(hr))return 34;
        auto real=s.realCallbacks.load()-beforeReal,generated=s.generatedCallbacks.load()-beforeGenerated;
        fprintf(log.file,"{\"stage\":\"phase\",\"phase\":%u,\"enabled\":%s,\"realSubmissions\":%u,\"realCallbacks\":%u,\"generatedCallbacks\":%u,\"DXGIPresentDelta\":%u,\"callbackErrors\":%u}\n",phase,enabled?"true":"false",frames,real,generated,afterPresents-beforePresents,s.callbackErrors.load());fflush(log.file);
        if(real!=frames||s.callbackErrors||(!enabled&&generated)||(enabled&&generated<frames/2))return 35;
        }
        if(!s.retire())return 24;
    }
    fprintf(log.file,"{\"stage\":\"complete\",\"realSubmissions\":120,\"SDKerrors\":%u,\"SDKwarnings\":%u,\"gameFilesChanged\":false,\"presentationQualified\":true}\n",errors.load(),warnings.load());fflush(log.file);return 0;
}
