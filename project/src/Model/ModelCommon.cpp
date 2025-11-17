#include "ModelCommon.h"
#include "Model.h"
#include <cassert>
#include "Graphics/TextureManager.h"

void ModelCommon::Initialize(ID3D12Device *device) {
    assert(device);
    device_ = device;
}

void ModelCommon::PreDraw(ID3D12GraphicsCommandList *commandList) {
    assert(commandList);
    commandList_ = commandList;

    // ★重要修正: テクスチャを使用するための「場所（ヒープ）」をGPUに教える
    // これがないと SetGraphicsRootDescriptorTable で必ずクラッシュします
    ID3D12DescriptorHeap *descriptorHeaps[] = { TextureManager::GetInstance()->GetSrvDescriptorHeap() };
    commandList_->SetDescriptorHeaps(1, descriptorHeaps);

    // ★推奨: 描画形状（三角形リスト）を指定しておく
    // これを忘れると、前の描画設定（線など）が残っていた場合に表示がおかしくなります
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ModelCommon::AddModel(Model *model) {
    models_.push_back(model);
}

void ModelCommon::RemoveModel(Model *model) {
    models_.remove(model);
}

void ModelCommon::DrawAll(const Matrix4x4 &viewProjectionMatrix) {
    for(Model *model : models_) {
        // モデル自身のDrawを呼ぶ（引数にはVP行列だけ渡す）
        model->Draw(viewProjectionMatrix);
    }
}
