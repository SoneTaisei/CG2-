#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <cstdint>

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    bool Initialize(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, UINT maxParticles = 1024);
    // テクスチャハンドルを渡すためシグネチャを変更
    void Update();
    void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
    void Release();

private:
    struct Instance {
        DirectX::XMFLOAT4X4 world;
    };

    struct Vertex {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 uv;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferUpload;

    // 追加: SRV ヒープを保持しておく
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

    UINT m_maxParticles = 0;
    UINT m_instanceCount = 0;

    // CPU-side instance data
    std::vector<Instance> m_instances;

    // 固定ステップの内部時間（既存実装に合わせる）
    float m_time = 0.0f;
    static constexpr float kFixedDt = 1.0f / 60.0f;

    // 頂点/インデックス情報
    uint32_t m_indexCount = 0;
};

