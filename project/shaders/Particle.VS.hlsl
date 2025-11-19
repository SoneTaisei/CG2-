#include "Particle.hlsli"
StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

cbuffer gMaterial : register(b0) {
    Material gMaterial;
}

cbuffer gDirectionalLight : register(b4) {
    DirectionalLight gDirectionalLight;
}

VertexShaderOutput main(VertexShaderInput input, int32_t instanceId : SV_InstanceID) {
    VertexShaderOutput output;
    
    // これで下の計算式にある gTransformationMatrix が使えるようになります
    TransformationMatrix gTransformationMatrices[] = gTransformationMatrices[instanceId];
    
    output.position = mul(input.position, gTransformationMatrices[instanceId].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrices[instanceId].World));
    
    return output;
}