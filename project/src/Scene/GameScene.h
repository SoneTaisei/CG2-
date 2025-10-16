#pragma once
#include "IScene.h"
#include <d3d12.h>

class GameScene : public IScene {
public:
    void Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) override;
    void Update(SceneManager *sceneManager) override;
    void Draw(const Matrix4x4 &viewProjectionMatrix) override;
};