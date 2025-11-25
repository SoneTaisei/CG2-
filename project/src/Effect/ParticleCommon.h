#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include "Utility/Utilityfunctions.h"

// 共通の頂点データ構造体
struct ParticleVertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

class ParticleCommon {
public:
    void Initialize(ID3D12Device *device);
    void PreDraw(ID3D12GraphicsCommandList *commandList);

    // ゲッター
    ID3D12Device *GetDevice() const { return device_; }
    ID3D12GraphicsCommandList *GetCommandList() const { return commandList_; }
    const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const { return vertexBufferView_; }
    const Microsoft::WRL::ComPtr<ID3D12RootSignature> &GetRootSignature() const { return rootSignature_; }
    const Microsoft::WRL::ComPtr<ID3D12PipelineState> &GetPipelineState() const { return pipelineState_; }
    UINT GetVertexCount() const { return static_cast<UINT>(vertices_.size()); }

private:
    void CreateRootSignature();
    void CreatePipelineState();
    void CreateMesh(); // 共通の板ポリゴン生成

private:
    ID3D12Device *device_ = nullptr;
    ID3D12GraphicsCommandList *commandList_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    // 共通の板ポリゴンデータ
    std::vector<ParticleVertexData> vertices_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
};