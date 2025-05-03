#include "object3d.hlsli"

Texture2D<float4> gTexture : register(t3);
SamplerState gSampler : register(s0);

struct Material {
    float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);
struct PixelShaderOutput {
    // ピクセルシェーダーを結合して出力する
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = gMaterial.color * textureColor;
    return output;
}