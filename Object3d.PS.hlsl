struct Material {
    float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);
struct PixelShaderOutput {
    // ピクセルシェーダーを結合して出力する
    float4 color : SV_TARGET0;
};

PixelShaderOutput main() {
    PixelShaderOutput output;
    output.color = gMaterial.color;
    return output;
}