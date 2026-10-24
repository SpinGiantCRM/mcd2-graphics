// Preserve stored RGB code values. The game's RGB-only scene draw leaves its
// backbuffer alpha undefined; the owned HUD-less snapshot supplies opaque alpha.
Texture2D<float4> scene : register(t0);
RWTexture2D<float4> snapshot : register(u0);
[numthreads(8, 8, 1)]
void main(uint3 p : SV_DispatchThreadID) {
    uint width,height;
    snapshot.GetDimensions(width,height);
    if(p.x<width&&p.y<height)
        snapshot[p.xy]=float4(scene.Load(int3(p.xy,0)).rgb,1.0);
}
