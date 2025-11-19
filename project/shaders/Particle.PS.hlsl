#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t3);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput {
    // ピクセルシェーダーを結合して出力する
    float4 color : SV_TARGET0;
};

cbuffer MaterialCB : register(b3) {
    Material material;
};

cbuffer TransformCB : register(b4) {
    TransformationMatrix transform;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    // 1. テクスチャ座標の変換
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    
    // 2. テクスチャから色を取得
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // 3. 色の計算（ライティングなし。単純に掛け合わせる）
    output.color = gMaterial.color * textureColor;
    
    // 4. 透明部分の破棄（α値が0なら描画しない）
    if (output.color.a == 0.0) {
        discard;
    }

    return output;
}