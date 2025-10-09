#pragma once

// C++標準ライブラリ
#include <cstdint>
#include <vector>

// Windows & DirectX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h> 

// DirectX Shader Compiler
#include <dxcapi.h>

// XAudio2
#include <xaudio2.h>

// 自分のプロジェクトのヘッダ
#include "Graphics/D3DResourceLeakChacker.h"
#include "Utility/UtilityFunctions.h" 
#include "Graphics/DebugCamera.h" // DebugCameraクラスの定義
//#include "Audio/SoundManager.h"   // SoundData構造体の定義
#include "Input/KeyboardInput.h"  // Inputクラスの定義
#include "Input/GamepadInput.h"   // Inputクラスの定義

class MyGameApp {
public:
	// public: 外部から呼び出すためのインターフェース
	MyGameApp(UINT width, UINT height);
	~MyGameApp();
	void Run();

private:
	// private: クラス内部でのみ使用する実装詳細
	void Initialize();
	void MainLoop();
	void Finalize();

	void Update();
	void Render();

	void InitializeWindow();
	void InitializeDirectX();
	void InitializeImGui();
	void LoadAssets();
	void WaitForGpu();

	// ====================================================================================
	// メンバ変数
	// C++の仕様により、このセクションに宣言された順序の「逆順」で破棄（デストラクタが呼ばれる）
	// されます。この性質を利用して、リソースの解放順序を自動的に制御します。
	// ====================================================================================

private:
	UINT width_;
	UINT height_;
	HWND hwnd_ = nullptr;
	D3DResourceLeakChacker leakChacker_;

	// --- DirectX コアオブジェクト (解放順序が最後になるもの) ---
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
#endif

	// --- コマンド関連 ---
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

	// --- スワップチェーン関連 ---
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2];
	UINT backBufferIndex_ = 0;

	// --- デスクリプタヒープ ---
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiSrvDescriptorHeap_;

	// --- パイプラインステート & ルートシグネチャ ---
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	// --- シェーダコンパイラ関連 ---
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

	// --- 各種リソース (ComPtrの実体) ---
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSphere_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSphere_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSphere_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite_;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite_;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModel_;
	Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionResource_;

	// --- 同期オブジェクト ---
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// --- オーディオ ---
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	SoundData soundData1_{};

	// --- ゲームの状態やオブジェクト ---
	DebugCamera debugCamera_;
	bool isSphere_ = true;
	bool isModel_ = true;
	bool isSprite_ = true;
	bool useMonsterBall_ = true;
	bool isSound_ = false;
	bool preIsSound_ = false;

	Transform cameraTransform_ = {{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f}};
	Transform transformSphere_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform transformModel_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform transformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {640.0f, 360.0f, 0.0f} };
	Transform uvTransformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	DirectionalLight directionalLightData_ = { {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, 1.0f };

	// --- マップ済みポインタ (定数バッファへの書き込み用) ---
	Material *materialData_ = nullptr;
	Material *materialDataSprite_ = nullptr;
	DirectionalLight *mappedDirectionalLightData_ = nullptr;
	TransformMatrix *mappedTransformationMatrixSphere_ = nullptr;
	TransformMatrix *transformationMatrixDataModel_ = nullptr;
	Matrix4x4 *transformationMatrixDataSprite_ = nullptr;
	ViewProjection *viewProjectionData_ = nullptr;

	// --- 描画設定など ---
	D3D12_VIEWPORT viewport_ = {};
	D3D12_RECT scissorRect_ = {};
	D3D12_RESOURCE_BARRIER barrier_ = {};
};

