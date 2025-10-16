#include "SceneManager.h"
#include <cassert>

SceneManager::SceneManager() {}

SceneManager::~SceneManager() {}

void SceneManager::Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
    // 最初のシーンをタイトルに設定
    currentScene_ = std::make_unique<TitleScene>();
    currentScene_->Initialize(commandList);
    commandList_ = commandList;
}

void SceneManager::Update() {
    if(currentScene_) {
        currentScene_->Update(this); // thisポインタを渡す
    }
}

void SceneManager::Draw(const Matrix4x4 &viewProjectionMatrix) {
    if(currentScene_) {
        currentScene_->Draw(viewProjectionMatrix);
    }
}

void SceneManager::ChangeScene(IScene *newScene) {
    assert(newScene); // 渡されたシーンがnullptrでないことを確認

    // 現在のシーンを破棄し、新しいシーンを設定
    currentScene_.reset(newScene);
    currentScene_->Initialize(commandList_);
}
