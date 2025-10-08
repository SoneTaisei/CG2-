#include "Model.h"
#include <cassert>

// 静的メンバ変数の実体定義
ID3D12Device *Model::sDevice_ = nullptr;
ID3D12GraphicsCommandList *Model::sCommandList_ = nullptr;

// 注意：以下の関数は、元々main.cppから呼び出されていたもので、
// 実際の定義は "Utility/Utilityfunctions.cpp" などにあると想定しています。
// このファイルがコンパイルエラーになる場合は、関数の定義元をインクルードするか、
// このファイルに実装を移動してください。
extern ModelData LoadObjFile(const std::string &directoryPath, const std::string &filename);
extern void CreateSphereMesh(std::vector<VertexData> &vertices, std::vector<uint32_t> &indices, float radius, uint32_t latDiv, uint32_t lonDiv);


void Model::StaticInitialize(ID3D12Device *device) {
    assert(device);
    sDevice_ = device;
}

void Model::PreDraw(ID3D12GraphicsCommandList *commandList) {
    assert(commandList);
    sCommandList_ = commandList;
}

Model *Model::CreateFromObj(const std::string &directoryPath, const std::string &filename) {
    Model *model = new Model();
    // モデルデータを外部関数でロード
    model->modelData_ = LoadObjFile(directoryPath, filename);
    // バッファを作成
    model->CreateBuffers();
    return model;
}

Model *Model::CreateSphere() {
    Model *model = new Model();
    // 球のメッシュを外部関数で生成
    CreateSphereMesh(model->modelData_.vertices, model->modelData_.indices, 1.0f, 32, 32);
    // バッファを作成
    model->CreateBuffers();
    return model;
}

void Model::CreateBuffers() {
    HRESULT hr;

    // Vertex Buffer
    vertexResource_ = CreateBufferResource(sDevice_, sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData *vertexData = nullptr;
    hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
    assert(SUCCEEDED(hr));
    std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
    vertexResource_->Unmap(0, nullptr);

    // Index Buffer
    indexResource_ = CreateBufferResource(sDevice_, sizeof(uint32_t) * modelData_.indices.size());
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * modelData_.indices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t *indexData = nullptr;
    hr = indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    assert(SUCCEEDED(hr));
    std::memcpy(indexData, modelData_.indices.data(), sizeof(uint32_t) * modelData_.indices.size());
    indexResource_->Unmap(0, nullptr);
}

void Model::Draw(const Transform &transform, const Matrix4x4 &viewProjectionMatrix, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
    assert(sCommandList_);

    // ワールド変換行列を計算
    Matrix4x4 worldMatrix = TransformFunctions::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    // シェーダーに渡すための変換行列を作成
    TransformMatrix transformMatrix;
    transformMatrix.WVP = TransformFunctions::Multiply(worldMatrix, viewProjectionMatrix);
    transformMatrix.World = worldMatrix;

    // ルートパラメータ1番に定数としてWVP行列などを設定
    sCommandList_->SetGraphicsRoot32BitConstants(1, sizeof(TransformMatrix) / 4, &transformMatrix, 0);

    // ルートパラメータ2番にテクスチャを設定
    sCommandList_->SetGraphicsRootDescriptorTable(2, textureHandle);

    // 頂点バッファとインデックスバッファを設定
    sCommandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
    sCommandList_->IASetIndexBuffer(&indexBufferView_);

    // 描画コマンド発行
    sCommandList_->DrawIndexedInstanced(UINT(modelData_.indices.size()), 1, 0, 0, 0);
}