#include "object3d.hlsli"
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

cbuffer gMaterial : register(b0) {
    Material gMaterial;
}

cbuffer gDirectionalLight : register(b4) {
    DirectionalLight gDirectionalLight;
}

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
    return output;
}