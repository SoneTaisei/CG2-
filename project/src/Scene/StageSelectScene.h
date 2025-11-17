#pragma once
#include "IScene.h"
#include <d3d12.h>

class StageSelectScene : public IScene {
public:
    ~StageSelectScene()override;
    void Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)override;
    void Update(SceneManager *sceneManager) override;
    void Draw(const Matrix4x4 &viewProjectionMatrix) override;
};
