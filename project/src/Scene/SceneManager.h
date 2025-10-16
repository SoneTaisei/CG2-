#pragma once
#include <memory>
#include "IScene.h"
// 各シーンのヘッダをインクルード
#include "TitleScene.h"
#include "StageSelectScene.h"
#include "GameScene.h"


class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
    void Update();
    void Draw(const Matrix4x4 &viewProjectionMatrix);

    void ChangeScene(IScene *newScene);

private:
    std::unique_ptr<IScene> currentScene_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
};
