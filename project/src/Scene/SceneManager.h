#pragma once
#include <memory>
#include "IScene.h"
// 各シーンのヘッダをインクルード
#include "TitleScene.h"
#include "StageSelectScene.h"
#include "GameScene.h"

class SpriteCommon;

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
    void Update();
    void Draw(const Matrix4x4 &viewProjectionMatrix);

    void ChangeScene(IScene *newScene);

    // SpriteCommonをセットする関数
    void SetSpriteCommon(SpriteCommon *spriteCommon) { spriteCommon_ = spriteCommon; }

    // SpriteCommonを取得する関数
    SpriteCommon *GetSpriteCommon() const { return spriteCommon_; }

private:
    std::unique_ptr<IScene> currentScene_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    SpriteCommon *spriteCommon_ = nullptr;
};
