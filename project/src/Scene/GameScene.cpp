#include "GameScene.h"
#include "SceneManager.h"
#include "Input/KeyboardInput.h"
#include "../externals/imgui/imgui.h"

void GameScene::Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
}

void GameScene::Update(SceneManager *sceneManager) {

    // スペースキーが押されたらタイトルシーンへ戻る
    if(KeyboardInput::GetInstance()->IsKeyPressed(DIK_SPACE)) {
        sceneManager->ChangeScene(new TitleScene());
    }
}

void GameScene::Draw(const Matrix4x4 &viewProjectionMatrix) {
    // ここにゲームシーンの描画処理を記述する
}