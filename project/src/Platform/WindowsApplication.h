#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include "Scene/SceneManager.h"
#include <memory>
#include "Graphics/DebugCamera.h" 
#include "Utility/Utilityfunctions.h"
#include "DirectXCommon/DirectXCommon.h"
#include "DirectXCommon/D3DResourceLeakChecker.h"
#include "Sprite/SpriteCommon.h"

class WindowsApplication {
public:
	// ウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// 定数
	static const int kWindowWidth_ = 1280;
	static const int kWindowHeight_ = 720;

public:
	void Initialize();
	void Run();
	void Finalize();

private:
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};

	// DirectX関連の処理をまとめたクラス
	std::unique_ptr<DirectXCommon> dxCommon_;

	// スプライト共通部のメンバ変数
	std::unique_ptr<SpriteCommon> spriteCommon_;

	// --- DirectX関連以外のメンバ変数 ---
	std::unique_ptr<SceneManager> sceneManager_;

	std::unique_ptr<DebugCamera> debugCamera_;
	Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionResource_;
	ViewProjection *viewProjectionData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
	DirectionalLight *directionalLightData_ = nullptr;

	const UINT materialBufferSize = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = {};
	Material *materialData = nullptr;

#ifdef _DEBUG
// リソースリークチェッカー
	std::unique_ptr<D3DResourceLeakChecker> leakChecker_;
#endif
};

