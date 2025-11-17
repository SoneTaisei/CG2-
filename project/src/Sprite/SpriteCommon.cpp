#include "SpriteCommon.h"
#include "Sprite.h" // Spriteの定義が必要

void SpriteCommon::Initialize(ID3D12Device *device, int windowWidth, int windowHeight) {
    device_ = device;

    // 射影行列の計算 (平行投影)
    projectionMatrix_ = TransformFunctions::MakeOrthographicMatrix(
        0.0f, 0.0f, float(windowWidth), float(windowHeight), 0.0f, 100.0f
    );

    CreateCommonResources();
}

void SpriteCommon::Finalize() {
    sprites_.clear();
    // ComPtrなのでリソースは自動解放されます
}

void SpriteCommon::CreateCommonResources() {
    // --- 頂点リソース作成 (元のStaticInitializeの内容) ---
    // 単位矩形(0,0)~(1,1)で作成
    const int kVertexCount = 4;
    vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * kVertexCount);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData *vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
    vertexData[0] = { {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 左下
    vertexData[1] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 左上
    vertexData[2] = { {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 右下
    vertexData[3] = { {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 右上
    vertexResource_->Unmap(0, nullptr);

    // --- インデックスリソース作成 ---
    const int kIndexCount = 6;
    indexResource_ = CreateBufferResource(device_, sizeof(uint32_t) * kIndexCount);
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t *indexData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
    indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;
    indexResource_->Unmap(0, nullptr);
}

void SpriteCommon::PreDraw(ID3D12GraphicsCommandList *commandList) {
    commandList_ = commandList;

    // 共通のバッファをセット
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList_->IASetIndexBuffer(&indexBufferView_);
    // トポロジ設定なども必要であればここで行う
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteCommon::AddSprite(Sprite *sprite) {
    sprites_.push_back(sprite);
}

void SpriteCommon::RemoveSprite(Sprite *sprite) {
    sprites_.remove(sprite);
}

void SpriteCommon::DrawAll() {
    // リストに登録されている全スプライトを描画
    for(Sprite *sprite : sprites_) {
        sprite->Draw();
    }
}
