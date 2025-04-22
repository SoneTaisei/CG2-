struct VertexShaderOutput{
    // 頂点シェーダー
    float4 positon : SV_POSITION;
};

struct VertexShaderInput{
    // 三角形の頂点を入力
    float4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input){
    VertexShaderOutput output;
    output.positon = input.position;
    return output;
}