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
#include "api/include/dx12/ffx_api_dx12.h"
#include "upscalers/include/ffx_upscale.h"

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
static unsigned errors, warnings;
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
    ffxContext context = nullptr;
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
        if (context) {
            auto code = destroy(&context, nullptr);
            outputLog->result("destroy-context-after-fence", code);
            if (code) { retirementFailed = true; return false; }
            outputLog->result("destroy-returned-context-nonnull", context ? 1 : 0);
            // Successful destruction consumes the handle. Some runtime versions
            // leave the caller's pointer unchanged; never destroy it a second time.
            context = nullptr;
        }
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
int main() {
    Log log; fopen_s(&log.file, "fsr-owned-inputs.jsonl", "w"); if (!log.file) return 1; outputLog = &log;
    Session s;
    // The runner checks pinned runtime bytes before execution. Full paths prevent
    // loading similarly named DLLs from a game, system directory or search path.
    auto path = std::make_unique<wchar_t[]>(32768);
    auto size = GetCurrentDirectoryW(32768, path.get()); if (!size || size > 32690) return 2;
    wcscat_s(path.get(), 32768, L"\\amd_fidelityfx_upscaler_dx12.dll");
    s.sdk = LoadLibraryExW(path.get(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    log.result("load-pinned-upscaler", s.sdk ? 0 : GetLastError()); if (!s.sdk) return 3;
    s.create = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(s.sdk, "ffxCreateContext"));
    s.destroy = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(s.sdk, "ffxDestroyContext"));
    s.query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(s.sdk, "ffxQuery"));
    s.dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(s.sdk, "ffxDispatch"));
    if (!s.create || !s.destroy || !s.query || !s.dispatch) return 4;
    auto hr = CreateDXGIFactory1(IID_PPV_ARGS(&s.factory)); log.result("factory", hr); if (FAILED(hr)) return 5;
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&s.device));
    log.result("device", hr); if (FAILED(hr)) return 6;
    IDXGIAdapter1* adapter = nullptr;
    hr = s.factory->EnumAdapterByLuid(s.device->GetAdapterLuid(), IID_PPV_ARGS(&adapter)); if (FAILED(hr)) return 7;
    DXGI_ADAPTER_DESC1 info{}; hr = adapter->GetDesc1(&info); release(adapter); if (FAILED(hr)) return 7;
    fprintf(log.file, "{\"stage\":\"rendering-adapter\",\"vendor\":%u,\"device\":%u,\"software\":%s}\n",
        info.VendorId, info.DeviceId, (info.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)?"true":"false"); fflush(log.file);
    ffxQueryDescGetVersions versions{}; versions.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    versions.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE; versions.device = s.device;
    uint64_t count = 0; versions.outputCount = &count;
    auto code = s.query(nullptr, &versions.header); log.result("query-provider-count", code);
    if (code || !count || count > 32) return 8;
    std::vector<uint64_t> ids(count); std::vector<const char*> names(count);
    versions.versionIds = ids.data(); versions.versionNames = names.data();
    code = s.query(nullptr, &versions.header); log.result("query-provider-versions", code);
    if (code || count > ids.size()) return 9;
    uint64_t selected = 0;
    for (size_t i=0; i<count; ++i) {
        if (!names[i]) return 9;
        std::string name(names[i]);
        // Explicit analytic experiment; never force an unsupported ML provider.
        fprintf(log.file, "{\"stage\":\"provider\",\"id\":%llu,\"name\":\"%s\"}\n", ids[i], name.c_str());
        if (name.find("3.1.5") != std::string::npos) selected = ids[i];
    }
    fflush(log.file); if (!selected) return 10;
    D3D12_COMMAND_QUEUE_DESC queue{};
    if (FAILED(s.device->CreateCommandQueue(&queue, IID_PPV_ARGS(&s.queue))) ||
        FAILED(s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s.allocator))) ||
        FAILED(s.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s.allocator, nullptr, IID_PPV_ARGS(&s.cmd))) ||
        FAILED(s.cmd->Close()) || FAILED(s.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s.fence)))) return 11;
    s.event = CreateEventW(nullptr, FALSE, FALSE, nullptr); if (!s.event) return 11;
    for (unsigned trial=0; trial<10; ++trial) {
        bool hdr = trial >= 5; unsigned mode = trial%5;
        ffxOverrideVersion version{}; version.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION; version.versionId = selected;
        ffxCreateBackendDX12Desc backend{}; backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backend.device = s.device; backend.header.pNext = &version.header;
        unsigned w=0,h=0;
        ffxQueryDescUpscaleGetRenderResolutionFromQualityMode resolution{};
        resolution.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
        resolution.header.pNext = &backend.header; resolution.qualityMode = mode;
        resolution.displayWidth = Width; resolution.displayHeight = Height; resolution.pOutRenderWidth = &w; resolution.pOutRenderHeight = &h;
        code = s.query(nullptr, &resolution.header); log.result("query-render-resolution", code);
        if (code || !w || !h || w>Width || h>Height) return 12;
        fprintf(log.file, "{\"stage\":\"trial\",\"trial\":%u,\"HDR\":%s,\"quality\":%u,\"renderWidth\":%u,\"renderHeight\":%u}\n", trial, hdr?"true":"false", mode,w,h); fflush(log.file);
        ffxCreateContextDescUpscale create{}; create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
        create.header.pNext = &backend.header; create.maxRenderSize = {w,h}; create.maxUpscaleSize = {Width,Height};
        create.flags = FFX_UPSCALE_ENABLE_DEBUG_CHECKING | (hdr?FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE:0); create.fpMessage = message;
        code = s.create(&s.context, &create.header, nullptr); log.result("create-context", code);
        if (code || !s.context) return 13;
        ffxQueryGetProviderVersion active{}; active.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
        code = s.query(&s.context, &active.header); log.result("confirm-active-provider", code);
        if (code || active.versionId != selected) return 14;
        if (!resources(s,w,h)) return 15;
        int phases=0;
        ffxQueryDescUpscaleGetJitterPhaseCount phase{}; phase.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT;
        phase.renderWidth=w; phase.displayWidth=Width; phase.pOutPhaseCount=&phases;
        code=s.query(&s.context,&phase.header); if(code || phases<1) return 16;
        for (unsigned frame=0; frame<16; ++frame) {
            if (FAILED(s.allocator->Reset()) || FAILED(s.cmd->Reset(s.allocator,nullptr))) return 17;
            auto handle=s.rtv->GetCPUDescriptorHandleForHeapStart();
            auto stride=s.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            const float colors[7][4]={{.25f,.5f,1,1},{.5f,0,0,0},{0,0,0,0},{1,0,0,0},{0,0,0,0},{0,0,0,0},{11,13,17,1}};
            for(unsigned i=0;i<7;++i) {
                if(frame) transition(s.cmd,s.images[i],i==6?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:Read,D3D12_RESOURCE_STATE_RENDER_TARGET);
                s.cmd->ClearRenderTargetView(handle,colors[i],0,nullptr);
                if(i==0) { D3D12_RECT rect{LONG(w/2),0,LONG(w),LONG(h)};
                    const float bright[]={hdr?2.f:.125f,hdr?4.f:.25f,hdr?8.f:.5f,1}; s.cmd->ClearRenderTargetView(handle,bright,1,&rect); }
                transition(s.cmd,s.images[i],D3D12_RESOURCE_STATE_RENDER_TARGET,i==6?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:Read);
                handle.ptr+=stride;
            }
            ffxDispatchDescUpscale dispatch{}; dispatch.header.type=FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
            dispatch.commandList=s.cmd; dispatch.color=ffxApiGetResourceDX12(s.images[0],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatch.depth=ffxApiGetResourceDX12(s.images[1],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatch.motionVectors=ffxApiGetResourceDX12(s.images[2],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatch.exposure=ffxApiGetResourceDX12(s.images[3],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatch.reactive=ffxApiGetResourceDX12(s.images[4],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatch.transparencyAndComposition=ffxApiGetResourceDX12(s.images[5],FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
            dispatch.output=ffxApiGetResourceDX12(s.images[6],FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            ffxQueryDescUpscaleGetJitterOffset jitter{}; jitter.header.type=FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET;
            jitter.index=int(frame); jitter.phaseCount=phases; jitter.pOutX=&dispatch.jitterOffset.x; jitter.pOutY=&dispatch.jitterOffset.y;
            if(s.query(&s.context,&jitter.header)) return 18;
            dispatch.motionVectorScale={float(w),float(h)}; dispatch.renderSize={w,h}; dispatch.upscaleSize={Width,Height};
            dispatch.frameTimeDelta=16.667f; dispatch.preExposure=1; dispatch.reset=frame==0 || frame==8;
            dispatch.cameraNear=.1f; dispatch.cameraFar=1000; dispatch.cameraFovAngleVertical=1.04719755f; dispatch.viewSpaceToMetersFactor=1;
            code=s.dispatch(&s.context,&dispatch.header); log.result("dispatch-owned-inputs",code); if(code) return 19;
            transition(s.cmd,s.images[6],D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION source{}; source.pResource=s.images[6]; source.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION dest{}; dest.pResource=s.readback; dest.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dest.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,Width,Height,1,Width*8};
            s.cmd->CopyTextureRegion(&dest,0,0,0,&source,nullptr);
            transition(s.cmd,s.images[6],D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            if(FAILED(s.cmd->Close())) return 20;
            ID3D12CommandList* lists[]={s.cmd}; s.submitted=true; s.drained=false; s.queue->ExecuteCommandLists(1,lists);
            if(!s.wait()) return 21;
            if(!checkOutput(s,hdr,frame)) return 22;
            if(errors || FAILED(s.device->GetDeviceRemovedReason())) return 23;
        }
        if(!s.retire()) return 24;
    }
    fprintf(log.file,"{\"stage\":\"complete\",\"dispatches\":160,\"SDKerrors\":%u,\"SDKwarnings\":%u,\"gameFilesChanged\":false}\n",errors,warnings);
    fflush(log.file); return 0;
}
