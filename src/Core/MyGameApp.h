#pragma once
#include "Utility/UtilityFunctions.h"
class MyGameApp {
public:
	MyGameApp(UINT width, UINT height);
	~MyGameApp();
	void Run();

private:
	void Initialize();
	void MainLoop();
	void Finalize();

	void Update();
	void Render();

	void InitializeDirectX();
	void InitializeWindow();
	void InitializeImGui();
	void LoadAssets();

	// Transform変数を作る
	Transform transform = {
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	Transform cameraTransform = {
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,-5.0f}
	};

	Transform transformSprite = {
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	Transform transformSphere = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	bool useMonsterBall = true;

	// ウィンドウサイズ
	UINT width_;
	UINT height_;

	HWND hwnd_ = nullptr; // ← メンバ変数として保持

	// デバイスとファクトリ
	IDXGIFactory7 *dxgiFactory = nullptr;
	IDXGIAdapter4 *useAdapter = nullptr;
	ID3D12Device *device = nullptr;

	// デバッグ
#ifdef _DEBUG
	ID3D12Debug1 *debugController = nullptr;
	ID3D12InfoQueue *infoQueue = nullptr;
#endif

	// スワップチェーンバッファ
	IDXGISwapChain4 *swapChain = nullptr;
	ID3D12Resource *swapChainResources[2] = { nullptr };
	UINT backBufferIndex = 0;

	// コマンド系
	ID3D12CommandQueue *commandQueue = nullptr;
	ID3D12CommandAllocator *commandAllocator = nullptr;
	ID3D12GraphicsCommandList *commandList = nullptr;

	// デスクリプタヒープ
	ID3D12DescriptorHeap *rtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap *srvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap *imguiSrvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap *dsvDescriptorHeap = nullptr;

	// RTV・DSVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	// フェンス関連
	ID3D12Fence *fence = nullptr;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	// パイプライン・ルートシグネチャ
	ID3D12RootSignature *rootSignature = nullptr;
	ID3D12PipelineState *graphicsPipelineState = nullptr;

	// シェーダ
	IDxcUtils *dxcUtils = nullptr;
	IDxcCompiler3 *dxcCompiler = nullptr;
	IDxcIncludeHandler *includeHandler = nullptr;
	IDxcBlob *vertexShaderBlob = nullptr;
	IDxcBlob *pixelShaderBlob = nullptr;
	ID3DBlob *signatureBlob = nullptr;
	ID3DBlob *errorBlob = nullptr;

	// DepthStencil
	ID3D12Resource *depthStencilResource = nullptr;

	// ビューポート・シザー
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};

	// マテリアルにデータを書き込む
	Material *materialData = nullptr;

	DirectionalLight directionalLightData = {};

	ID3D12Device *device_ = nullptr;

	// バリア
	D3D12_RESOURCE_BARRIER barrier{};

	// データを書き込む
	Matrix4x4 *transformationMatrixDataSprite = nullptr;

	// データを書き込むためのポインタ
	TransformMatrix *transformationMatrixDataSphere = nullptr;

	ID3D12DescriptorHeap *descriptorHeaps_[1];

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	// マテリアルリソース
	ID3D12Resource *materialResource = {};

	// GPUリソース
	ID3D12Resource *directionalLightResource = {};

};

