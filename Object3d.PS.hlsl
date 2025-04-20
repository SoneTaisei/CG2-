struct PixelShaderOutput{
    // ピクセルシェーダーを結合して出力する
    float4 color : SV_Target;
};

PixelShaderOutput main(){
    PixelShaderOutput output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}