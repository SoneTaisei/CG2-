#include "MyGameApp.h"
#include "Input/KeyboardInput.h" // 必要に応じてインクルード
#include "Input/GamepadInput.h"  // 必要に応じてインクルード
#include <vector>

// ImGuiのヘッダ
#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"

// コンストラクタ：ウィンドウサイズを初期化
MyGameApp::MyGameApp(UINT width, UINT height) : width_(width), height_(height) {}

// デストラクタ：Finalizeを呼び出す
MyGameApp::~MyGameApp() {
	Finalize();
}

// アプリケーションの実行
void MyGameApp::Run() {
	Initialize();
	MainLoop();
}

// 初期化処理の全体管理
void MyGameApp::Initialize() {
	CoInitializeEx(0, COINIT_MULTITHREADED);
	InitializeWindow();
	InitializeDirectX();
	InitializeImGui();
	LoadAssets();
}

// メインループ
void MyGameApp::MainLoop() {
	MSG msg{};
	while(msg.message != WM_QUIT) {
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			Update();
			Render();
		}
	}
}

// 終了処理
void MyGameApp::Finalize() {
	// 実行中の可能性のある全てのGPUコマンドの完了を待つ
	WaitForGpu();

	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// イベントハンドルの解放
	if(fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}

	// COMの終了処理
	CoUninitialize();

	// これ以降のDirectXオブジェクトの解放は、MyGameAppクラスのインスタンスが
	// 破棄される際に、ComPtrのデストラクタによって自動的に行われる。
	// ここに手動でRelease()を記述する必要はない。
}

// ゲームロジックの更新
void MyGameApp::Update() {
	// --- 入力処理 ---
	KeyboardInput::GetInstance()->Update();
	GamepadInput::GetInstance()->Update();

	// 入力デバイスへのショートカット
	auto keyboard = KeyboardInput::GetInstance();
	auto gamepad = GamepadInput::GetInstance();

	// --- フレーム開始前の状態保存 ---
	preIsSound_ = isSound_;

	// --- ImGuiフレームの開始 ---
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#ifdef _DEBUG
	// ImGuiのデモウィンドウ表示
	ImGui::ShowDemoWindow();
#endif

	// --- ImGuiによるUIの定義とパラメータ更新 ---
	ImGui::Begin("Game Controls");

	// Sphereの制御
	ImGui::Checkbox("Sphere", &isSphere_);
	if(isSphere_ && ImGui::TreeNode("Sphere Details")) {
		ImGui::DragFloat3("Translate", &transformSphere_.translate.x, 0.1f);
		ImGui::DragFloat3("Rotate", &transformSphere_.rotate.x, 0.01f);
		ImGui::DragFloat3("Scale", &transformSphere_.scale.x, 0.1f, 0.01f, 10.0f);
		ImGui::Checkbox("Use MonsterBall Texture", &useMonsterBall_);

		// ライティング関連
		ImGui::ColorEdit4("Material Color", &materialData_->color.x);
		ImGui::DragFloat("Light Intensity", &directionalLightData_.intensity, 0.05f, 0.0f, 10.0f);
		ImGui::DragFloat3("Light Direction", &directionalLightData_.direction.x, 0.01f);
		//Normalize(directionalLightData_.direction); // 方向ベクトルなので正規化する

		ImGui::TreePop();
	}

	// Modelの制御
	ImGui::Checkbox("Model", &isModel_);
	if(isModel_ && ImGui::TreeNode("Model Details")) {
		ImGui::DragFloat3("Translate", &transformModel_.translate.x, 0.01f);
		ImGui::DragFloat3("Rotate", &transformModel_.rotate.x, 0.01f);
		ImGui::DragFloat3("Scale", &transformModel_.scale.x, 0.01f);
		ImGui::TreePop();
	}

	// Spriteの制御
	ImGui::Checkbox("Sprite", &isSprite_);
	if(isSprite_ && ImGui::TreeNode("Sprite Details")) {
		ImGui::DragFloat3("Translate", &transformSprite_.translate.x, 1.0f);
		ImGui::DragFloat3("Rotate", &transformSprite_.rotate.x, 0.01f);
		ImGui::DragFloat3("Scale", &transformSprite_.scale.x, 0.01f);

		if(ImGui::TreeNode("UV Transform")) {
			ImGui::DragFloat2("UV Translate", &uvTransformSprite_.translate.x, 0.01f);
			ImGui::DragFloat("UV Rotate", &uvTransformSprite_.rotate.z, 0.01f);
			ImGui::DragFloat2("UV Scale", &uvTransformSprite_.scale.x, 0.01f);
			ImGui::ColorEdit4("Sprite Color", &materialDataSprite_->color.x);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	// Soundの制御
	ImGui::Checkbox("Play Sound", &isSound_);

	ImGui::End();

	// --- カメラの更新 ---
	debugCamera_.Update();
	Matrix4x4 viewMatrix = debugCamera_.GetViewMatrix();
	Matrix4x4 projectionMatrix = debugCamera_.GetProjectionMatrix();
	Matrix4x4 viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);


	// --- 行列計算と定数バッファへのデータ書き込み ---

	// Sprite用行列
	Matrix4x4 worldMatrixSprite = TransformFunctions::MakeAffineMatrix(transformSprite_.scale, transformSprite_.rotate, transformSprite_.translate);
	Matrix4x4 projectionMatrixSprite = TransformFunctions::MakeOrthographicMatrix(0.0f, 0.0f, float(width_), float(height_), 0.0f, 100.0f);
	*transformationMatrixDataSprite_ = TransformFunctions::Multiply(worldMatrixSprite, projectionMatrixSprite);

	// Sphere用行列
	Matrix4x4 worldMatrixSphere = TransformFunctions::MakeAffineMatrix(transformSphere_.scale, transformSphere_.rotate, transformSphere_.translate);
	mappedTransformationMatrixSphere_->WVP = TransformFunctions::Multiply(worldMatrixSphere, viewProjectionMatrix);
	mappedTransformationMatrixSphere_->World = worldMatrixSphere;

	// Model用行列
	Matrix4x4 worldMatrixModel = TransformFunctions::MakeAffineMatrix(transformModel_.scale, transformModel_.rotate, transformModel_.translate);
	transformationMatrixDataModel_->WVP = TransformFunctions::Multiply(worldMatrixModel, viewProjectionMatrix);
	transformationMatrixDataModel_->World = worldMatrixModel;

	// UV Transform用行列
	Matrix4x4 uvTransformMatrix = TransformFunctions::MakeScaleMatrix(uvTransformSprite_.scale);
	uvTransformMatrix = TransformFunctions::Multiply(uvTransformMatrix, TransformFunctions::MakeRoteZMatrix(uvTransformSprite_.rotate.z));
	uvTransformMatrix = TransformFunctions::Multiply(uvTransformMatrix, TransformFunctions::MakeTranslateMatrix(uvTransformSprite_.translate));
	materialDataSprite_->uvTransform = uvTransformMatrix;

	// カメラとライトの情報を定数バッファに書き込む
	viewProjectionData_->viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);
	viewProjectionData_->cameraPosition = cameraTransform_.translate;
	viewProjectionData_->padding = 0.0f;

	// --- その他のゲームロジック ---

	// サウンド再生
	if(isSound_ && !preIsSound_) {
		//SoundPlayWave(xAudio2_.Get(), soundData1_);
	}
}

// 描画処理
void MyGameApp::Render() {
	HRESULT hr;

	// 1. コマンドの準備
	// フレームの最初に、命令を書き込むためのコマンドアロケータとリストをリセット
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), graphicsPipelineState_.Get());
	assert(SUCCEEDED(hr));

	// 2. 描画の初期設定
	// これから使うデスクリプタヒープをGPUに通知
	ID3D12DescriptorHeap *descriptorHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	// ビューポートとシザー矩形を設定
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

	// 3. 描画先の準備
	// これから描画するバックバッファのインデックスを取得
	backBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

	// リソースバリアを「表示用(Present)」から「描画先(Render Target)」へ変更
	D3D12_RESOURCE_BARRIER barrier = {}; // ローカル変数として宣言
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex_].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);

	// 描画先のRTVとDSVを設定
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), backBufferIndex_);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(); // ローカル変数として宣言
	commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// 4. 画面のクリア
	// 指定した色で画面全体をクリア
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	// 深度バッファもクリア
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 5. ゲームオブジェクトの描画
	// 全オブジェクトで共通のパイプラインステートなどを設定
	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
	commandList_->SetPipelineState(graphicsPipelineState_.Get());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//// --- 球の描画 ---
	//if(isSphere_) {
	//	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	//	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = useMonsterBall_ ? textureSrvHandleGPU_[1] : textureSrvHandleGPU_[0];
	//	commandList_->SetGraphicsRootDescriptorTable(2, srvHandle);
	//	commandList_->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere_->GetGPUVirtualAddress());
	//	commandList_->IASetVertexBuffers(0, 1, &vertexBufferViewSphere_);
	//	commandList_->IASetIndexBuffer(&indexBufferViewSphere_);
	//	commandList_->DrawIndexedInstanced(sphereIndexCount_, 1, 0, 0, 0);
	//}

	//// --- モデルの描画 ---
	//if(isModel_) {
	//	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress()); // マテリアルは球と共通
	//	commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_[2]); // モデル用テクスチャ
	//	commandList_->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModel_->GetGPUVirtualAddress());
	//	commandList_->IASetVertexBuffers(0, 1, &vertexBufferViewModel_);
	//	commandList_->IASetIndexBuffer(&indexBufferViewModel_);
	//	commandList_->DrawIndexedInstanced(modelIndexCount_, 1, 0, 0, 0);
	//}

	//// --- スプライトの描画 ---
	//if(isSprite_) {
	//	commandList_->SetGraphicsRootConstantBufferView(0, materialResourceSprite_->GetGPUVirtualAddress());
	//	commandList_->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite_->GetGPUVirtualAddress());
	//	commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_[0]); // uvCheckerテクスチャを使用
	//	commandList_->IASetVertexBuffers(0, 1, &vertexBufferViewSprite_);
	//	commandList_->IASetIndexBuffer(&indexBufferViewSprite_);
	//	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
	//}

	// 6. UIの描画
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());

	// 7. フレームの完成と表示
	// リソースバリアを「描画先」から「表示用」へ戻す
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier);

	// コマンドリストの記録を終了
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を命令
	ID3D12CommandList *commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// 画面に表示
	hr = swapChain_->Present(1, 0);
	assert(SUCCEEDED(hr));

	// 8. 同期
	// このフレームの処理がGPUで完了するまで待つ
	WaitForGpu();
}

// ウィンドウの初期化
void MyGameApp::InitializeWindow() {
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"MyGameWindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	RECT wrc = { 0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_) };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd_ = CreateWindow(
		wc.lpszClassName, L"MyGame", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top,
		nullptr, nullptr, wc.hInstance, nullptr
	);
	assert(hwnd_ != nullptr);
	ShowWindow(hwnd_, SW_SHOW);
}

// DirectXの初期化
void MyGameApp::InitializeDirectX() {
	HRESULT hr = S_OK;

#ifdef _DEBUG
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_));
	if(SUCCEEDED(hr)) {
		debugController_->EnableDebugLayer();
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));

	for(UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 desc{};
		hr = useAdapter_->GetDesc3(&desc);
		assert(SUCCEEDED(hr));
		if(!(desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			break;
		}
		useAdapter_.Reset();
	}
	assert(useAdapter_ != nullptr);

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
	for(auto level : featureLevels) {
		hr = D3D12CreateDevice(useAdapter_.Get(), level, IID_PPV_ARGS(&device_));
		if(SUCCEEDED(hr)) {
			break;
		}
	}
	assert(device_ != nullptr);

#ifdef _DEBUG
	hr = device_->QueryInterface(IID_PPV_ARGS(&infoQueue_));
	if(SUCCEEDED(hr)) {
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif

	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));

	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width_;
	swapChainDesc.Height = height_;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), hwnd_, &swapChainDesc, nullptr, nullptr, &swapChain1);
	assert(SUCCEEDED(hr));
	hr = swapChain1.As(&swapChain_);
	assert(SUCCEEDED(hr));

	rtvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	srvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	dsvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	for(UINT i = 0; i < 2; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), i);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandle);
	}

	hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

// ImGuiの初期化
void MyGameApp::InitializeImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd_);

	// ImGui用のSRVヒープを別途作成
	imguiSrvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);

	ImGui_ImplDX12_Init(device_.Get(),
						2, // SwapChainのバッファ数
						DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
						imguiSrvDescriptorHeap_.Get(),
						imguiSrvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
						imguiSrvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart()
	);
}

// アセット（リソース）の読み込みと作成
void MyGameApp::LoadAssets() {
	// ここにPSO作成、ルートシグネチャ作成、頂点バッファ作成などの処理を
	// main.cppから移植してきます。
	// 変数名は全て _ 付きのメンバ変数に変更してください。
	// 例：
	// rootSignature -> rootSignature_
	// vertexResourceSphere -> vertexResourceSphere_

	// ビューポートとシザー矩形の設定
	viewport_.Width = static_cast<float>(width_);
	viewport_.Height = static_cast<float>(height_);
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissorRect_.left = 0;
	scissorRect_.right = width_;
	scissorRect_.top = 0;
	scissorRect_.bottom = height_;

	// コマンドリストを一旦閉じて実行し、リソースアップロードなどを待つ
	commandList_->Close();
	ID3D12CommandList *commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);
	WaitForGpu(); // アップロード完了を待つ
}

// GPUの処理完了を待つヘルパー関数
void MyGameApp::WaitForGpu() {
	fenceValue_++;
	HRESULT hr = commandQueue_->Signal(fence_.Get(), fenceValue_);
	assert(SUCCEEDED(hr));

	if(fence_->GetCompletedValue() < fenceValue_) {
		hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		assert(SUCCEEDED(hr));
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}