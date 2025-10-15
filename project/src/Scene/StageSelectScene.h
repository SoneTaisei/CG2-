#pragma once
#include "IScene.h"

class StageSelectScene : public IScene {
public:
    void Initialize() override;
    void Update(SceneManager *sceneManager) override;
    void Draw() override;
};
