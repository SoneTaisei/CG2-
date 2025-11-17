#include "TitleScene.h"
#include "SceneManager.h" // Updateで使うのでインクルード
#include "Input/KeyboardInput.h" // キー入力取得のため
#include "../externals/imgui/imgui.h"
#include "Sprite/Sprite.h"
#include "Sprite/SpriteCommon.h"
#include "Graphics/TextureManager.h"
#include "Core/TimeManager.h"
#include "StageSelectScene.h"

TitleScene::~TitleScene() {
}

void TitleScene::Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
	// モデルを作成
	commandList_ = commandList;
	model_.reset(Model::CreateFromObj("resources/plane", "plane.obj"));

	ModelData planeDataForTexture = model_->GetModelData();
	textureHandle_ = TextureManager::GetInstance()->Load(planeDataForTexture.material.textureFilePath, commandList.Get());

	// テクスチャをロード
	//textureHandle_ = TextureManager::GetInstance()->Load("resources/uvChecker.png",commandList_.Get());

	// 2. プレイヤー用スプライトを作る
	std::unique_ptr<Sprite> playerSprite = std::make_unique<Sprite>();
	// ★ プレイヤーの画像番号を渡す
	playerSprite->Initialize(spriteCommon_, textureHandle_);
	playerSprite->SetPosition({ 100.0f, 100.0f }); // 左の方
	playerSprite->SetSize({ 100.0f, 100.0f });
	sprites_.push_back(std::move(playerSprite));


	// 3. 敵用スプライトを作る
	std::unique_ptr<Sprite> enemySprite = std::make_unique<Sprite>();
	// ★ 敵の画像番号を渡す
	enemySprite->Initialize(spriteCommon_, textureHandle_);
	enemySprite->SetPosition({ 500.0f, 100.0f }); // 右の方
	enemySprite->SetSize({ 500.0f, 100.0f });
	sprites_.push_back(std::move(enemySprite));
}

void TitleScene::Update(SceneManager *sceneManager) {

	// デルタタイムを取得
	//float dt = TimeManager::GetInstance().GetDeltaTime();

	//transform_.translate.x = ++transform_.translate.x * dt;


	// スペースキーが押されたらステージセレクトシーンへ
	if(KeyboardInput::GetInstance()->IsKeyPressed(DIK_SPACE)) {
		sceneManager->ChangeScene(new StageSelectScene());
	}
}

void TitleScene::Draw(const Matrix4x4 &viewProjectionMatrix) {
	Model::PreDraw(commandList_.Get());

	// まず、TextureManagerを使って整理番号(uvCheckerHandle)からGPUハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE planeGpuHandle = TextureManager::GetInstance()->GetGpuHandle(textureHandle_);
	// そのまま呼べる
	model_->Draw(transform_, viewProjectionMatrix, planeGpuHandle);

	if(spriteCommon_) {
		spriteCommon_->PreDraw(commandList_.Get());
		spriteCommon_->DrawAll();
	}

}