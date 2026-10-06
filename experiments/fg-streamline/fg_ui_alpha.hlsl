Texture2D<float4> sourceUI : register(t0);
RWTexture2D<float> targetAlpha : register(u0);
[numthreads(8, 8, 1)]
void main(uint3 pixel : SV_DispatchThreadID) {
 uint width, height;
 targetAlpha.GetDimensions(width, height);
 if (pixel.x < width && pixel.y < height)
  targetAlpha[pixel.xy] = sourceUI.Load(int3(pixel.xy, 0)).a;
}
