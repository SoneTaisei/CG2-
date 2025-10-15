#include "Input/KeyboardInput.h"
#include "Input/GamepadInput.h"
#include"Utility/Utilityfunctions.h"
#include"Graphics/DebugCamera.h"
#include"Utility/BlendMode.h"
#include "Audio/AudioManager.h"
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "Graphics/TextureManager.h"
#include"Sprite/Sprite.h"
#include "Model/Model.h"

const int kWindowWidth = 1280;
const int kWindowHeight = 720;


// windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // ← この行を追加

	D3DResourceLeakChacker leakCheck;

	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// 予期せぬ事態(Unhandled)に補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	/*********************************************************
	*WindowsAPIの初期化
	*********************************************************/

	WNDCLASS wc{};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名
	wc.lpszClassName = L"MyDreamGame";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスを登録する
	RegisterClass(&wc);

	// クライアント領域サイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	// ウィンドウサイズを表す構造体にクライアントを入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	// クライアント領域をもとに実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ログのディレクトリ用意
	std::filesystem::create_directories("logs");

	// 現在の時刻を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	// 日本時間（PCの設定時間）に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
	// formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%d_%H%M%S}", localTime);
	// 時刻を使ってファイルを決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	// ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);

	// ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,    //使用するクラス名
		L"MyDreamGame",      //タイトルバーの文字（なんでもいい）
		WS_OVERLAPPEDWINDOW, //よく見るウィンドウスタイル
		CW_USEDEFAULT,       //表示X座標（Windowsに任せる）
		CW_USEDEFAULT,       //表示X座標（WindowsOSに任せる）
		wrc.right - wrc.left,//ウィンドウ横幅
		wrc.bottom - wrc.top,//ウィンドウ縦幅
		nullptr,             //親ウィンドウハンドル
		nullptr,             //メニューハンドル
		wc.hInstance,        //インスタンスハンドル
		nullptr);            //オプション

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);

	}


#endif

	// ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	/*********************************************************
	*DirectX初期化処理
	*********************************************************/

	// キーボードの初期化
	assert(KeyboardInput::GetInstance()->Initialize(wc.hInstance, hwnd));
	// コントローラーの初期化
	GamepadInput::GetInstance()->Initialize(wc.hInstance, hwnd); // こちらは接続されていなくてもエラーにしない

	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// HRESULTはWindows系のエラーコードであり関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	if(FAILED(hr)) {
		OutputDebugStringA(std::format("CreateDXGIFactory failed: HRESULT=0x{:08X}\n", hr).c_str());
	}
	//エラーが出たらassert
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	// いいアダプターを選択させる
	for(UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference
	(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		// アダプター情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプタでなければ採用
		if(!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報を
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	Microsoft::WRL::ComPtr<ID3D12Device> device;


	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
	  D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char *featureLevelStrings[] = { "12.2","12.1","12.0" };
	// 高い順に生成できるか試していく
	for(size_t i = 0; i < _countof(featureLevels); ++i) {
		// 採用したアダプターデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
		// 指定した機能レベルでデバイスが生成できたかを確認
		if(SUCCEEDED(hr)) {
			// 生成できたのでログ出力を行ってループを抜ける
			Log(std::format("FeatureLevel : {}\n", std::string(featureLevelStrings[i])));

			break;
		}
	}
	// デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	// 初期化完了のログ
	Log("Complete create D3D12Devie!!!\n");

	// SpriteクラスとModelクラスの静的初期化
	Model::StaticInitialize(device.Get());

#ifdef _DEBUG
	/*エラーが起きたら止める処理
	*********************************************************/
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// 一番危険なエラーの時にとまる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラーの時にとまる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時にとまる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		// 制御するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 制御するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージの表示を制御する
		infoQueue->PushStorageFilter(&filter);
	}
#endif

	/*********************************************************
	*デバッグカメラの作成
	*********************************************************/

	DebugCamera debugCamera;


	/*********************************************************
	*画面を青くする
	*********************************************************/

	// コマンドキューと生成する
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成がうまくいかなかったら起動できない
	assert(SUCCEEDED(hr));

	// コマンドアロケータを生成する
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケータの生成がうまくいかなかったら生成できない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成がうまくいかないと起動できない
	assert(SUCCEEDED(hr));

	// スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;
	swapChainDesc.Height = kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// ウィンドウの色
	swapChainDesc.SampleDesc.Count = 1;// マルチサンプル市内
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// 描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;// ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;// モニタに移ったら中身を破棄
	// コマンドキュー、ウィンドウハンドルの設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1 **>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

	/*********************************************************
	*デスクリプターヒープの生成
	*********************************************************/

	// RTV用のヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap =
		CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	// SRV用のヒープ作成を削除し、TextureManagerの初期化に置き換える
	TextureManager::GetInstance()->Initialize(device);

	// SwapChainからResourceを引っ張ってくる
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2];
	for(int i = 0; i < 2; ++i) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		// うまく取得できなければ起動できない
		assert(SUCCEEDED(hr));
	}

	// RTVの設定
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;// 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;// 2Dテクスチャとして書き込む
	// ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	// まず1つ目をつくる。1つ目は最初のところに作る。作る場所をこちらで指定しないといけない
	rtvHandles[0] = rtvStartHandle;
	rtvHandles[1].ptr = rtvStartHandle.ptr + descriptorSizeRTV;
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
	// 2つ目のディスクリプタハンドルを得る
	rtvHandles[1] = GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, 1);
	// 2つ目を作る
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);


	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain.Get()->GetCurrentBackBufferIndex();
	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	// 推移前のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 推移後のResouceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);
	// 描画先のRTVを設定する
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
	// 指定した色で画面全体をクリアにする
	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	// RenderTargetからPresentにする
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBarrier1を張る
	commandList->ResourceBarrier(1, &barrier);
	// コマンドリストの内容を確定させる。全てのコマンドを積んでからCloseする
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// 初期値0でFenceを作る
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// dxcCompilerを初期化
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// 現時点でincludeはしないが、includeに対応するための設定を行っておく
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	// FenceEventをassertする
	assert(fenceEvent != nullptr);

	// GPUにコマンドリストの実行を行わせる
	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList.Get() };
	commandQueue.Get()->ExecuteCommandLists(1, commandLists->GetAddressOf());
	// GPUとOSに画面の交換を行うように通知する
	swapChain.Get()->Present(1, 0);
	// Fenceの値を更新
	fenceValue++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値を代入するようにSignalを送る
	commandQueue.Get()->Signal(fence.Get(), fenceValue);
	/* Fenceの値が指定したSignal値にたどり着いているか確認する
	   GetCompletedValueの初期値はFence作成時に渡した初期値*/
	if(fence->GetCompletedValue() < fenceValue) {
		// 指定したSignalにたどり着いていないのでたどり着くまで待つ設定
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		// イベント待つ
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));

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
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; // CBVから変更
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	// TransformMatrix(Matrix4x4 * 2) 分のサイズを32bit(4byte)単位で指定
	rootParameters[1].Constants.Num32BitValues = sizeof(TransformMatrix) / 4;
	rootParameters[1].Constants.ShaderRegister = 0; // 頂点シェーダーの register(b0) に対応
	rootParameters[1].Constants.RegisterSpace = 0;
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
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
									 D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if(FAILED(hr)) {
		Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(0,
									 signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
									 IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	// InputLayout
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
	// PSOをブレンドモードの数だけ格納する配列
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStates[BlendMode::kCountOfBlnedMode];
	Microsoft::WRL::ComPtr<ID3D12PipelineState> spritePipelineStates[BlendMode::kCountOfBlnedMode];


	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// 全てのRGBAチャンネルに書き込み
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = true; // ブレンドを有効にする
	// アルファ値のブレンド設定は全モードで共通
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"shaders/Object3D.VS.hlsl",
																	  L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"shaders/Object3D.PS.hlsl",
																	 L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	assert(pixelShaderBlob != nullptr);

	// PSOの基本設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// ブレンドモードごとにPSOを生成
	for(int i = 0; i < static_cast<int>(BlendMode::kCountOfBlnedMode); ++i) {
		BlendMode mode = static_cast<BlendMode>(i);

		// ループの最初に必ずブレンド有効にリセットする
		blendDesc.RenderTarget[0].BlendEnable = true;

		switch(mode) {
		case BlendMode::kBlendModeNone: // ブレンドなし
			blendDesc.RenderTarget[0].BlendEnable = false;
			break;
		case BlendMode::kBlendModeNomal: // 通常（アルファブレンド）
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case BlendMode::kBlendModeAdd: // 加算
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		case BlendMode::kBlnedModeSubtract: // 減算
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		case BlendMode::kBlendModeMaltily: // 乗算
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
			break;
		case BlendMode::kBlendModeScreen: // スクリーン
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		}

		psoDesc.BlendState = blendDesc;

		// 3Dモデル用のPSOを作成
		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineStates[i]));
		assert(SUCCEEDED(hr));

		// スプライト用のPSOを作成 (深度テスト無効)
		D3D12_GRAPHICS_PIPELINE_STATE_DESC spritePsoDesc = psoDesc;
		spritePsoDesc.DepthStencilState.DepthEnable = false;
		hr = device->CreateGraphicsPipelineState(&spritePsoDesc, IID_PPV_ARGS(&spritePipelineStates[i]));
		assert(SUCCEEDED(hr));
	}

	// ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示する
	viewport.Width = kClientWidth; // 横幅
	viewport.Height = kClientHeight; // 縦幅
	viewport.TopLeftX = 0;// 描画開始位置(左上座標X)
	viewport.TopLeftY = 0;// 描画開始位置(左上座標Y)
	viewport.MinDepth = 0.0f;// 深度値(物の近さ)の最小
	viewport.MaxDepth = 1.0f;// 深度値(物の近さ)の最大

	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	/*********************************************************
	*Material用のResourceを作る
	*********************************************************/

	const UINT materialBufferSize =
		(sizeof(Material) + 255) & ~255u; // 256 バイトに丸め
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource =
		CreateBufferResource(device.Get(), materialBufferSize);
	// マテリアルにデータを書き込む
	Material *materialData = nullptr;
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
	// 今回は赤を書き込んでみる
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->lightingType = 0;
	materialResource->Unmap(0, nullptr);

	/*********************************************************
	*DirectionalLight用のResourceを作る
	*********************************************************/

	// デフォルト値はとりあえず以下のようにしておく
	DirectionalLight directionalLightData = {};
	directionalLightData.color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData.direction = { 0.0f,-1.0f,0.0f };
	directionalLightData.intensity = 1.0f;

	// GPUリソースを作る（256バイトアライン）
	const UINT directionalLightBufferSize = (sizeof(DirectionalLight) + 255) & ~255u;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =
		CreateBufferResource(device.Get(), directionalLightBufferSize);

	// CPU側の書き込み用ポインタを取得
	DirectionalLight *mappedDirectionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedDirectionalLightData));

	// directionalLightData の中身をコピーして書き込む
	*mappedDirectionalLightData = directionalLightData;
	directionalLightResource->Unmap(0, nullptr);

	UINT viewProjectionSize = (sizeof(ViewProjection) + 255) & ~255;
	Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionResource = CreateBufferResource(device.Get(), viewProjectionSize);
	ViewProjection *viewProjectionData = nullptr;
	viewProjectionResource->Map(0, nullptr, reinterpret_cast<void **>(&viewProjectionData));


	// カメラの初期化
	Matrix4x4 initViewMatrix = TransformFunctions::MakeIdentity4x4();
	float aspect = float(kClientWidth) / float(kClientHeight);
	Matrix4x4 initProjectionMatrix = TransformFunctions::MakePerspectiveFovMatrix(0.45f * 3.14159265f, aspect, 0.1f, 100.0f);

	debugCamera.Initialize(initViewMatrix, initProjectionMatrix, kClientWidth, kClientHeight);



	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	// win32の初期化
	ImGui_ImplWin32_Init(hwnd);
	// DX12の初期化 (srvDescriptorHeap.Get() の部分を変更)
	ImGui_ImplDX12_Init(device.Get(),
						swapChainDesc.BufferCount,
						rtvDesc.Format,
						TextureManager::GetInstance()->GetSrvDescriptorHeap(), // <<< 変更
						TextureManager::GetInstance()->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
						TextureManager::GetInstance()->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart()
	);

	ImGui_ImplDX12_CreateDeviceObjects();

	Model *sphereModel = Model::CreateSphere();
	Model *planeModel = Model::CreateFromObj("resources/plane", "plane.obj");

	// TextureManagerを使ってテクスチャを読み込む
	// この関数はテクスチャのハンドル(uint32_t型のインデックス)を返す
	uint32_t uvCheckerHandle = TextureManager::GetInstance()->Load("resources/uvChecker.png", commandList.Get());
	uint32_t monsterBallHandle = TextureManager::GetInstance()->Load("resources/monsterBall.png", commandList.Get());
	ModelData planeDataForTexture = planeModel->GetModelData();
	uint32_t planeTextureHandle = TextureManager::GetInstance()->Load(planeDataForTexture.material.textureFilePath, commandList.Get());

	// コマンドを閉じる
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// 実行
	commandQueue.Get()->ExecuteCommandLists(1, commandLists->GetAddressOf());

	// Fenceを使ってGPUの完了を待つ
	fenceValue++;
	commandQueue.Get()->Signal(fence.Get(), fenceValue);
	if(fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// 次のコマンドのためにリセット
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device.Get(), kClientWidth, kClientHeight);

	// DSV用のヒープででスクリプタの数は1
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;// formatをresourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	// DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	/*********************************************************
	*音声出力をする
	*********************************************************/

	//AudioManagerを初期化
	AudioManager::Initialize();

	const std::string alarmSoundHandle = AudioManager::LoadSound("resources/Alarm.wav");

	/*********************************************************
	*変数宣言
	*********************************************************/

	// テクスチャハンドル配列を作成
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandles;
	// TextureManagerにロードされている全テクスチャのGPUハンドルを取得してvectorに追加する
	for(size_t i = 0; i < TextureManager::GetInstance()->GetTextureCount(); ++i) {
		textureHandles.push_back(TextureManager::GetInstance()->GetGpuHandle(static_cast<uint32_t>(i)));
	}

	// Spriteクラスの静的リソースを初期化
	Sprite::StaticInitialize(device.Get(), kClientWidth, kClientHeight, textureHandles);

	// Transform変数を作る
	Transform transform = {
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	Transform transformSphere = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	Transform transformModel = {
	{1.0f, 1.0f, 1.0f},   // scale
	{0.0f, 0.0f, 0.0f},   // rotate
	{0.0f, 0.0f, 0.0f}    // translate
	};

	bool useMonsterBall = true;

	materialData->uvTransform = TransformFunctions::MakeIdentity4x4();

	// 使用するか
	bool isSphere = true;
	bool isModel = true;
	bool isSprite = true;
	bool isSound = false;

	// 音楽を鳴らすための変数
	bool preIsSound = false;

	// 現在選択中のブレンドモード
	int currentBlendMode = static_cast<int>(BlendMode::kBlendModeNomal);

	// 全キーの入力状態を取得する
	BYTE keys[256] = {};
	BYTE preKeys[256] = {};

	bool debugCameraMode = false;

	MSG msg{};
	// ウィンドウのxボタンが押されるまでループ
	while(msg.message != WM_QUIT) {
		// Windowにメッセージが来てたら最優先で処理させる
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {

			// --- 更新処理 ---
			KeyboardInput::GetInstance()->Update();
			GamepadInput::GetInstance()->Update();
			AudioManager::Update();

			// --- 入力判定の例 ---
			auto keyboard = KeyboardInput::GetInstance();
			auto gamepad = GamepadInput::GetInstance();

			// 前フレームの取得
			preIsSound = isSound;

			// ImGuiにこれからフレームが始まる旨を告げる
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

#ifdef _DEBUG

			// 開発用UIの処理
			ImGui::ShowDemoWindow();

#endif // DEBUG_


			ImGui::Begin("Window");

			/*Sphere
			*********************************************************/

			// 表示設定
			ImGui::Checkbox("isSphere", &isSphere);
			if(isSphere) {
				if(ImGui::TreeNode("Sphere")) {

					// 平行移動 (translate)
					ImGui::DragFloat3("Translate", &transformSphere.translate.x, 0.1f);

					// 回転 (rotate) - ラジアン単位、±π の範囲で表示
					ImGui::DragFloat3("Rotate", &transformSphere.rotate.x, 0.01f, -3.14f, 3.14f);

					// 拡大縮小 (scale)
					ImGui::DragFloat3("Scale", &transformSphere.scale.x, 0.1f, 0.01f, 10.0f);

					// テクスチャ切り替え
					ImGui::Checkbox("useMonsterBall", &useMonsterBall);

					// どのライティングを使うかの切り替え
					const char *lightingItems[] = { "No Lighting", "Lambert", "Half Lambert" };
					int currentItem = materialData->lightingType;
					if(ImGui::Combo("Lighting", &currentItem, lightingItems, IM_ARRAYSIZE(lightingItems))) {
						materialData->lightingType = currentItem;
					}
					// ライティングの色切り替え
					ImGui::ColorEdit4("MaterialColor", &materialData->color.x);

					// 光の強さを変える
					ImGui::DragFloat("LightingIntensity", &directionalLightData.intensity, 0.1f, 0.0f, 10.0f);

					// 光の位置を変える
					ImGui::DragFloat3("LightingDirection", &directionalLightData.direction.x, 0.1f, 0.01f, 0.01f);

					const char *blendModeItems[] = { "Normal", "Add", "Subtract", "Multiply", "Screen" };
					ImGui::Combo("Blend Mode", &currentBlendMode, blendModeItems, IM_ARRAYSIZE(blendModeItems));

					ImGui::TreePop();
				}
			}

			/*OBJModel
			*********************************************************/

			// 表示設定
			ImGui::Checkbox("isModel", &isModel);
			if(gamepad->IsButtonPressed(0)) {
				if(isModel) {
					if(ImGui::TreeNode("Model")) {

						// transformModel
						ImGui::DragFloat3("Translate", &transformModel.translate.x, 0.01f, -10.0f, 10.0f);
						ImGui::DragFloat3("Scale", &transformModel.scale.x, 0.01f, -10.0f, 10.0f);
						ImGui::DragFloat3("Rotate", &transformModel.rotate.x, 0.01f, -10.0f, 10.0f);

						ImGui::TreePop();
					}
				}
			}

			/*Sound
			*********************************************************/

			ImGui::Checkbox("isSound", &isSound);

			ImGui::End();

			// --- ImGuiの更新 ---
			ImGui::Begin("Sprite Control");
			static float pos[2] = { 200.0f, 200.0f };
			static float size[2] = { 100.0f, 100.0f };
			static float scale[2] = { 1.0f, 1.0f };
			static float angle = 0.0f;
			static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			ImGui::DragFloat2("Position", pos);
			ImGui::DragFloat2("Size", size);
			ImGui::DragFloat2("Scale", scale, 0.01f);
			ImGui::SliderAngle("Angle", &angle);
			ImGui::ColorEdit4("Color", color);
			ImGui::End();

			//デバッグカメラの処理
			if(KeyboardInput::GetInstance()->IsKeyPressed(DIK_F3)) {
				debugCameraMode = !debugCameraMode; // F3キーでモードをトグル(ON/OFF切り替え)
				if(!debugCameraMode) {
					// デバッグカメラをOFFにした瞬間に、カメラの状態をリセットする
					debugCamera.Reset();
				}
			}
			if(debugCameraMode) {
				debugCamera.Update();
			}

			// ゲームの処理
			backBufferIndex = swapChain.Get()->GetCurrentBackBufferIndex();

			// バリア Present → RenderTarget
			barrier.Type =
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags =
				D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource =
				swapChainResources[backBufferIndex].Get();
			barrier.Transition.StateBefore =
				D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter =
				D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList->ResourceBarrier(1, &barrier);

			// 描画先のRTVとDSVを設定する
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);


			// 描画先セット＆クリア
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			// 指定した深度で画面全体をクリアにする
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			/*********************************************************
			*更新処理
			*********************************************************/

			/* Soundの再生
			*********************************************************/

			// isSoundがtrueになり、かつ前のフレームではfalseだった場合に再生
			if(isSound && !preIsSound) {
				AudioManager::Play(alarmSoundHandle); // ← こちらに変更
			}

			/*********************************************************
			*描画処理
			*********************************************************/

			// 描画コマンド
			// TextureManagerからSRVディスクリプタヒープを取得して配列に格納
			ID3D12DescriptorHeap *heaps[] = { TextureManager::GetInstance()->GetSrvDescriptorHeap() };
			// ↑で作成した配列をコマンドリストに設定する
			commandList->SetDescriptorHeaps(1, heaps);
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);

			// --- 3Dモデルの描画準備 ---
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(graphicsPipelineStates[currentBlendMode].Get()); // 3Dモデル用のPSO
			Model::PreDraw(commandList.Get());

			// この行を追加します
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// 共通マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			Matrix4x4 viewMatrix = debugCamera.GetViewMatrix();
			Matrix4x4 projectionMatrix = debugCamera.GetProjectionMatrix();
			// 共通カメラCBufferの場所を設定
			viewProjectionData->viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);
			viewProjectionData->cameraPosition = debugCamera.GetTranslation();
			commandList->SetGraphicsRootConstantBufferView(3, viewProjectionResource->GetGPUVirtualAddress());
			// 共通ライトCBufferの場所を設定
			*mappedDirectionalLightData = directionalLightData;
			commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());

			// --- 球の描画 ---
			/*if(isSphere) {
				D3D12_GPU_DESCRIPTOR_HANDLE sphereTextureHandle = useMonsterBall ? TextureManager::GetInstance()->GetGpuHandle(monsterBallHandle) : TextureManager::GetInstance()->GetGpuHandle(uvCheckerHandle);
				sphereModel->Draw(transformSphere, viewProjectionData->viewProjectionMatrix, sphereTextureHandle);
			}*/

			// --- モデルの描画 ---
			if(isModel) {
				commandList->SetPipelineState(spritePipelineStates[currentBlendMode].Get());
				// まず、TextureManagerを使って整理番号(uvCheckerHandle)からGPUハンドルを取得する
				D3D12_GPU_DESCRIPTOR_HANDLE planeGpuHandle = TextureManager::GetInstance()->GetGpuHandle(planeTextureHandle);
				// 取得したGPUハンドルをDraw関数に渡す
				planeModel->Draw(transformModel, viewProjectionData->viewProjectionMatrix, planeGpuHandle);
			}

			if(isSprite) {
				// スプライトを描画する前に、スプライト専用のPSOに切り替える
				commandList->SetPipelineState(spritePipelineStates[currentBlendMode].Get());

				// スプライト描画の開始を宣言
				Sprite::PreDraw(commandList.Get());

				// Sprite::Draw() で描画
				Sprite::Draw(
					pos[0], pos[1],                           // 位置
					size[0], size[1],                         // サイズ
					uvCheckerHandle,                                        // テクスチャハンドル (0: uvChecker)
					scale[0], scale[1],                       // 拡縮
					angle,                                    // 回転
					{ color[0], color[1], color[2], color[3] } // 色
				);

				Sprite::Draw(
					640.0f, 360.0f,                   // 位置 (X=640, Y=360)
					128.0f, 128.0f,                   // サイズ (128x128ピクセル)
					monsterBallHandle,                                // テクスチャハンドル (1: モンスターボール)
					1.0f, 1.0f,                       // 拡縮 (等倍)
					0.0f,                             // 回転 (なし)
					{ 1.0f, 1.0f, 1.0f, 1.0f }          // 色 (不透明な白)
				);

				// 別のスプライトも簡単に追加できる
				Sprite::Draw(150, 50, 100, 100, 1); // 座標(50,50)にサイズ100x100でモンスターボールを描画
			}

			// ImGUiの内部コマンドを生成する
			ImGui::Render();



			// 実際のcommandListのImGuiの描画コマンドを積む
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

			// バリア RenderTarget → Present
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList->ResourceBarrier(1, &barrier);

			// 1. コマンドリストを閉じる
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			// 2. コマンドリストを実行
			commandQueue.Get()->ExecuteCommandLists(1, commandLists->GetAddressOf());

			// 3. 画面を表示
			swapChain.Get()->Present(1, 0);

			// --- GPUが終わるのを待つ ←ここ追加 ---
			fenceValue++;
			commandQueue.Get()->Signal(fence.Get(), fenceValue);
			if(fence->GetCompletedValue() < fenceValue) {
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			// 4. 次のフレームに向けて準備（リセット）
			commandAllocator->Reset();
			commandList->Reset(commandAllocator.Get(), nullptr);
		}
	}

	// GPUの処理がすべて完了するのを待つ
	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);
	if(fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	/*********************************************************
	*DirectX12のオブジェクト解放処理
	*********************************************************/

	// Modelインスタンスを解放する処理を追加
	delete planeModel;
	delete sphereModel;

	// ======== ImGui解放 ============
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	AudioManager::Finalize();
	Sprite::StaticFinalize();
	TextureManager::GetInstance()->Finalize();
	CloseHandle(fenceEvent);


	// ======== COMの終了処理 ============
	CoUninitialize();
	return 0;
}
