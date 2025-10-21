#include "Sprite.h"

// 静的メンバ変数の実体定義
ID3D12Device *Sprite::sDevice_ = nullptr;
ID3D12GraphicsCommandList *Sprite::sCommandList_ = nullptr;
Microsoft::WRL::ComPtr<ID3D12Resource> Sprite::sVertexResource_ = nullptr;
D3D12_VERTEX_BUFFER_VIEW Sprite::sVertexBufferView_ = {};
Microsoft::WRL::ComPtr<ID3D12Resource> Sprite::sIndexResource_ = nullptr;
D3D12_INDEX_BUFFER_VIEW Sprite::sIndexBufferView_ = {};
Microsoft::WRL::ComPtr<ID3D12Resource> Sprite::sMaterialResource_ = nullptr;
Microsoft::WRL::ComPtr<ID3D12Resource> Sprite::sTransformationMatrixResource_ = nullptr;
Material *Sprite::sMaterialData_ = nullptr;
Matrix4x4 *Sprite::sTransformationMatrixData_ = nullptr;
Matrix4x4 Sprite::sProjectionMatrix_ = {};
std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> Sprite::sTextureSrvHandles_ = {};

int Sprite::sWindowWidth_ = 0;
int Sprite::sWindowHeight_ = 0;

void Sprite::StaticInitialize(
    ID3D12Device *device,
    int windowWidth, int windowHeight,
    const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> &textureSrvHandles) {

    sDevice_ = device;
    sTextureSrvHandles_ = textureSrvHandles;

    // --- 頂点・インデックスリソース作成 (全スプライトで共通) ---
    sVertexResource_ = CreateBufferResource(sDevice_, sizeof(VertexData) * kVertexCount);
    sVertexBufferView_.BufferLocation = sVertexResource_->GetGPUVirtualAddress();
    sVertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
    sVertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData *vertexData = nullptr;
    sVertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
    // 単位矩形(0,0)~(1,1)で作成。サイズや位置は行列で指定する。
    vertexData[0] = { {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 左下
    vertexData[1] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 左上
    vertexData[2] = { {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 右下
    vertexData[3] = { {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 右上

    sIndexResource_ = CreateBufferResource(sDevice_, sizeof(uint32_t) * kIndexCount);
    sIndexBufferView_.BufferLocation = sIndexResource_->GetGPUVirtualAddress();
    sIndexBufferView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
    sIndexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    uint32_t *indexData = nullptr;
    sIndexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
    indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;

    // --- 定数バッファ作成 ---
    sMaterialResource_ = CreateBufferResource(sDevice_, sizeof(Material));
    sMaterialResource_->Map(0, nullptr, reinterpret_cast<void **>(&sMaterialData_));

    // --- 射影行列の計算 ---
    sProjectionMatrix_ = TransformFunctions::MakeOrthographicMatrix(0.0f, 0.0f, float(windowWidth), float(windowHeight), 0.0f, 100.0f);

    sWindowWidth_ = windowWidth;
    sWindowHeight_ = windowHeight;
}

void Sprite::StaticFinalize() {
    // ComPtrが自動で解放してくれますが、明示的にリセットも可能です
    sVertexResource_.Reset();
    sIndexResource_.Reset();
    sMaterialResource_.Reset();
    sTransformationMatrixResource_.Reset();
}

void Sprite::PreDraw(ID3D12GraphicsCommandList *commandList) {
    sCommandList_ = commandList;
    // 頂点・インデックスバッファをセット (描画の最初に1回でOK)
    sCommandList_->IASetVertexBuffers(0, 1, &sVertexBufferView_);
    sCommandList_->IASetIndexBuffer(&sIndexBufferView_);
}

void Sprite::Draw(
    float destX, float destY, float width, float height,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, float scaleX, float scaleY,
    float angle, const Vector4 &color) {

    // スプライトの実際の幅と高さを計算
    float scaledWidth = width * scaleX;
    float scaledHeight = height * scaleY;

    // 画面外判定 (カリング)
    if(destX + scaledWidth < 0) { return; }  // 完全に左画面外なら処理を中断
    if(destX > sWindowWidth_) { return; }  // 完全に右画面外なら処理を中断
    if(destY + scaledHeight < 0) { return; }  // 完全に上画面外なら処理を中断
    if(destY > sWindowHeight_) { return; }  // 完全に下画面外なら処理を中断

    // --- マテリアルに行列を設定 ---
    sMaterialData_->color = color;
    sMaterialData_->lightingType = false;
    sMaterialData_->uvTransform = TransformFunctions::MakeIdentity4x4(); // UVアニメは別途実装

    // --- 座標変換行列を計算 ---
    Matrix4x4 worldMatrix = TransformFunctions::MakeAffineMatrix(
        { width * scaleX, height * scaleY, 1.0f }, // Scale
        { 0.0f, 0.0f, angle },                     // Rotate
        { destX, destY, 0.0f }                     // Translate
    );
    Matrix4x4 viewMatrix = TransformFunctions::MakeIdentity4x4();
    // sTransformationMatrixData_ は Matrix4x4* なので、TransformMatrix* にキャストして使う
    // GPUに直接送るための TransformMatrix 構造体を作成
    TransformMatrix transformMatrix;
    transformMatrix.WVP = TransformFunctions::Multiply(worldMatrix, TransformFunctions::Multiply(viewMatrix, sProjectionMatrix_));
    transformMatrix.World = worldMatrix;

    // --- コマンドを積む ---
    sCommandList_->SetGraphicsRootConstantBufferView(0, sMaterialResource_->GetGPUVirtualAddress());

    // ルートパラメータの1番に、transformMatrix のデータを直接セットする
    sCommandList_->SetGraphicsRoot32BitConstants(1, sizeof(TransformMatrix) / 4, &transformMatrix, 0);

    // 古い SetGraphicsRootConstantBufferView(1, ...) は不要
    sCommandList_->SetGraphicsRootDescriptorTable(2, textureHandle);

    // 描画
    sCommandList_->DrawIndexedInstanced(kIndexCount, 1, 0, 0, 0);
}
