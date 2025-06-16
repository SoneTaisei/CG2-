#include "MyGameApp.h"

MyGameApp::MyGameApp(UINT width, UINT height) {
	width_ = width;
	height_ = height;
}

MyGameApp::~MyGameApp() {
	Finalize();
}

void MyGameApp::Run() {
	Initialize();
	MainLoop();
	Finalize();
}

void MyGameApp::Initialize() {
	CoInitializeEx(0, COINIT_MULTITHREADED); // COM初期化（忘れず！）

	InitializeWindow();     // ウィンドウを作成して表示
	InitializeDirectX();    // DXGIファクトリ、アダプタ、デバイス、コマンド類など
	InitializeImGui();      // ImGuiコンテキスト、Win32+DX12 初期化
	LoadAssets();           // 頂点バッファやテクスチャなどのリソース生成
}

void MyGameApp::MainLoop() {
	MSG msg{};
	while(msg.message != WM_QUIT) {
		// Windowsメッセージの処理（閉じるボタン等）
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// フレーム開始
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			Update();  // ゲームロジック・入力・変数更新など
			Render();  // コマンドリスト実行・ImGui描画・Present

			// フレーム終了
			ImGui::Render();
		}
	}
}

void MyGameApp::Finalize() {
	// === ImGui 解放 ===
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// === 各種リソース解放 ===
	if(depthStencilResource) { depthStencilResource->Release(); depthStencilResource = nullptr; }

	for(int i = 0; i < 2; ++i) {
		if(swapChainResources[i]) {
			swapChainResources[i]->Release();
			swapChainResources[i] = nullptr;
		}
	}

	if(dsvDescriptorHeap) { dsvDescriptorHeap->Release(); dsvDescriptorHeap = nullptr; }
	if(rtvDescriptorHeap) { rtvDescriptorHeap->Release(); rtvDescriptorHeap = nullptr; }
	if(srvDescriptorHeap) { srvDescriptorHeap->Release(); srvDescriptorHeap = nullptr; }
	if(imguiSrvDescriptorHeap) { imguiSrvDescriptorHeap->Release(); imguiSrvDescriptorHeap = nullptr; }

	if(commandList) { commandList->Release(); commandList = nullptr; }
	if(commandAllocator) { commandAllocator->Release(); commandAllocator = nullptr; }
	if(commandQueue) { commandQueue->Release(); commandQueue = nullptr; }

	if(fence) { fence->Release(); fence = nullptr; }
	if(fenceEvent) { CloseHandle(fenceEvent); fenceEvent = nullptr; }

	if(rootSignature) { rootSignature->Release(); rootSignature = nullptr; }
	if(graphicsPipelineState) { graphicsPipelineState->Release(); graphicsPipelineState = nullptr; }

	if(signatureBlob) { signatureBlob->Release(); signatureBlob = nullptr; }
	if(errorBlob) { errorBlob->Release(); errorBlob = nullptr; }
	if(pixelShaderBlob) { pixelShaderBlob->Release(); pixelShaderBlob = nullptr; }
	if(vertexShaderBlob) { vertexShaderBlob->Release(); vertexShaderBlob = nullptr; }

	if(includeHandler) { includeHandler->Release(); includeHandler = nullptr; }
	if(dxcCompiler) { dxcCompiler->Release(); dxcCompiler = nullptr; }
	if(dxcUtils) { dxcUtils->Release(); dxcUtils = nullptr; }

	if(swapChain) { swapChain->Release(); swapChain = nullptr; }

	if(useAdapter) { useAdapter->Release(); useAdapter = nullptr; }
	if(dxgiFactory) { dxgiFactory->Release(); dxgiFactory = nullptr; }

#ifdef _DEBUG
	if(debugController) { debugController->Release(); debugController = nullptr; }
#endif

	// === Liveオブジェクトチェック ===
	IDXGIDebug1 *debug = nullptr;
	if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	if(device) { device->Release(); device = nullptr; }

	// COM終了
	CoUninitialize();
}

//void MyGameApp::Update() {
//	// ImGuiにこれからフレームが始まる旨を告げる
//	ImGui_ImplDX12_NewFrame();
//	ImGui_ImplWin32_NewFrame();
//	ImGui::NewFrame();
//
//	// 開発用UIの処理
//	ImGui::ShowDemoWindow();
//
//	ImGui::Begin("Window");
//	// 平行移動 (translate)
//	ImGui::DragFloat3("Translate", &transformSphere.translate.x, 0.1f);
//
//	// 回転 (rotate) - ラジアン単位、±π の範囲で表示
//	ImGui::DragFloat3("Rotate", &transformSphere.rotate.x, 0.01f, -3.14f, 3.14f);
//
//	// 拡大縮小 (scale)
//	ImGui::DragFloat3("Scale", &transformSphere.scale.x, 0.1f, 0.01f, 10.0f);
//
//	// テクスチャ切り替え
//	ImGui::Checkbox("useMonsterBall", &useMonsterBall);
//
//	// ライティングを使うかの切り替え
//	ImGui::Checkbox("useLight", &materialData->enableLighting);
//
//	// 謎
//	ImGui::DragFloat("LightingIntensity", &directionalLightData.intensity, 0.1f, 0.0f, 10.0f);
//
//	// 光の位置を変える
//	ImGui::DragFloat3("LightingDirection", &directionalLightData.direction.x, 0.1f, 0.01f, 0.01f);
//
//	// ライティングのカラー切り替え
//	ImGui::DragFloat4("LightingColor", &directionalLightData.color.x, 0.1f, 0.01f, 0.01f);
//
//
//	ImGui::End();
//
//
//	// ゲームの処理
//	backBufferIndex = swapChain->GetCurrentBackBufferIndex();
//
//	// バリア Present → RenderTarget
//	barrier.Type =
//		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags =
//		D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource =
//		swapChainResources[backBufferIndex];
//	barrier.Transition.StateBefore =
//		D3D12_RESOURCE_STATE_PRESENT;
//	barrier.Transition.StateAfter =
//		D3D12_RESOURCE_STATE_RENDER_TARGET;
//	commandList->ResourceBarrier(1, &barrier);
//
//	// 描画先のRTVとDSVを設定する
//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
//
//
//	// 描画先セット＆クリア
//	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
//	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
//
//	// 指定した深度で画面全体をクリアにする
//	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
//
//	/*********************************************************
//	*更新処理
//	*********************************************************/
//
//	// Y軸を回転させる
//	transform.rotate.y += 0.03f;
//
//	/*camera用の座標変換
//	*********************************************************/
//
//	Matrix4x4 worldMatrix =
//		TransformFunctions::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
//	Matrix4x4 viewMatrix =
//		TransformFunctions::MakeIdentity4x4();
//	Matrix4x4 projectionMatrix =
//		TransformFunctions::MakeOrthographicMatrix(0.0f, 0.0f, float(width_), float(height_), 0.0f, 100.0f);
//	Matrix4x4 worldViewProjectionMatrix =
//		TransformFunctions::Multiply(worldMatrix, TransformFunctions::Multiply(viewMatrix, projectionMatrix));
//	//*transformationMatrixData = worldViewProjectionMatrix;
//
//	/*Sprite用の座標変換
//	*********************************************************/
//
//	Matrix4x4 worldMatrixSprite =
//		TransformFunctions::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
//	Matrix4x4 viewMatrixSprite =
//		TransformFunctions::MakeIdentity4x4();
//	Matrix4x4 projectionMatrixSprite =
//		TransformFunctions::MakeOrthographicMatrix(0.0f, 0.0f, float(width_), float(height_), 0.0f, 100.0f);
//	Matrix4x4 worldViewProjectionMatrixSprite =
//		TransformFunctions::Multiply(worldMatrixSprite, TransformFunctions::Multiply(viewMatrixSprite, projectionMatrixSprite));
//	*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;
//
//	/*Sphere用の座標変換
//	*********************************************************/
//
//	// 1. カメラのビュー行列を計算（カメラTransformの逆行列）
//	Matrix4x4 viewMatrixSphere
//		= TransformFunctions::Inverse(
//			TransformFunctions::MakeAffineMatrix(
//				cameraTransform.scale,
//				cameraTransform.rotate,
//				cameraTransform.translate
//			)
//		);
//
//	// 2. パース投影行列を作成（FOV 45度、アスペクト比、near=0.1, far=100）
//	float aspect = float(width_) / float(height_);
//	Matrix4x4 projectionMatrixSphere
//		= TransformFunctions::MakePerspectiveFovMatrix(
//			45.0f * 3.14159265f / 180.0f, // ラジアン
//			aspect,
//			0.1f, 100.0f
//		);  // :contentReference[oaicite:2]{index=2}
//
//	// 3. ワールド→ビュー→投影をまとめてWVP行列に
//	Matrix4x4 worldMatrixSphere
//		= TransformFunctions::MakeAffineMatrix(
//			transformSphere.scale,
//			transformSphere.rotate,
//			transformSphere.translate
//		);
//
//	Matrix4x4 wvpSphere
//		= TransformFunctions::Multiply(
//			worldMatrixSphere,
//			TransformFunctions::Multiply(viewMatrixSphere, projectionMatrixSphere)
//		);
//
//
//	// 4. 定数バッファに書き込む
//	transformationMatrixDataSphere->WVP = wvpSphere;
//	transformationMatrixDataSphere->World = worldMatrixSphere;
//
//
//
//	/*********************************************************
//	*描画処理
//	*********************************************************/
//
//	// 描画コマンド
//	commandList->SetDescriptorHeaps(1, descriptorHeaps_);
//	commandList->RSSetViewports(1, &viewport);
//	commandList->RSSetScissorRects(1, &scissorRect);
//	commandList->SetGraphicsRootSignature(rootSignature);
//	commandList->SetPipelineState(graphicsPipelineState);
//	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
//	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//	// マテリアルCBufferの場所を設定
//	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
//	commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());
//
//	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle =
//		useMonsterBall
//		? textureSrvHandleGPU[1]   // Monster Ball
//		: textureSrvHandleGPU[0];
//
//	//SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である
//	commandList->SetGraphicsRootDescriptorTable(2, srvHandle);
//
//	commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
//	commandList->IASetIndexBuffer(&indexBufferViewSphere);
//	commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
//	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress());
//
//	viewProjectionData->viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);
//	viewProjectionData->cameraPosition = cameraTransform.translate;
//	viewProjectionData->padding = 0.0f;
//
//	commandList->SetGraphicsRootConstantBufferView(3, viewProjectionResource->GetGPUVirtualAddress());
//
//	// ライトの変更内容をGPUに書き戻す
//	*mappedDirectionalLightData = directionalLightData;
//
//	commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
//
//	// マテリアルCBufferの場所を指定
//	commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
//
//	// Spriteの描画。変更が必要なものだけを変更する
//	commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
//	//SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である
//	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU[0]);
//	// TransformationMatrixCBufferの場所を設定
//	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
//	// 描画！
//	//commandList->DrawInstanced(6, 1, 0, 0);
//
//	// ImGUiの内部コマンドを生成する
//	ImGui::Render();
//
//	// 実際のcommandListのImGuiの描画コマンドを積む
//	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
//
//	// バリア RenderTarget → Present
//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
//	commandList->ResourceBarrier(1, &barrier);
//
//	// 1. コマンドリストを閉じる
//	hr = commandList->Close();
//	assert(SUCCEEDED(hr));
//
//	// 2. コマンドリストを実行
//	commandQueue->ExecuteCommandLists(1, commandLists);
//
//	// 3. 画面を表示
//	swapChain->Present(1, 0);
//
//	// --- GPUが終わるのを待つ ←ここ追加 ---
//	fenceValue++;
//	commandQueue->Signal(fence, fenceValue);
//	if(fence->GetCompletedValue() < fenceValue) {
//		fence->SetEventOnCompletion(fenceValue, fenceEvent);
//		WaitForSingleObject(fenceEvent, INFINITE);
//	}
//
//	// 4. 次のフレームに向けて準備（リセット）
//	commandAllocator->Reset();
//	commandList->Reset(commandAllocator, nullptr);
//}

void MyGameApp::Update() {
}

void MyGameApp::Render() {
	// コマンドアロケータ・リストをリセット
	commandAllocator->Reset();
	commandList->Reset(commandAllocator, graphicsPipelineState);

	// バックバッファのインデックス取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// ===== リソースバリア: Present → RenderTarget =====
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);

	// ===== 描画開始 =====
	// 描画先を設定
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);

	// 画面をクリア
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

	// ImGui描画準備（もし必要なら）
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// ===== リソースバリア: RenderTarget → Present =====
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	// コマンドリストを閉じる
	HRESULT hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// コマンドをキューに積んで実行
	ID3D12CommandList *cmdLists[] = { commandList };
	commandQueue->ExecuteCommandLists(1, cmdLists);

	// 画面に表示
	swapChain->Present(1, 0);

	// フェンス待機（同期処理）
	//WaitForGpu();
}

void MyGameApp::InitializeDirectX() {
	HRESULT hr = S_OK;

#ifdef _DEBUG
	// デバッグレイヤー有効化
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	if(SUCCEEDED(hr)) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// DXGIファクトリ作成
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	// ハードウェアアダプタ選択
	for(UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 desc{};
		useAdapter->GetDesc3(&desc);
		if(!(desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			break;
		}
		useAdapter->Release();
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	// デバイス作成
	hr = D3D12CreateDevice(useAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
	assert(SUCCEEDED(hr));

#ifdef _DEBUG
	if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->Release();
	}
#endif

	// コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));

	// コマンドアロケータ & リスト作成
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));

	// スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width_;
	swapChainDesc.Height = height_;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd_, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1 **>(&swapChain));
	assert(SUCCEEDED(hr));

	// RTVディスクリプタヒープ作成（2つ）
	rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// バックバッファ取得 & RTV作成
	for(int i = 0; i < 2; ++i) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		assert(SUCCEEDED(hr));

		rtvHandles[i] = GetCPUDescriptorHandle(rtvDescriptorHeap, rtvDescriptorSize, i);
		device->CreateRenderTargetView(swapChainResources[i], nullptr, rtvHandles[i]);
	}

	backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// フェンス作成
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));
	fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent != nullptr);
}

void MyGameApp::InitializeWindow() {
	// ウィンドウクラスを登録
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc; // ユーザー定義のウィンドウプロシージャ
	wc.lpszClassName = L"MyGameWindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);

	// クライアント領域サイズをもとに、ウィンドウサイズを調整
	RECT wrc = { 0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_) };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウを生成
	hwnd_ = CreateWindow(
		wc.lpszClassName,            // クラス名
		L"MyGame",                   // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,         // ウィンドウスタイル
		CW_USEDEFAULT,               // X位置
		CW_USEDEFAULT,               // Y位置
		wrc.right - wrc.left,        // 幅
		wrc.bottom - wrc.top,        // 高さ
		nullptr,                     // 親ウィンドウなし
		nullptr,                     // メニューなし
		wc.hInstance,                // インスタンスハンドル
		nullptr                      // オプション
	);

	assert(hwnd_ != nullptr);

	// ウィンドウを表示
	ShowWindow(hwnd_, SW_SHOW);

	// デバッグ用ログ出力（あれば便利）
	Log("Window initialized.\n");
}

void MyGameApp::InitializeImGui() {
	// ImGui のバージョンチェックとコンテキスト作成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// ダークスタイル適用（好みに応じて変更可）
	ImGui::StyleColorsDark();

	// Win32 側初期化
	ImGui_ImplWin32_Init(hwnd_);

	// DX12 側初期化
	IDXGISwapChain4 *swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	//swapChain_->GetDesc(&swapChainDesc);

	//// DX12の初期化
	//ImGui_ImplDX12_Init(device,
	//	swapChainDesc.BufferCount,
	//	rtvDesc.Format,
	//	srvDescriptorHeap,
	//	srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
	//	srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	//);

	// ImGui の内部リソース初期化（※フォント等）
	ImGui_ImplDX12_CreateDeviceObjects();

	ID3D12DescriptorHeap *descriptorHeaps[1] = { srvDescriptorHeap };
	descriptorHeaps_[0] = descriptorHeaps[0];
}

void MyGameApp::LoadAssets() {
	/*********************************************************
	*PSOを生成する
	*********************************************************/

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// DescriptorRangeの設定
	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	descriptorRange[0].BaseShaderRegister = 3;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;// offsetを自動計算
	descriptorRange[1].BaseShaderRegister = 4;
	descriptorRange[1].NumDescriptors = 3;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;// offsetを自動計算


	// RootParameter作成。複数設定できるので。今回は結果1つだけなので長さ1の配列
	D3D12_ROOT_PARAMETER rootParameters[5] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;// CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;// PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;// レジスタ番号0とバインド
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;// CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;// VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;// レジスタ番号0を使う
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;// DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;// ZPixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;// Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);// Tableで利用する数
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;// CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;// PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 3;// レジスタ番号1を使う
	rootParameters[3].Descriptor.RegisterSpace = 0;
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[4].Descriptor.ShaderRegister = 1; // ← DirectionalLight に対応
	rootParameters[4].Descriptor.RegisterSpace = 0;
	descriptionRootSignature.pParameters = rootParameters;// ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);// 配列の長さ

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;// バイリニアフィルタ。拡大縮小してもLinearなら滑らかになる
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;// 比較しない
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;// ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;// レジスタ番号0を使う
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;// PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリする
	ID3DBlob *signatureBlob = nullptr;
	ID3DBlob *errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if(FAILED(hr)) {
		Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	ID3D12RootSignature *rootSignature = nullptr;
	hr = device->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;//3次元なのでXYZが必要
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// 全ての色素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	IDxcBlob *vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl",
		L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob *pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl",
		L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジーのタイプ。三角形を選択
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定(気にしなくていい)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 上で設定したものを実際に生成
	ID3D12PipelineState *graphicsPipelineState = nullptr;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));


	assert(SUCCEEDED(hr));

	// クラスのメンバに代入
	this->rootSignature = rootSignature;
	this->graphicsPipelineState = graphicsPipelineState;

	/*********************************************************
	*VertexResourceを生成する
	*********************************************************/

	// 頂点リソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする。
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeof(VertexData) * 6;// どのくらいデータを使うかをGPUに伝える。三角形は三点なので*3
	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点リソースを作る
	ID3D12Resource *vertexResource = CreateBufferResource(device, sizeof(VertexData) * 6);
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	// 頂点バッファビューを作成する
	// リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;// 何バイト分かを書く
	// 1頂点当たりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);// 三角形の1頂点当たりのデータサイズ


	// ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示する
	viewport.Width = float(width_); // 横幅
	viewport.Height = float(height_); // 縦幅
	viewport.TopLeftX = 0;// 描画開始位置(左上座標X)
	viewport.TopLeftY = 0;// 描画開始位置(左上座標Y)
	viewport.MinDepth = 0.0f;// 深度値(物の近さ)の最小
	viewport.MaxDepth = 1.0f;// 深度値(物の近さ)の最大

	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = width_;
	scissorRect.top = 0;
	scissorRect.bottom = height_;

	/*********************************************************
	*Material用のResourceを作る
	*********************************************************/

	// マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	//ID3D12Resource *materialResource = CreateBufferResource(device, sizeof(Vector4));

	const UINT materialBufferSize =
		(sizeof(Material) + 255) & ~255u; // 256 バイトに丸め
	materialResource =
		CreateBufferResource(device, materialBufferSize);
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
	// 今回は赤を書き込んでみる
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = 0;

	/*********************************************************
	*DirectionalLight用のResourceを作る
	*********************************************************/

	// デフォルト値はとりあえず以下のようにしておく
	directionalLightData.color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData.direction = { 0.0f,-1.0f,0.0f };
	directionalLightData.intensity = 1.0f;

	// GPUリソースを作る（256バイトアライン）
	const UINT directionalLightBufferSize = (sizeof(DirectionalLight) + 255) & ~255u;
	ID3D12Resource *directionalLightResource =
		CreateBufferResource(device, directionalLightBufferSize);

	// CPU側の書き込み用ポインタを取得
	DirectionalLight *mappedDirectionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedDirectionalLightData));

	// directionalLightData の中身をコピーして書き込む
	*mappedDirectionalLightData = directionalLightData;

	/*********************************************************
	*TransformationMatrix用のResourceを作る
	*********************************************************/

	// WVP用のリソースを作る。Matrix4x41つ分のサイズを用意する
	ID3D12Resource *wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
	// データを書き込む
	Matrix4x4 *wvpData = nullptr;
	// 書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void **>(&wvpData));
	// 谷行列を書き込んでおく
	*wvpData = TransformFunctions::MakeIdentity4x4();



	/*********************************************************
	*sprite用のResourceを作る
	*********************************************************/

	// Sprite用のマテリアル用リソースを作る
	ID3D12Resource *materialResourceSprite = CreateBufferResource(device, sizeof(Material));
	Material *materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void **>(&materialDataSprite));

	// 初期設定：色は白、ライティングは無効
	materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialDataSprite->enableLighting = false;

	ID3D12Resource *vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);
	// 頂点BufferViewを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite = {};
	// リソースの先頭アドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点当たりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	VertexData *vertexDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void **>(&vertexDataSprite));

	vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };

	/*1つ目の三角形
	*********************************************************/

	// 左下
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	// 上
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	// 右下
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };

	/*2つ目の三角形
	*********************************************************/

	// 左下
	vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	// 上
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	// 右下
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };

	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	ID3D12Resource *transformationMatrixResourceSprite = CreateBufferResource(device, sizeof(TransformMatrix));
	// 書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSprite));
	// 単位行列を書き込んでおく
	*transformationMatrixDataSprite = TransformFunctions::MakeIdentity4x4();

	UINT viewProjectionSize = (sizeof(ViewProjection) + 255) & ~255;
	ID3D12Resource *viewProjectionResource = CreateBufferResource(device, viewProjectionSize);
	ViewProjection *viewProjectionData = nullptr;
	viewProjectionResource->Map(0, nullptr, reinterpret_cast<void **>(&viewProjectionData));

	/*********************************************************
	*球体を生成する
	*********************************************************/

	/*メッシュ生成
	*********************************************************/

	// 頂点とインデックスの配列を作成
	std::vector<VertexData> sphereVertices;

	std::vector<uint32_t> sphereIndices;
	CreateSphereMesh(sphereVertices, sphereIndices, 1.0f, 16, 32); // 半径1.0, 縦16, 横32 分割

	// 頂点バッファリソース作成
	ID3D12Resource *vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * sphereVertices.size());

	// 頂点データ書き込み
	VertexData *mappedVertexData = nullptr;
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void **>(&mappedVertexData));
	std::memcpy(mappedVertexData, sphereVertices.data(), sizeof(VertexData) * sphereVertices.size());
	vertexResourceSphere->Unmap(0, nullptr);

	// 頂点バッファビュー作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	vertexBufferViewSphere.SizeInBytes = UINT(sizeof(VertexData) * sphereVertices.size());
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	/*resourceを生成する
	*********************************************************/

	// インデックスバッファリソース作成
	ID3D12Resource *indexResourceSphere = CreateBufferResource(device, sizeof(uint32_t) * sphereIndices.size());

	// 頂点データをGPUに書き込む（位置と法線をセット）
	VertexData *vertexData = nullptr;
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

	for(size_t index = 0; index < sphereVertices.size(); ++index) {
		vertexData[index] = sphereVertices[index];
		vertexData[index].normal = {
			vertexData[index].position.x,
			vertexData[index].position.y,
			vertexData[index].position.z
		};
	}

	vertexResourceSphere->Unmap(0, nullptr);

	uint32_t *mappedIndexData = nullptr;
	indexResourceSphere->Map(0, nullptr, reinterpret_cast<void **>(&mappedIndexData));
	std::memcpy(mappedIndexData, sphereIndices.data(), sizeof(uint32_t) * sphereIndices.size());
	indexResourceSphere->Unmap(0, nullptr);

	// 球体用の TransformationMatrix のリソースを作る
	ID3D12Resource *transformationMatrixResourceSphere = CreateBufferResource(device, sizeof(TransformMatrix));

	// Map して書き込みアドレスを取得
	transformationMatrixResourceSphere->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSphere));

	// 初期値として単位行列を書いておく（あとで更新される）
	transformationMatrixDataSphere->WVP = TransformFunctions::MakeIdentity4x4();
	transformationMatrixDataSphere->World = TransformFunctions::MakeIdentity4x4();

	// インデックスバッファビュー作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
	indexBufferViewSphere.SizeInBytes = UINT(sizeof(uint32_t) * sphereIndices.size());
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;

	UINT indexCount = static_cast<UINT>(sphereIndices.size());

}
