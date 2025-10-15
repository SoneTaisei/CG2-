#include "TitleScene.h"
#include "SceneManager.h" // Updateで使うのでインクルード
#include "Input/KeyboardInput.h" // キー入力取得のため
#include "../externals/imgui/imgui.h"

void TitleScene::Initialize() {}

void TitleScene::Update(SceneManager *sceneManager) {
    ImGui::Begin("Scene Info");
    ImGui::Text("This is Title Scene.");
    ImGui::End();

    // スペースキーが押されたらステージセレクトシーンへ
    if(KeyboardInput::GetInstance()->IsKeyPressed(DIK_SPACE)) {
        sceneManager->ChangeScene(new StageSelectScene());
    }
}

void TitleScene::Draw() {
    // ここにタイトルシーンの描画処理を記述する
    // 例：スプライト描画など
}