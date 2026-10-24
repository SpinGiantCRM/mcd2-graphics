// Owned-input compute qualification only. Never opens a game, save or swapchain.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include "mfg_headless.hpp"
#include "mfg_owned_pattern.hpp"
#include "../../src/providers/mfg_phase_plan.hpp"

namespace fs=std::filesystem;
constexpr unsigned Width=1280,Height=720,Frames=12;
constexpr auto Read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
constexpr char RuntimeHash[]="f131734e9bf1fd191a9f60fad54231cd88e45ada3c957bd02c8763f8e9ad4440";
FILE* evidence{};
void require(bool ok,const char* stage) {
    if(!ok) { fprintf(evidence,"{\"stage\":\"failure\",\"operation\":\"%s\"}\n",stage); fflush(evidence); throw std::runtime_error(stage); }
}
void api(const char* stage,NVSDK_NGX_Result result) {
    fprintf(evidence,"{\"stage\":\"%s\",\"result\":%u}\n",stage,unsigned(result)); fflush(evidence);
    require(result==NVSDK_NGX_Result_Success,stage);
}
// Keep the file open without write/delete sharing through module load and use.
// This closes the hash/load replacement window for this isolated experiment.
HANDLE verify_runtime(const fs::path& path) {
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return nullptr;
    BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{};
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    DWORD objectSize{},copied{};
    ok=ok && BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&copied,0)>=0;
    std::vector<unsigned char> object(objectSize),buffer(65536);
    ok=ok && BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0)>=0;
    while(ok) {
        DWORD bytes{}; ok=ReadFile(file,buffer.data(),DWORD(buffer.size()),&bytes,nullptr)!=0;
        if(!ok || !bytes) break;
        ok=BCryptHashData(hash,buffer.data(),bytes,0)>=0;
    }
    unsigned char digest[32]{}; char hex[65]{};
    ok=ok && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash) BCryptDestroyHash(hash); if(alg) BCryptCloseAlgorithmProvider(alg,0);
    for(unsigned i=0;i<32;++i) snprintf(hex+2*i,3,"%02x",digest[i]);
    if(!ok || strcmp(hex,RuntimeHash)) { CloseHandle(file); return nullptr; }
    return file;
}
template<class T> void drop(T*& p) { if(p) { p->Release(); p=nullptr; } }
void barrier(ID3D12GraphicsCommandList* cmd,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to) {
    D3D12_RESOURCE_BARRIER b{}; b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to}; cmd->ResourceBarrier(1,&b);
}
struct Image {
    ID3D12Resource* texture{}; ID3D12Resource* upload{}; ID3D12Resource* readback{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{}; uint64_t bytes{};
    DXGI_FORMAT format{}; bool input{};
};
struct Session {
    MfgHeadless backend;
    MfgNgxParameters params;
    ID3D12Device* device{}; ID3D12CommandQueue* queue{};
    ID3D12CommandAllocator* allocator{}; ID3D12GraphicsCommandList* cmd{};
    ID3D12Fence* fence{}; HANDLE event{},runtimeFile{}; uint64_t fenceValue{};
    IDXGIFactory6* factory{}; IDXGIAdapter1* adapter{};
    std::array<Image,9> images{}; ID3D12Resource* scratch{};
    bool submitted{},drained=true,initialized{};
    // Deliberately no destructor: a failed completion/release retains all live
    // resources and the module until this bounded isolated process exits.
    bool wait() {
        const auto next=++fenceValue;
        drained=SUCCEEDED(queue->Signal(fence,next)) && SUCCEEDED(fence->SetEventOnCompletion(next,event)) &&
            WaitForSingleObject(event,30000)==WAIT_OBJECT_0;
        return drained;
    }
    void begin() {
        require(drained,"recording-drained");
        require(SUCCEEDED(allocator->Reset()) && SUCCEEDED(cmd->Reset(allocator,nullptr)),"reset-recording");
    }
    void submit() {
        require(SUCCEEDED(cmd->Close()),"close-recording");
        ID3D12CommandList* lists[]={cmd}; queue->ExecuteCommandLists(1,lists);
        submitted=true; drained=false; require(wait(),"queue-completion");
    }
    ID3D12Resource* buffer(D3D12_HEAP_TYPE type,uint64_t bytes,D3D12_RESOURCE_STATES state) {
        D3D12_HEAP_PROPERTIES heap{}; heap.Type=type;
        D3D12_RESOURCE_DESC desc{}; desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width=bytes; desc.Height=desc.DepthOrArraySize=desc.MipLevels=1;
        desc.SampleDesc.Count=1; desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ID3D12Resource* result{};
        require(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result))),"create-buffer");
        return result;
    }
    void image(unsigned index,DXGI_FORMAT format,bool input) {
        auto& image=images[index]; image.format=format; image.input=input;
        D3D12_RESOURCE_DESC d{}; d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width=Width; d.Height=Height; d.DepthOrArraySize=d.MipLevels=1;
        d.SampleDesc.Count=1; d.Format=format;
        d.Flags=input ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D3D12_HEAP_PROPERTIES heap{}; heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        require(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,input?Read:D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,IID_PPV_ARGS(&image.texture))),"create-image");
        device->GetCopyableFootprints(&d,0,1,0,&image.footprint,nullptr,nullptr,&image.bytes);
        image.upload=buffer(D3D12_HEAP_TYPE_UPLOAD,image.bytes,D3D12_RESOURCE_STATE_GENERIC_READ);
        if(!input) image.readback=buffer(D3D12_HEAP_TYPE_READBACK,image.bytes,D3D12_RESOURCE_STATE_COPY_DEST);
    }
    void upload(unsigned index,unsigned frame) {
        auto& image=images[index]; unsigned char* mapped{};
        D3D12_RANGE empty{}; require(SUCCEEDED(image.upload->Map(0,&empty,reinterpret_cast<void**>(&mapped))),"map-upload");
        memset(mapped,0,size_t(image.bytes));
        for(unsigned y=0;y<Height;++y) {
            auto row=mapped+image.footprint.Offset+uint64_t(y)*image.footprint.Footprint.RowPitch;
            for(unsigned x=0;x<Width;++x) {
                if(index==0) {
                    // Exactly representable half floats; a translating striped
                    // scene supplies disjoint time positions without game data.
                    auto pixel=reinterpret_cast<uint16_t*>(row)+4*x;
                    const bool stripe=((x+Width-frame*8)%Width/24)%2!=0;
                    pixel[0]=stripe?0x3a00:0x3400; pixel[1]=y<Height/2?0x3800:0x3400;
                    pixel[2]=0x3800; pixel[3]=0x3c00;
                } else if(index==1) reinterpret_cast<float*>(row)[x]=0.5f;
                else if(index==2) { auto p=reinterpret_cast<uint16_t*>(row)+2*x; p[0]=0xc800; p[1]=0; } // -8px current→previous
            }
        }
        if(index==0 && frame>=Frames-2) {
            const char* name=frame==Frames-2?"previous-real.rgba16f":"current-real.rgba16f";
            FILE* dump{}; require(fopen_s(&dump,name,"wb")==0 && dump,"open-real-reference");
            for(unsigned y=0;y<Height;++y) require(fwrite(mapped+image.footprint.Offset+uint64_t(y)*image.footprint.Footprint.RowPitch,8,Width,dump)==Width,"write-real-reference");
            require(fclose(dump)==0,"close-real-reference");
        }
        image.upload->Unmap(0,nullptr);
        const auto state=image.input?Read:D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier(cmd,image.texture,state,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION to{},from{}; to.pResource=image.texture; to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        from.pResource=image.upload; from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; from.PlacedFootprint=image.footprint;
        cmd->CopyTextureRegion(&to,0,0,0,&from,nullptr); barrier(cmd,image.texture,D3D12_RESOURCE_STATE_COPY_DEST,state);
    }
    void readback(unsigned index) {
        auto& image=images[index]; barrier(cmd,image.texture,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION to{},from{}; from.pResource=image.texture; from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        to.pResource=image.readback; to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; to.PlacedFootprint=image.footprint;
        cmd->CopyTextureRegion(&to,0,0,0,&from,nullptr); barrier(cmd,image.texture,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    void retire() {
        require(!submitted || wait(),"retire-queue-completion");
        // Invalidate the closed recording, then prove completion again before
        // releasing vendor context or any resource it may still reference.
        drop(cmd); require(wait(),"retire-post-invalidation-fence");
        if(backend.feature) { api("release",backend.release(backend.feature)); backend.feature=nullptr; }
        if(initialized) { api("shutdown",backend.shutdown(device)); initialized=false; }
        for(auto& image:images) { drop(image.texture); drop(image.upload); drop(image.readback); }
        drop(scratch); drop(allocator); drop(fence); drop(queue); drop(device); drop(adapter); drop(factory);
        if(event) { CloseHandle(event); event=nullptr; }
        // Module + locked backing file remain process-owned, including success.
    }
};
void inspect_image(Session& s,unsigned frame,unsigned slot,unsigned multiplier) {
    auto& image=s.images[3+slot]; unsigned char* mapped{};
    D3D12_RANGE range{0,SIZE_T(image.bytes)};
    require(SUCCEEDED(image.readback->Map(0,&range,reinterpret_cast<void**>(&mapped))),"map-readback");
    bool finite=true,nonzero=false; uint64_t hash=14695981039346656037ull;
    FILE* dump=nullptr;
    if(frame==Frames-1) {
        char name[80]{}; snprintf(name,sizeof(name),"phase-%u-of-%u.rgba16f",slot,multiplier);
        require(fopen_s(&dump,name,"wb")==0 && dump!=nullptr,"open-owned-image");
    }
    for(unsigned y=0;y<Height;++y) {
        auto row=mapped+image.footprint.Offset+uint64_t(y)*image.footprint.Footprint.RowPitch;
        if(dump) require(fwrite(row,8,Width,dump)==Width,"write-owned-image");
        for(unsigned x=0;x<Width*4;++x) {
            const auto value=reinterpret_cast<uint16_t*>(row)[x];
            finite=finite && (value&0x7c00)!=0x7c00;
            if(x%4!=3) nonzero=nonzero || (value&0x7fff)!=0;
        }
        for(unsigned x=0;x<Width*8;++x) { hash^=row[x]; hash*=1099511628211ull; }
    }
    if(dump) require(fclose(dump)==0,"close-owned-image");
    std::vector<float> red(Width);
    auto middle=reinterpret_cast<uint16_t*>(mapped+image.footprint.Offset+uint64_t(Height/2)*image.footprint.Footprint.RowPitch);
    for(unsigned x=0;x<Width;++x) {
        const auto h=middle[4*x]; const auto e=(h>>10)&31, m=h&1023;
        float value=e==0?std::ldexp(float(m),-24):e==31?std::numeric_limits<float>::infinity():std::ldexp(1.f+float(m)/1024,int(e)-15);
        red[x]=(h&0x8000)?-value:value;
    }
    const auto fit=mcd2::mfg::fit_phase(red.data(),Width,(frame-1)*8);
    D3D12_RANGE empty{}; image.readback->Unmap(0,&empty);
    fprintf(evidence,"{\"stage\":\"image\",\"epoch\":1,\"previous\":%u,\"current\":%u,\"slot\":%u,\"denominator\":%u,\"result\":1,\"finite\":%s,\"nonzero\":%s,\"hash\":\"%016llx\",\"bestOffset\":%u,\"meanAbsoluteError\":%.8f}\n",
        frame,frame+1,slot,multiplier,finite?"true":"false",nonzero?"true":"false",static_cast<unsigned long long>(hash),fit.offset,fit.meanAbsoluteError);
    fflush(evidence); require(finite && nonzero,"owned-output-content");
}
int wmain(int argc,wchar_t** argv) {
    if(fopen_s(&evidence,"mfg-evidence.jsonl","w")!=0 || !evidence) return 2;
    // Leak on a failed fence/SDK retirement by design; the isolated process is
    // the containment boundary. No callbacks/resources may escape into a game.
    auto s=new Session;
    try {
        require(argc==3,"arguments-runtime-and-multiplier");
        const std::wstring count=argv[2]; require(count.size()==1 && count[0]>=L'2' && count[0]<=L'6',"multiplier");
        const unsigned multiplier=unsigned(count[0]-L'0');
        const auto runtime=fs::path(argv[1]);
        require(runtime.is_absolute() && runtime.filename()==L"dlss-enabler-headless.dll","headless-path");
        s->runtimeFile=verify_runtime(runtime); require(s->runtimeFile!=nullptr,"runtime-hash");
        require(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&s->factory))),"create-factory");
        for(unsigned i=0;;++i) {
            IDXGIAdapter1* adapter{};
            if(s->factory->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter))==DXGI_ERROR_NOT_FOUND) break;
            if(!adapter) continue; DXGI_ADAPTER_DESC1 desc{}; auto described=adapter->GetDesc1(&desc);
            if(SUCCEEDED(described) && !(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE) &&
               SUCCEEDED(D3D12CreateDevice(adapter,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&s->device)))) {
                s->adapter=adapter;
                fprintf(evidence,"{\"stage\":\"adapter\",\"vendor\":%u,\"device\":%u,\"software\":false}\n",desc.VendorId,desc.DeviceId); break;
            }
            adapter->Release();
        }
        require(s->device!=nullptr,"hardware-device");
        D3D12_COMMAND_QUEUE_DESC q{}; q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
        require(SUCCEEDED(s->device->CreateCommandQueue(&q,IID_PPV_ARGS(&s->queue))),"create-queue");
        require(SUCCEEDED(s->device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&s->allocator))),"create-allocator");
        require(SUCCEEDED(s->device->CreateCommandList(0,q.Type,s->allocator,nullptr,IID_PPV_ARGS(&s->cmd))),"create-command-list");
        require(SUCCEEDED(s->cmd->Close()),"initial-close");
        require(SUCCEEDED(s->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&s->fence))),"create-fence");
        s->event=CreateEventW(nullptr,FALSE,FALSE,nullptr); require(s->event!=nullptr,"create-event");
        require(s->backend.load(runtime.c_str()),"headless-exports");
        api("init",s->backend.init(0,fs::current_path().c_str(),s->device,NVSDK_NGX_Version_API,nullptr)); s->initialized=true;
        api("populate",s->backend.populate(&s->params));
        unsigned maxGenerated{}; const auto cap=s->params.Get("DLSSG.MultiFrameCountMax",&maxGenerated);
        fprintf(evidence,"{\"stage\":\"capability\",\"generatedMax\":%u}\n",maxGenerated);
        require(cap==NVSDK_NGX_Result_Success && maxGenerated>=multiplier-1 && maxGenerated<=5,"phase-capability");
        const auto plan=mcd2::mfg::make_plan(mcd2::mfg::Mode::Software,multiplier,{maxGenerated+1,0,false});
        require(plan.has_value(),"software-phase-plan");
        s->image(0,DXGI_FORMAT_R16G16B16A16_FLOAT,true); s->image(1,DXGI_FORMAT_R32_FLOAT,true);
        s->image(2,DXGI_FORMAT_R16G16_FLOAT,true);
        for(unsigned i=3;i<3+multiplier;++i) s->image(i,DXGI_FORMAT_R16G16B16A16_FLOAT,false);
        auto& p=s->params;
        p.Set("Width",Width); p.Set("Height",Height); p.Set("OutWidth",Width); p.Set("OutHeight",Height);
        p.Set("CreationNodeMask",1u); p.Set("VisibilityNodeMask",1u);
        p.Set("DLSSG.BackbufferWidth",Width); p.Set("DLSSG.BackbufferHeight",Height);
        p.Set("DLSSG.CmdQueue",static_cast<void*>(s->queue)); p.Set("DLSSG.CmdAlloc",static_cast<void*>(s->allocator));
        p.Set("DLSSG.ColorBuffersHDR",1u); p.Set("DLSSG.DepthInverted",0u);
        p.Set("DLSSG.MvecDilated",1u); p.Set("DLSSG.MvecJittered",0u);
        p.Set("DLSSG.MvecScaleX",1.f); p.Set("DLSSG.MvecScaleY",1.f);
        p.Set("DLSSG.CameraNear",0.1f); p.Set("DLSSG.CameraFar",1000.f);
        p.Set("DLSSG.CameraFOV",1.04719755f); p.Set("DLSSG.CameraAspectRatio",float(Width)/Height);
        p.Set("DLSSG.JitterOffsetX",0.f); p.Set("DLSSG.JitterOffsetY",0.f);
        p.Set("FrameTimeDeltaInMsec",16.666666f); p.Set("DLSSG.EnableInterp",1u);
        std::array<float,16> matrix{}; for(unsigned i=0;i<4;++i) matrix[5*i]=1.f;
        p.Set("DLSSG.CameraViewToClip",static_cast<void*>(matrix.data()));
        p.Set("DLSSG.ClipToPrevClip",static_cast<void*>(matrix.data()));
        for(const char* name:{"Backbuffer","Depth","MVecs","HUDLess"}) {
            const std::string prefix=std::string("DLSSG.")+name;
            p.Set((prefix+"SubrectBaseX").c_str(),0u); p.Set((prefix+"SubrectBaseY").c_str(),0u);
            p.Set((prefix+"SubrectWidth").c_str(),Width); p.Set((prefix+"SubrectHeight").c_str(),Height);
        }
        p.Set("DLSSG.Backbuffer",s->images[0].texture); p.Set("DLSSG.HUDLess",s->images[0].texture);
        p.Set("DLSSG.Depth",s->images[1].texture); p.Set("DLSSG.MVecs",s->images[2].texture);
        p.Set("DLSSG.OutputReal",s->images[3].texture);
        p.Set("DLSSG.OutputInterpolated",s->images[4].texture);
        size_t scratchBytes{}; api("scratch",s->backend.scratch(NVSDK_NGX_Feature_FrameGeneration,&p,&scratchBytes));
        require(scratchBytes<=512ull*1024*1024,"scratch-bound");
        if(scratchBytes) { s->scratch=s->buffer(D3D12_HEAP_TYPE_DEFAULT,scratchBytes,D3D12_RESOURCE_STATE_COMMON); p.Set("Scratch",s->scratch); }
        require(p.healthy(),"create-parameters"); s->begin();
        api("create",s->backend.create(s->cmd,NVSDK_NGX_Feature_FrameGeneration,&p,&s->backend.feature));
        require(s->backend.feature!=nullptr,"created-feature"); s->submit();
        unsigned images=0;
        for(unsigned frame=0;frame<Frames;++frame) {
            s->begin(); for(unsigned i=0;i<3+multiplier;++i) s->upload(i,frame); s->submit();
            mcd2::mfg::Batch batch;
            const mcd2::mfg::RealPair pair{1,frame,frame+1};
            if(frame>=2) require(batch.begin(*plan,pair),"begin-real-pair");
            p.Set("DLSSG.FrameIndex",frame); p.Set("DLSSG.FrameCount",frame);
            p.Set("DLSSG.Reset",frame==0?1u:0u);
            p.Set("DLSSG.MultiFrameCount",plan->count);
            for(unsigned slot=1;slot<=plan->count;++slot) {
                p.Set("DLSSG.MultiFrameIndex",slot); p.Set("DLSSG.OutputInterpolated",s->images[3+slot].texture);
                require(p.healthy(),"evaluate-parameters"); s->begin();
                api("evaluate",s->backend.evaluate(s->cmd,s->backend.feature,&p,nullptr));
                s->readback(3+slot); s->submit();
                if(frame>=2) {
                    require(batch.record(pair,slot,mcd2::mfg::Provider::Software,reinterpret_cast<uint64_t>(s->images[3+slot].texture),true),"record-phase");
                    inspect_image(*s,frame,slot,multiplier); ++images;
                }
            }
            if(frame>=2) require(batch.complete(),"complete-real-pair");
        }
        s->retire();
        fprintf(evidence,"{\"stage\":\"complete\",\"images\":%u,\"swapchain\":false,\"gameIntegrationQualified\":false,\"moduleRetainedUntilExit\":true}\n",images);
        fclose(evidence); return 0;
    } catch(...) {
        // Attempt retirement only with owned queue completion available. If it
        // cannot prove completion or the SDK refuses release, retain until exit.
        try { if(s->queue && s->fence && s->event && s->cmd) s->retire(); } catch(...) {}
        fclose(evidence); return 1;
    }
}
