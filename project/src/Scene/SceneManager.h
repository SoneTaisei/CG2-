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

    void Initialize();
    void Update();
    void Draw();

    void ChangeScene(IScene *newScene);

private:
    std::unique_ptr<IScene> currentScene_ = nullptr;
};
