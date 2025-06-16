#include "object3d.hlsli"

Texture2D<float4> gTexture : register(t3);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

//cbuffer ViewProjection : register(b3) {
//    matrix viewProjectionMatrix; // 64バイト
//    float3 cameraPosition; // 12バイト
//    float padding; // 4バイト → 合計80バイト
//};

//ConstantBuffer<ViewProjection> gViewProjection : register(b1);

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

//cbuffer LightCB : register(b3) {
//    DirectionalLight light;
//};

//SamplerState smp : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET {
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-gDirectionalLight.direction);

    // Lambert反射モデル
    float4 transformedUV = mul(float4(input.texcoord,0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float4 color;

    if (gMaterial.enableLighting != 0) {
        // HalfLambertでライティングの影を滑らかにする
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
    } else {
        color = gMaterial.color * textureColor;
    }
    
    return color;
}