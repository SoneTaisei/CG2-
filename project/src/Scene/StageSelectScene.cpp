#include "StageSelectScene.h"
#include "SceneManager.h"
#include "Input/KeyboardInput.h"
#include "../externals/imgui/imgui.h"

void StageSelectScene::Initialize() {}

void StageSelectScene::Update(SceneManager *sceneManager) {
    ImGui::Begin("Scene Info");
    ImGui::Text("This is StageSelect Scene.");
    ImGui::End();

    // スペースキーが押されたらゲームシーンへ
    if(KeyboardInput::GetInstance()->IsKeyPressed(DIK_SPACE)) {
        sceneManager->ChangeScene(new GameScene());
    }
}

void StageSelectScene::Draw() {
    // ここにステージセレクトシーンの描画処理を記述する
}
