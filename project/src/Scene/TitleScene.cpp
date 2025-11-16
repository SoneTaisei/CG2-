#include "TitleScene.h"
#include "SceneManager.h" // Updateで使うのでインクルード
#include "Input/KeyboardInput.h" // キー入力取得のため
#include "../externals/imgui/imgui.h"
#include"Sprite/Sprite.h"
#include "Graphics/TextureManager.h"
#include "Core/TimeManager.h"

void TitleScene::Initialize(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
	// モデルを作成
	commandList_ = commandList;
	model_ = Model::CreateFromObj("resources/plane", "plane.obj");

	ModelData planeDataForTexture = model_->GetModelData();
	textureHandle_ = TextureManager::GetInstance()->Load(planeDataForTexture.material.textureFilePath, commandList.Get());

	// テクスチャをロード
	//textureHandle_ = TextureManager::GetInstance()->Load("resources/uvChecker.png",commandList_.Get());

	// 座標を初期化
	transform_ = {
		{1.0f, 1.0f, 1.0f}, // Scale
		{0.0f, 0.0f, 0.0f}, // Rotate
		{0.0f, 0.0f, 0.0f}  // Translate
	};
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
	//model_->CreateSphere();
	// 取得したGPUハンドルをDraw関数に渡す
	model_->Draw(transform_,viewProjectionMatrix, planeGpuHandle);
	//model_->Draw({
	//	{1.0f, 1.0f, 0.5f}, // Scale
	//	{0.0f, 0.0f, 0.0f}, // Rotate
	//	{1.0f, 0.0f, 0.0f}  // Translate
	//			 },viewProjectionMatrix, planeGpuHandle);

	//Sprite::PreDraw(commandList_.Get());
	//Sprite::Draw(150, 50, 100, 100, planeGpuHandle);
	//Sprite::Draw(150, 80, 100, 100, planeGpuHandle);

}