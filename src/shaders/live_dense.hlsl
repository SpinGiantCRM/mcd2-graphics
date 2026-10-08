// Current-frame conversion. Depth is sampled through its original plane view,
// avoiding an incompatible typeless-depth -> R32 texture copy.
cbuffer Pass : register(b0) { float4 passData[21]; }
cbuffer View : register(b1) { float4 viewData[158]; }
Texture2D<float4> packedVelocity : register(t0);
Texture2D<float> sceneDepth : register(t1);
StructuredBuffer<float4> exposureData : register(t2);
RWTexture2D<float2> denseMotion : register(u0);
RWTexture2D<float> exposureTexture : register(u1);
RWTexture2D<float> ownedDepth : register(u2);
[numthreads(8,8,1)]
void main(uint3 tid : SV_DispatchThreadID) {
 uint2 size=uint2(passData[9].xy);
#ifdef MCD2_ACTIVE_VIEW_RECT
 // FSR consumes the active viewport, excluding the allocation's padded edge.
 // Native/DLSS retain the original full-texture conversion below.
 uint4 rect=asuint(passData[10]);
 size=rect.zw+1-rect.xy;
#endif
 if(any(tid.xy>=size))return;
 float2 ndc=float2((tid.x+.5)*2./size.x-1.,1.-(tid.y+.5)*2./size.y);
 float2 unjittered=ndc-viewData[144].xy;
 float depth=sceneDepth.Load(int3(tid.xy,0));
 float4 clip=float4(unjittered,depth,1.);
 float4 previous=clip.x*viewData[136]+clip.y*viewData[137]+clip.z*viewData[138]+viewData[139];
 float2 backward=previous.xy/previous.w-unjittered;
 float2 packed=packedVelocity.Load(int3(tid.xy,0)).xy;
 if(packed.x>0.)backward=-(packed*2.0040080547332764-1.0019887685775757)*abs(packed*4.008016109466553-2.0039775371551514);
 denseMotion[tid.xy]=backward*float2(float(size.x)*.5,-float(size.y)*.5);
 ownedDepth[tid.xy]=depth;
 if(all(tid.xy==uint2(0,0)))exposureTexture[uint2(0,0)]=exposureData[0].x;
}
