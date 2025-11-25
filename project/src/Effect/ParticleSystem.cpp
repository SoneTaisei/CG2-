#include "ParticleSystem.h"
#include <stdexcept>
#include <algorithm>
#include <cstring>
using namespace DirectX;
using Microsoft::WRL::ComPtr;

ParticleSystem::ParticleSystem() = default;
ParticleSystem::~ParticleSystem() { Release(); }

static void CreateBufferResource(ID3D12Device* device,
    const void* srcData, UINT64 byteSize,
    ComPtr<ID3D12Resource>& defaultRes,
    ComPtr<ID3D12Resource>& uploadRes)
{
    D3D12_HEAP_PROPERTIES heapDefault = {};
    heapDefault.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = byteSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(device->CreateCommittedResource(
        &heapDefault, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultRes))))
    {
        throw std::runtime_error("CreateCommittedResource default failed");
    }

    D3D12_HEAP_PROPERTIES heapUpload = {};
    heapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
    if (FAILED(device->CreateCommittedResource(
        &heapUpload, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadRes))))
    {
        throw std::runtime_error("CreateCommittedResource upload failed");
    }

    void* mapped = nullptr;
    D3D12_RANGE r{0,0};
    if (SUCCEEDED(uploadRes->Map(0, &r, &mapped))) {
        std::memcpy(mapped, srcData, (size_t)byteSize);
        D3D12_RANGE written{0, (SIZE_T)byteSize};
        uploadRes->Unmap(0, &written);
    }
}

bool ParticleSystem::Initialize(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, UINT maxParticles) {
    if (!device) return false;
    m_maxParticles = maxParticles;
    m_instances.resize(m_maxParticles);

    for (UINT i = 0; i < m_maxParticles; ++i) {
        XMStoreFloat4x4(&m_instances[i].world, XMMatrixIdentity());
    }
    m_instanceCount = 0;
    m_time = 0.0f;

    // SRV ヒープを保持（nullptr 可）
    if (srvHeap) {
        m_srvHeap = srvHeap;
    }

    // インスタンスバッファ (UPLOAD)
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = sizeof(Instance) * m_maxParticles;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProp = {};
        heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

        if (FAILED(device->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_instanceBuffer)))) {
            return false;
        }
    }

    // quad の頂点・インデックス
    Vertex vertices[] = {
        { { -0.5f,  0.5f, 0.0f }, {0.0f, 0.0f} },
        { {  0.5f,  0.5f, 0.0f }, {1.0f, 0.0f} },
        { { -0.5f, -0.5f, 0.0f }, {0.0f, 1.0f} },
        { {  0.5f, -0.5f, 0.0f }, {1.0f, 1.0f} },
    };
    uint16_t indices[] = { 0,1,2, 2,1,3 };
    m_indexCount = _countof(indices);

    try {
        CreateBufferResource(device, vertices, sizeof(vertices), m_vertexBuffer, m_vertexBufferUpload);
        CreateBufferResource(device, indices, sizeof(indices), m_indexBuffer, m_indexBufferUpload);
    } catch (...) {
        return false;
    }

    return true;
}

void ParticleSystem::Update() {
    m_time += kFixedDt;

    m_instanceCount = std::min<UINT>(m_maxParticles, 200);
    for (UINT i = 0; i < m_instanceCount; ++i) {
        float t = float(i) + m_time;
        XMMATRIX m = XMMatrixTranslation(sinf(t * 0.5f) * 2.0f, cosf(t * 0.3f) * 1.0f, 0.0f);
        XMStoreFloat4x4(&m_instances[i].world, m);
    }

    Instance* mapped = nullptr;
    D3D12_RANGE range{ 0, 0 };
    if (SUCCEEDED(m_instanceBuffer->Map(0, &range, reinterpret_cast<void**>(&mapped)))) {
        std::memcpy(mapped, m_instances.data(), sizeof(Instance) * m_instanceCount);
        D3D12_RANGE writtenRange{ 0, sizeof(Instance) * m_instanceCount };
        m_instanceBuffer->Unmap(0, &writtenRange);
    }
}

void ParticleSystem::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
    if (!cmdList || m_instanceCount == 0) return;

    // 1) SRV ヒープをセット（存在すれば）
    if (m_srvHeap) {
        ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);
    }

    // 2) ルートにディスクリプタテーブルをセットする
    // 注意: rootIndex はあなたのルートシグネチャでテクスチャが定義されているインデックスに合わせてください。
    // 例: ルートパラメータ0が DescriptorTable なら 0 を指定
    const UINT rootIndexForTexture = 0; // <--- プロジェクトに合わせて調整すること
    cmdList->SetGraphicsRootDescriptorTable(rootIndexForTexture, textureHandle);

    // 頂点/インスタンス/インデックスビューのセット
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    vbv.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(Vertex);
    vbv.SizeInBytes = sizeof(Vertex) * 4;

    D3D12_VERTEX_BUFFER_VIEW instanceView{};
    instanceView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
    instanceView.StrideInBytes = sizeof(Instance);
    instanceView.SizeInBytes = sizeof(Instance) * m_instanceCount;

    D3D12_INDEX_BUFFER_VIEW ibv{};
    ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R16_UINT;
    ibv.SizeInBytes = sizeof(uint16_t) * m_indexCount;

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vbv);
    cmdList->IASetVertexBuffers(1, 1, &instanceView);
    cmdList->IASetIndexBuffer(&ibv);

    // 描画コマンドを発行
    cmdList->DrawIndexedInstanced(static_cast<UINT>(m_indexCount), m_instanceCount, 0, 0, 0);
}

void ParticleSystem::Release() {
    m_instanceBuffer.Reset();
    m_vertexBuffer.Reset();
    m_vertexBufferUpload.Reset();
    m_indexBuffer.Reset();
    m_indexBufferUpload.Reset();
    m_instances.clear();
    m_instanceCount = 0;
    m_maxParticles = 0;
    m_time = 0.0f;
    m_indexCount = 0;
    m_srvHeap.Reset();
}
