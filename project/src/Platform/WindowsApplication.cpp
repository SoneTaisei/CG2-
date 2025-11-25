#include "Platform/WindowsApplication.h"
#include "Utility/Utilityfunctions.h"
#include "Input/KeyboardInput.h"
#include "Input/GamepadInput.h"
#include "Graphics/TextureManager.h"
#include "Audio/AudioManager.h"
#include "Model/Model.h"
#include "Sprite/Sprite.h"
#include "Graphics/DebugCamera.h"
#include <cassert>
#include <format>
#include <chrono>
#include "Utility/TransformFunctions.h"
//#include "Core/TimeManager.h"

// ImGuiの外部リンケージ
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "winmm.lib")


LRESULT CALLBACK WindowsApplication::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// ImGuiへのメッセージ転送
	if(ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	// メッセージに応じてゲーム固有の処理を行う
	switch(msg) {
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WindowsApplication::Initialize() {



	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	/*********************************************************
	*WindowsAPIの初期化
	*********************************************************/
	wc_.lpfnWndProc = WindowProc;
	wc_.lpszClassName = L"MyDreamGame";
	wc_.hInstance = GetModuleHandle(nullptr);
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc_);

	RECT wrc = { 0, 0, kWindowWidth_, kWindowHeight_ };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd_ = CreateWindow(
		wc_.lpszClassName,
		L"MyDreamGame",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc_.hInstance,
		nullptr);

	ShowWindow(hwnd_, SW_SHOW);


	/*********************************************************
	*DirectX初期化処理
	*********************************************************/
	// DirectXCommonクラスのインスタンスを作成し、初期化
	dxCommon_ = std::make_unique<DirectXCommon>();
	dxCommon_->Initialize(hwnd_, kWindowWidth_, kWindowHeight_);

	// dxCommon_から必要なポインタを取得
	ID3D12Device *device = dxCommon_->GetDevice();
	ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList();

	// キーボードとコントローラーの初期化
	KeyboardInput::GetInstance()->Initialize(wc_.hInstance, hwnd_);
	GamepadInput::GetInstance()->Initialize(wc_.hInstance, hwnd_);

	 // ★ 2. SceneManager の生成
	sceneManager_ = std::make_unique<SceneManager>();

	// SpriteクラスとModelクラスの静的初期化
	// ModelCommonの生成と初期化
	modelCommon_ = std::make_unique<ModelCommon>();
	modelCommon_->Initialize(device);

	// SceneManagerに渡す
	sceneManager_->SetModelCommon(modelCommon_.get());
	TextureManager::GetInstance()->Initialize(device);

	// ★ 1. SpriteCommon の生成と初期化
	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Initialize(dxCommon_->GetDevice(), kWindowWidth_, kWindowHeight_);

	// ★ 3. SpriteCommon を SceneManager に渡す
	sceneManager_->SetSpriteCommon(spriteCommon_.get());

	// SceneManager初期化
	sceneManager_->Initialize(commandList);

	//std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandles;
	//for(size_t i = 0; i < TextureManager::GetInstance()->GetTextureCount(); ++i) {
	//    textureHandles.push_back(TextureManager::GetInstance()->GetGpuHandle(static_cast<uint32_t>(i)));
	//}
	//Sprite::StaticInitialize(device, kWindowWidth_, kWindowHeight_, textureHandles);

	// ViewProjectionリソースの作成
	UINT viewProjectionSize = (sizeof(ViewProjection) + 255) & ~255;
	viewProjectionResource_ = CreateBufferResource(device, viewProjectionSize);
	viewProjectionResource_->Map(0, nullptr, reinterpret_cast<void **>(&viewProjectionData_));

	const UINT directionalLightBufferSize = (sizeof(DirectionalLight) + 255) & ~255u;
	directionalLightResource_ = CreateBufferResource(device, directionalLightBufferSize);
	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
	// 初期値を設定
	directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData_->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData_->intensity = 1.0f;

	// デバッグカメラの初期化
	debugCamera_ = std::make_unique<DebugCamera>();
	Matrix4x4 initViewMatrix = TransformFunctions::MakeIdentity4x4();
	float aspect = float(kWindowWidth_) / float(kWindowHeight_);
	Matrix4x4 initProjectionMatrix = TransformFunctions::MakePerspectiveFovMatrix(0.45f, aspect, 0.1f, 100.0f);
	debugCamera_->Initialize(initViewMatrix, initProjectionMatrix, kWindowWidth_, kWindowHeight_);

	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd_);
	ImGui_ImplDX12_Init(device, // device_.Get() から変更
						dxCommon_->GetSwapChainDesc().BufferCount, // swapChainDesc_.BufferCount から変更
						dxCommon_->GetRtvDesc().Format, // rtvDesc_.Format から変更
						TextureManager::GetInstance()->GetSrvDescriptorHeap(),
						TextureManager::GetInstance()->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
						TextureManager::GetInstance()->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart()
	);

	// 音声の初期化
	AudioManager::Initialize();

	const UINT materialBufferSize = (sizeof(Material) + 255) & ~255u;
	materialResource = CreateBufferResource(device, materialBufferSize);
	materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->lightingType = 0;
	materialData->uvTransform = TransformFunctions::MakeIdentity4x4();
	materialResource->Unmap(0, nullptr);

   // 1. 配列全体のサイズを計算 (行列のサイズ × 個数)
	UINT size = kParticleCount * sizeof(TransformMatrix);

	// 2. リソースを作成 (1つの大きなバッファ)
	instancingResource_ = CreateBufferResource(device, size);

	// 3. マップ (ずっと書き込み続けるので、Unmapせずにポインタを持っておく)
	instancingResource_->Map(0, nullptr, reinterpret_cast<void **>(&instancingData_));

	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kParticleCount;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformMatrix);

	// SRVを作成するディスクリプタヒープの場所を決める
	// ※今回はテクスチャなどで使っていない場所（例：kMaxTexturesに近い場所など）を使います
	// TextureManagerが管理しているHeapを使わせてもらいます
	ID3D12DescriptorHeap *srvDescriptorHeap = TextureManager::GetInstance()->GetSrvDescriptorHeap();
	UINT descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ヒープの先頭からハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	instancingSrvHandleGPU_ = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// 空いている場所までずらす（例：テクスチャが数枚あると仮定して、少し離れた場所に作る）
	// ※TextureManagerの実装によりますが、ここでは安全に「100番目」などを使ってみます。
	// 資料の「3」にするなら index = 3 ですが、他のテクスチャと被らないように注意してください。
	int index = 99; // 仮に99番目を使います
	instancingSrvHandleCPU.ptr += (descriptorSizeSRV * index);
	instancingSrvHandleGPU_.ptr += (descriptorSizeSRV * index);

	// SRVの生成
	device->CreateShaderResourceView(instancingResource_.Get(), &instancingSrvDesc, instancingSrvHandleCPU);

	// 4. 初期化 (単位行列を入れておく)
	for(int i = 0; i < kParticleCount; ++i) {
		instancingData_[i].World = TransformFunctions::MakeIdentity4x4();
		instancingData_[i].WVP = TransformFunctions::MakeIdentity4x4();
	}

	// --- 1つ目の三角形 (左上 -> 右上 -> 左下) ---
	modelData_.vertices.push_back({
		.position = {-1.0f, 1.0f, 0.0f, 1.0f}, // 左上
		.texcoord = {0.0f, 0.0f},              // 画像の左上
		.normal = {0.0f, 0.0f, -1.0f}
								  });
	modelData_.vertices.push_back({
		.position = {1.0f, 1.0f, 0.0f, 1.0f},  // 右上
		.texcoord = {1.0f, 0.0f},              // 画像の右上
		.normal = {0.0f, 0.0f, -1.0f}
								  });
	modelData_.vertices.push_back({
		.position = {-1.0f, -1.0f, 0.0f, 1.0f},// 左下
		.texcoord = {0.0f, 1.0f},              // 画像の左下
		.normal = {0.0f, 0.0f, -1.0f}
								  });

								  // --- 2つ目の三角形 (左下 -> 右上 -> 右下) ---
	modelData_.vertices.push_back({
		.position = {-1.0f, -1.0f, 0.0f, 1.0f},// 左下
		.texcoord = {0.0f, 1.0f},              // 画像の左下
		.normal = {0.0f, 0.0f, -1.0f}
								  });
	modelData_.vertices.push_back({
		.position = {1.0f, 1.0f, 0.0f, 1.0f},  // 右上
		.texcoord = {1.0f, 0.0f},              // 画像の右上
		.normal = {0.0f, 0.0f, -1.0f}
								  });
	modelData_.vertices.push_back({
		.position = {1.0f, -1.0f, 0.0f, 1.0f}, // 右下
		.texcoord = {1.0f, 1.0f},              // 画像の右下
		.normal = {0.0f, 0.0f, -1.0f}
								  });
	modelData_.material.textureFilePath = "./resources/uvChecker.png";

	// ★追加: TextureManagerに実際に画像を読み込ませる
	TextureManager::GetInstance()->Load(modelData_.material.textureFilePath, commandList);
	textureIndex_ = TextureManager::GetInstance()->Load(modelData_.material.textureFilePath, commandList);



	// 1. 頂点バッファのサイズを計算 (頂点数 × 1個分のサイズ)
	UINT sizeIB = static_cast<UINT>(sizeof(VertexData) * modelData_.vertices.size());

	// 2. リソース(メモリ)を作成
	vertexResource_ = CreateBufferResource(device, sizeIB);

	// 3. リソースにデータを書き込む
	VertexData *vertMap = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertMap));
	std::memcpy(vertMap, modelData_.vertices.data(), sizeIB); // 配列の中身をコピー
	vertexResource_->Unmap(0, nullptr);

	// 4. 頂点バッファビュー(VBV)を作成 (GPUに「ここに頂点があるよ」と伝える設定)
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeIB;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);

	// TimeManager を初期化
	//TimeManager::GetInstance().Initialize();

#ifdef _DEBUG
// リソースリークチェッカーのインスタンスを作成
	leakChecker_ = std::make_unique<D3DResourceLeakChecker>();
#endif

	// =========================================================================
// ▼▼▼ RootSignature の作成 (シェーダーの register に合わせる) ▼▼▼
// =========================================================================
	{
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// シェーダーの定義に合わせたパラメータ構成
		// RootParam[0]: Pixel Shader b0 -> Material
		// RootParam[1]: Vertex Shader b0 -> TransformationMatrix
		// RootParam[2]: Pixel Shader t3 -> Texture (DescriptorTable)
		// RootParam[3]: Pixel Shader b1 -> DirectionalLight
		D3D12_ROOT_PARAMETER rootParameters[4] = {};

		// --- [0] Material (Pixel Shader : register(b0)) ---
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.ShaderRegister = 0; // b0

		// Instancing用のDescriptorRangeを作成
		D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
		descriptorRangeForInstancing[0].BaseShaderRegister = 0; // t0
		descriptorRangeForInstancing[0].NumDescriptors = 1;
		descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // Tableを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

		// --- [2] Texture (Pixel Shader : register(t3)) ---
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 3; // t3
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		// --- [3] DirectionalLight (Pixel Shader : register(b1)) ---
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1; // b1

		descriptionRootSignature.pParameters = rootParameters;
		descriptionRootSignature.NumParameters = _countof(rootParameters);

		// サンプラーの設定 (Pixel Shader : register(s0))
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister = 0; // s0
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		// シリアライズして作成
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if(FAILED(hr)) {
			OutputDebugStringA((char *)errorBlob->GetBufferPointer());
			assert(false);
		}
		hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&particleRootSignature_));
		assert(SUCCEEDED(hr));
	}

	// =========================================================================
	// ▼▼▼ シェーダーコンパイルと PSO の作成 ▼▼▼
	// =========================================================================
	{
		// DXCの初期化
		IDxcUtils *dxcUtils = nullptr;
		IDxcCompiler3 *dxcCompiler = nullptr;
		IDxcIncludeHandler *includeHandler = nullptr;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		assert(SUCCEEDED(hr));
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
		assert(SUCCEEDED(hr));
		hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
		assert(SUCCEEDED(hr));

		// コンパイル用ローカル関数
		auto CompileShader = [&](const std::wstring &filePath, const wchar_t *profile) {
			OutputDebugStringW(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile).c_str());

			IDxcBlobEncoding *sourceBlob = nullptr;
			HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &sourceBlob);
			if(FAILED(hr)) {
				OutputDebugStringA("Failed to load shader file.\n");
				assert(false);
				return Microsoft::WRL::ComPtr<ID3DBlob>();
			}

			LPCWSTR arguments[] = {
				filePath.c_str(),
				L"-E", L"main",
				L"-T", profile,
				L"-Zi", L"-Qembed_debug",
				L"-Od",
				L"-Zpr",
			};

			DxcBuffer buffer = {};
			buffer.Ptr = sourceBlob->GetBufferPointer(); // バッファの先頭アドレス
			buffer.Size = sourceBlob->GetBufferSize();   // バッファサイズ
			buffer.Encoding = DXC_CP_UTF8;

			IDxcResult *result = nullptr;
			hr = dxcCompiler->Compile(
				&buffer,
				arguments,
				_countof(arguments),
				includeHandler,
				IID_PPV_ARGS(&result)
			);
			assert(SUCCEEDED(hr));

			IDxcBlobUtf8 *errorBlob = nullptr;
			result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), nullptr);
			if(errorBlob != nullptr && errorBlob->GetStringLength() > 0) {
				OutputDebugStringA((char *)errorBlob->GetStringPointer());
				assert(false);
			}

			Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob = nullptr;
			result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);

			result->Release();
			sourceBlob->Release();

			OutputDebugStringW(std::format(L"Compile Succeeded, path:{}\n", filePath).c_str());
			return shaderBlob;
			};

			// ★ファイル名を提示されたものに合わせて変更
		Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob = CompileShader(L"shaders/Particle.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob = CompileShader(L"shaders/Particle.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		// PSO設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
		graphicsPipelineStateDesc.pRootSignature = particleRootSignature_.Get();

		// InputLayout (Particle.hlsli の struct VertexShaderInput に合わせる)
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		graphicsPipelineStateDesc.InputLayout.pInputElementDescs = inputElementDescs;
		graphicsPipelineStateDesc.InputLayout.NumElements = _countof(inputElementDescs);

		graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
		graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

		// BlendState (通常ブレンド)
		graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		// RasterizerState
		graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		graphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // ビルボード等は裏面も描くことが多いのでNONE推奨

		graphicsPipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		// 書き込みフォーマットの設定 (環境に合わせて変更してください)
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		graphicsPipelineStateDesc.NumRenderTargets = 1;
		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		graphicsPipelineStateDesc.SampleDesc.Count = 1;

		// PSO作成
		hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&particlePipelineState_));
		assert(SUCCEEDED(hr));

		// 解放
		dxcUtils->Release();
		dxcCompiler->Release();
		includeHandler->Release();
	}
}

void WindowsApplication::Run() {
	// --- ここに main.cpp の while ループ前の初期化コードを移動 ---
	// (例：モデルのロード、スプライトの初期化など)

	// (このサンプルでは、main.cppの主要な部分を移植することに焦点を当てます)


	MSG msg{};
	// ウィンドウのxボタンが押されるまでループ
	while(msg.message != WM_QUIT) {
		// Windowにメッセージが来てたら最優先で処理させる
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {

			// デルタタイムを計算
			//TimeManager::GetInstance().Update();


			// --- 更新処理 (Update) ---
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			KeyboardInput::GetInstance()->Update();

			// カメラの更新
			debugCamera_->Update();
			// ViewProjectionを更新
			Matrix4x4 viewMatrix = debugCamera_->GetViewMatrix();
			Matrix4x4 projectionMatrix = debugCamera_->GetProjectionMatrix();
			viewProjectionData_->viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);
			viewProjectionData_->cameraPosition = debugCamera_->GetTranslation();

			sceneManager_->Update();

			// --- 描画処理 (Draw) ---
			dxCommon_->PreDraw();

			ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList();

			 // 定数バッファの設定 (これはゲーム固有の描画処理)
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(3, viewProjectionResource_->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource_->GetGPUVirtualAddress());

		   // 1. ルートシグネチャ（データの入力口の定義）をセット
			commandList->SetGraphicsRootSignature(particleRootSignature_.Get());

			// 2. パイプラインステート（シェーダー本体やブレンド設定）をセット
			commandList->SetPipelineState(particlePipelineState_.Get());

			// 3. トポロジー（形状の種類）をセット
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// 4. 頂点データとテクスチャのセット（今まで通り）
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

			// ディスクリプタヒープの設定
			ID3D12DescriptorHeap *descriptorHeaps[] = { TextureManager::GetInstance()->GetSrvDescriptorHeap() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			// RootParam[0]: Material (b0)
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

			// RootParam[2]: Texture (t3)
			// ※テクスチャは RootParam 2 に設定しました
			commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGpuHandle(textureIndex_));

			// RootParam[3]: DirectionalLight (b1)
			// ※ライトは RootParam 3 に設定しました
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());

			// Cameraの行列を取得
			viewMatrix = debugCamera_->GetViewMatrix();
			projectionMatrix = debugCamera_->GetProjectionMatrix();
			Matrix4x4 viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);

			// -------------------------------------------------
			// 1. データの更新 (CPUで計算してバッファに書き込む)
			// -------------------------------------------------
			for(int i = 0; i < kParticleCount; ++i) {
				// A. ワールド行列（場所）を決める
				Matrix4x4 worldMatrix = TransformFunctions::MakeTranslateMatrix({ 0.0f + 0.1f * i, 0.0f - 0.1f * i, 0.0f });

				// B. WVP行列（カメラ越しに見た状態）を作る
				Matrix4x4 wvpMatrix = TransformFunctions::Multiply(worldMatrix, viewProjectionMatrix);

				// C. 配列の i 番目にデータを書き込む
				instancingData_[i].World = worldMatrix;
				instancingData_[i].WVP = wvpMatrix;
			}

			// -------------------------------------------------
			// 2. 描画コマンド (ループせず一括で描画！)
			// -------------------------------------------------

			// RootParam[1] に SRV (StructuredBuffer) をセット
			// ★ SetGraphicsRootShaderResourceView を使います
			// 初期化で作ったハンドルをセットする
			commandList->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU_);

			// 描画実行 (第2引数にパーティクルの個数を指定)
			commandList->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()), kParticleCount, 0, 0);

			// ★追加: 描画前にコマンドリストをModelCommonに渡す
			modelCommon_->PreDraw(commandList);

			// (ここにゲームの描画コマンド)
			// --- 描画処理 (Draw) ---
			sceneManager_->Draw(viewProjectionData_->viewProjectionMatrix);

			// ImGuiの描画
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList); // commandList_.Get() から変更

			// 描画後処理
			dxCommon_->PostDraw();
		}
	}
}

void WindowsApplication::Finalize() {
	// ImGui解放
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	debugCamera_.reset();

	sceneManager_.reset();

	if(spriteCommon_) {
		spriteCommon_->Finalize();
		spriteCommon_.reset();
	}

	// その他マネージャクラスの解放
	AudioManager::Finalize();
	TextureManager::GetInstance()->Finalize();

	// DirectXCommonの終了処理
	if(dxCommon_) {
		dxCommon_->Finalize();
		dxCommon_.reset();
	}

	// COMの終了処理
	CoUninitialize();
}
