#include "Particle.hlsli"
StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t0);

cbuffer gMaterial : register(b0) {
    Material gMaterial;
}

VertexShaderOutput main(VertexShaderInput input,uint32_t instanceID:SV_InstanceID ) {
    VertexShaderOutput output;
    
    output.position = mul(input.position, gTransformationMatrix[instanceID].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix[instanceID].World));
    
    return output;
}