#include "GameScene.h"
#include "SceneManager.h"
#include "Input/KeyboardInput.h"
#include "../externals/imgui/imgui.h"

void GameScene::Initialize() {}

void GameScene::Update(SceneManager *sceneManager) {
    ImGui::Begin("Scene Info");
    ImGui::Text("This is Game Scene.");
    ImGui::End();

    // スペースキーが押されたらタイトルシーンへ戻る
    if(KeyboardInput::GetInstance()->IsKeyPressed(DIK_SPACE)) {
        sceneManager->ChangeScene(new TitleScene());
    }
}

void GameScene::Draw() {
    // ここにゲームシーンの描画処理を記述する
}