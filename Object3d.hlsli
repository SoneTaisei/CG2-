#define int32_t int
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct Material {
    float4 color;
    int enebleLighting;
};

struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
};

struct DirectionalLight {
    float4 color; //!< ライトの色
    float3 direction; //!< ライトの向き
    float intensity; //!< 光度
};

struct ViewProjection {
    matrix viewProjectionMatrix; // ビュー・プロジェクション行列
    float3 cameraPosition; // カメラの位置
    float padding; // パディング（16バイト単位のため）
};

