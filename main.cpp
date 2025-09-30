#include"Utilityfunctions.h"
#include"DebugCamera.h"
#include"BlendMode.h"


const int kWindowWidth = 1280;
const int kWindowHeight = 720;



// windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

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

	// 出力ウィンドウの文字出力
	OutputDebugStringA("Hello,DirectX\n");

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
	ID3D12Debug1 *debugController = nullptr;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);

	}
	debugController->Release();


#endif

	// ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	/*********************************************************
	*DirectX初期化処理
	*********************************************************/
	HRESULT result;

	IDirectInput8 *directInput = nullptr;
	result = DirectInput8Create(
		wc.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void **)&directInput, nullptr
	);
	assert(SUCCEEDED(result));

	// キーボードデバイスの生成
	IDirectInputDevice8 *keyboard = nullptr;
	result =
		directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
	);
	assert(SUCCEEDED(result));

	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// HRESULTはWindows系のエラーコードであり関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
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
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
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
	// SRV用のヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap =
		CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// ImGui専用のSRVヒープ（ImGuiが内部で使うフォント用）
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiSrvDescriptorHeap =
		CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);


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
	ID3D12CommandList *commandLists[] = { commandList.Get() };
	commandQueue.Get()->ExecuteCommandLists(1, commandLists);
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
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
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
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl",
																	  L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl",
																	 L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	assert(pixelShaderBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));


	assert(SUCCEEDED(hr));

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
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(device.Get(), sizeof(VertexData) * 6);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;// 何バイト分かを書く
	// 1頂点当たりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);// 三角形の1頂点当たりのデータサイズ


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

	// マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = CreateBufferResource(device, sizeof(Vector4));

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

	/*********************************************************
	*TransformationMatrix用のResourceを作る
	*********************************************************/

	// WVP用のリソースを作る。Matrix4x41つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource = CreateBufferResource(device.Get(), sizeof(Matrix4x4));
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
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(device.Get(), sizeof(Material));
	Material *materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void **>(&materialDataSprite));

	// 初期設定：色は白、ライティングは無効
	materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialDataSprite->lightingType = false;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(device.Get(), sizeof(VertexData) * 6);
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
	// 左上
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	// 右下
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };

	/*2つ目の三角形
	*********************************************************/

	// 左下
	vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 1.0f,0.0f };
	// 上
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	// 右下
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };

	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(TransformMatrix));
	// データを書き込む
	Matrix4x4 *transformationMatrixDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSprite));
	// 単位行列を書き込んでおく
	*transformationMatrixDataSprite = TransformFunctions::MakeIdentity4x4();

	UINT viewProjectionSize = (sizeof(ViewProjection) + 255) & ~255;
	Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionResource = CreateBufferResource(device.Get(), viewProjectionSize);
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
	CreateSphereMesh(sphereVertices, sphereIndices, 1.0f, 32, 32); // 半径1.0, 縦32, 横32 分割

	// 頂点バッファリソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSphere = CreateBufferResource(device.Get(), sizeof(VertexData) * sphereVertices.size());

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
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSphere = CreateBufferResource(device.Get(), sizeof(uint32_t) * sphereIndices.size());

	//// 頂点データをGPUに書き込む（位置と法線をセット）
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

	uint32_t *mappedIndexData = nullptr;
	indexResourceSphere->Map(0, nullptr, reinterpret_cast<void **>(&mappedIndexData));
	std::memcpy(mappedIndexData, sphereIndices.data(), sizeof(uint32_t) * sphereIndices.size());

	// 球体用の TransformationMatrix のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSphere = CreateBufferResource(device.Get(), sizeof(TransformMatrix));

	// データを書き込むためのポインタ
	TransformMatrix *transformationMatrixDataSphere = nullptr;

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

	/*********************************************************
	*IndexResource(無駄を省いたスプライト)を作る
	*********************************************************/

	// Resource作成
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);

	// View作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite = {};
	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	// 使用吸うrリソースサイズはインデックス5つ分
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	// インデックスリソースにデータを書き込む
	uint32_t *indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void **>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;

	/*********************************************************
	*ModelDataを使う
	*********************************************************/

	// モデル読み込み
	ModelData modelData = LoadObjFile("resources/plane", "plane.obj");
	Log("modelData.vertices.size() = " + std::to_string(modelData.vertices.size()) + "\n");
	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModel = CreateBufferResource(device.Get(), sizeof(VertexData) * modelData.vertices.size());
	// 頂点バッファーリソースを作る
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress();
	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferViewModel.StrideInBytes = sizeof(VertexData);

	// 1. モデルのインデックスデータ用のリソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceModel =
		CreateBufferResource(device.Get(), sizeof(uint32_t) * modelData.indices.size());

	// 2. インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewModel = {};
	indexBufferViewModel.BufferLocation = indexResourceModel->GetGPUVirtualAddress();
	indexBufferViewModel.SizeInBytes = UINT(sizeof(uint32_t) * modelData.indices.size());
	indexBufferViewModel.Format = DXGI_FORMAT_R32_UINT;

	// 3. インデックスリソースにデータを書き込む
	uint32_t *mappedIndexDataModel = nullptr;
	indexResourceModel->Map(0, nullptr, reinterpret_cast<void **>(&mappedIndexDataModel));
	std::memcpy(mappedIndexDataModel, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
	indexResourceModel->Unmap(0, nullptr); // MapしたらUnmapを忘れずに


	// 頂点リソースにデータを書き込む
	VertexData *vertexDataModel = nullptr;
	vertexResourceModel->Map(0, nullptr, reinterpret_cast<void **>(&vertexDataModel));// 書き込むためのアドレスを取得
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModel = CreateBufferResource(device.Get(), sizeof(TransformMatrix));
	TransformMatrix *transformationMatrixDataModel = nullptr;
	transformationMatrixResourceModel->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataModel));


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
	// DX12の初期化
	ImGui_ImplDX12_Init(device.Get(),
						swapChainDesc.BufferCount,
						rtvDesc.Format,
						srvDescriptorHeap.Get(),
						srvDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
						srvDescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart()
	);

	ImGui_ImplDX12_CreateDeviceObjects();

	ID3D12DescriptorHeap *descriptorHeaps[] = {
		srvDescriptorHeap.Get()
	};

	// Textureを読んで転送する
	const int textureCount = 3;
	DirectX::ScratchImage mipImages[textureCount] = { LoadTexture("resources/uvChecker.png"),LoadTexture("resources/monsterBall.png"),LoadTexture(modelData.material.textureFilePath) };
	const DirectX::TexMetadata metadata[textureCount] = { mipImages[0].GetMetadata(),mipImages[1].GetMetadata(),mipImages[2].GetMetadata() };
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource[textureCount] = { CreateTextureResource(device.Get(), metadata[0]),CreateTextureResource(device.Get(), metadata[1]),CreateTextureResource(device.Get(), metadata[2]) };
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource[textureCount] = {
		UploadTextureData(textureResource[0].Get(), mipImages[0], device.Get(), commandList.Get()),
		UploadTextureData(textureResource[1].Get(), mipImages[1], device.Get(), commandList.Get()),
		UploadTextureData(textureResource[2].Get(), mipImages[2], device.Get(), commandList.Get())
	};

	// コマンドを閉じる
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// 実行
	commandQueue.Get()->ExecuteCommandLists(1, commandLists);

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

	// metaDataを基にSRVの設定
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc[textureCount]{};
	for(int i = 0; i < textureCount; ++i) {
		srvDesc[i].Format = metadata[i].format;
		srvDesc[i].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc[i].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc[i].Texture2D.MipLevels = UINT(metadata[i].mipLevels);
	}
	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU[textureCount] = { GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 1),GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2),GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 3) };
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU[textureCount] = { GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 1),GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2),GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 3) };
	for(int i = 0; i < textureCount; ++i) {
		// SRVの生成
		device->CreateShaderResourceView(textureResource[i].Get(), &srvDesc[i], textureSrvHandleCPU[i]);
	}
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

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice *masterVoice;

	// XAudioエンジンのインスタンスを生成
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);

	SoundData soundData1 = SoundLoadWave("resources/Alarm.wav");



	/*********************************************************
	*変数宣言
	*********************************************************/

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

	Transform uvTransformSprite{
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

	materialDataSprite->uvTransform = TransformFunctions::MakeIdentity4x4();

	// 使用するか
	bool isSphere = false;
	bool isModel = true;
	bool isSprite = true;
	bool isSound = false;

	// 音楽を鳴らすための変数
	bool preIsSound = false;

	// 全キーの入力状態を取得する
	BYTE keys[256] = {};
	BYTE preKeys[256] = {};

	MSG msg{};
	// ウィンドウのxボタンが押されるまでループ
	while(msg.message != WM_QUIT) {
		// Windowにメッセージが来てたら最優先で処理させる
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {

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
					ImGui::ColorEdit4("MaterialColor",&materialData->color.x);

					// 光の強さを変える
					ImGui::DragFloat("LightingIntensity", &directionalLightData.intensity, 0.1f, 0.0f, 10.0f);

					// 光の位置を変える
					ImGui::DragFloat3("LightingDirection", &directionalLightData.direction.x, 0.1f, 0.01f, 0.01f);

					ImGui::TreePop();
				}
			}

			/*OBJModel
			*********************************************************/

			// 表示設定
			ImGui::Checkbox("isModel", &isModel);
			if(isModel) {
				if(ImGui::TreeNode("Model")) {

					// transformModel
					ImGui::DragFloat3("Translate", &transformModel.translate.x, 0.01f, -10.0f, 10.0f);
					ImGui::DragFloat3("Scale", &transformModel.scale.x, 0.01f, -10.0f, 10.0f);
					ImGui::DragFloat3("Rotate", &transformModel.rotate.x, 0.01f, -10.0f, 10.0f);

					ImGui::TreePop();
				}
			}

			/*Sprite
			*********************************************************/

			// 表示設定
			ImGui::Checkbox("isSprite", &isSprite);
			if(isSprite) {
				if(ImGui::TreeNode("Sprite")) {

					// transformModel
					ImGui::DragFloat3("Translate", &transformSprite.translate.x, 1.0f, 0.0f, 1280.0f);
					ImGui::DragFloat3("Scale", &transformSprite.scale.x, 0.01f, -10.0f, 10.0f);
					ImGui::DragFloat3("Rotate", &transformSprite.rotate.x, 0.01f, -10.0f, 10.0f);

					// UVトランスフォーム用にこのセクションを追加
					if(ImGui::TreeNode("UV Transform")) {
						ImGui::DragFloat2("UV Translate", &uvTransformSprite.translate.x, 0.01f);
						ImGui::DragFloat("UV Rotate", &uvTransformSprite.rotate.z, 0.01f);
						ImGui::DragFloat2("UV Scale", &uvTransformSprite.scale.x, 0.01f);
						ImGui::ColorEdit3("UV Color", &materialDataSprite->color.x);
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

			/*Sound
			*********************************************************/

			ImGui::Checkbox("isSound", &isSound);

			ImGui::End();

			// カメラの呼び出し
			debugCamera.Update();

			keyboard->Acquire();
			keyboard->GetDeviceState(sizeof(keys), keys);

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

			/*camera用の座標変換
			*********************************************************/

			Matrix4x4 worldMatrix =
				TransformFunctions::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix =
				TransformFunctions::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix =
				debugCamera.GetViewMatrix();
			Matrix4x4 projectionMatrix =
				debugCamera.GetProjectionMatrix();
			Matrix4x4 worldViewProjectionMatrix =
				TransformFunctions::Multiply(worldMatrix, TransformFunctions::Multiply(viewMatrix, projectionMatrix));
			//*transformationMatrixData = worldViewProjectionMatrix;

			/*Sprite用の座標変換
			*********************************************************/

			Matrix4x4 worldMatrixSprite =
				TransformFunctions::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite =
				TransformFunctions::MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite =
				TransformFunctions::MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite =
				TransformFunctions::Multiply(worldMatrixSprite, TransformFunctions::Multiply(viewMatrixSprite, projectionMatrixSprite));
			*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

			/*Sphere用の座標変換
			*********************************************************/

			// 3. ワールド→ビュー→投影をまとめてWVP行列に
			Matrix4x4 worldMatrixSphere
				= TransformFunctions::MakeAffineMatrix(
					transformSphere.scale,
					transformSphere.rotate,
					transformSphere.translate
				);

			Matrix4x4 wvpSphere
				= TransformFunctions::Multiply(
					worldMatrixSphere,
					TransformFunctions::Multiply(viewMatrix, projectionMatrix)
				);


			// 4. 定数バッファに書き込む
			transformationMatrixDataSphere->WVP = wvpSphere;
			transformationMatrixDataSphere->World = worldMatrixSphere;

			/*UVTransformの座標変換
			*********************************************************/

			Matrix4x4 uvTransformMatrix = TransformFunctions::MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = TransformFunctions::Multiply(uvTransformMatrix, TransformFunctions::MakeRoteZMatrix(uvTransformSprite.rotate.z));
			uvTransformMatrix = TransformFunctions::Multiply(uvTransformMatrix, TransformFunctions::MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataSprite->uvTransform = uvTransformMatrix;

			/* モデル用の座標変換
			*********************************************************/

			Matrix4x4 worldMatrixModel =
				TransformFunctions::MakeAffineMatrix(
					transformModel.scale,
					transformModel.rotate,
					transformModel.translate
				);

			Matrix4x4 wvpModel =
				TransformFunctions::Multiply(worldMatrixModel,
											 TransformFunctions::Multiply(viewMatrix, projectionMatrix));

			transformationMatrixDataModel->WVP = wvpModel;
			transformationMatrixDataModel->World = worldMatrixModel;

			/* Soundの再生
			*********************************************************/

			// isSoundがtrueになり、かつ前のフレームではfalseだった場合に再生
			if(isSound && !preIsSound) {
				SoundPlayWave(xAudio2.Get(), soundData1);
			}

			/*********************************************************
			*描画処理
			*********************************************************/

			// 描画コマンド
			commandList->SetDescriptorHeaps(1, descriptorHeaps);
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(graphicsPipelineState.Get());
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());

			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle =
				useMonsterBall
				? textureSrvHandleGPU[1]   // Monster Ball
				: textureSrvHandleGPU[0];

			//SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である
			commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
			commandList->IASetIndexBuffer(&indexBufferViewSphere);
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress());

			viewProjectionData->viewProjectionMatrix = TransformFunctions::Multiply(viewMatrix, projectionMatrix);
			viewProjectionData->cameraPosition = cameraTransform.translate;
			viewProjectionData->padding = 0.0f;

			commandList->SetGraphicsRootConstantBufferView(3, viewProjectionResource->GetGPUVirtualAddress());

			// ライトの変更内容をGPUに書き戻す
			*mappedDirectionalLightData = directionalLightData;

			/*********************************************************
			*球の描画
			*********************************************************/

			if(isSphere) {
				// 1. 球用のリソース（マテリアル、テクスチャ、座標変換行列）を設定
				commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
				D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = useMonsterBall ? textureSrvHandleGPU[1] : textureSrvHandleGPU[0];
				commandList->SetGraphicsRootDescriptorTable(2, srvHandle);
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress());

				// 2. 球の頂点・インデックスバッファを設定
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
				commandList->IASetIndexBuffer(&indexBufferViewSphere);

				// 3. 球を描画
				commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
			}

			/*********************************************************
			*モデルの描画
			*********************************************************/

			if(isModel) {
				// 1. モデル用のリソース（マテリアル、テクスチャ、座標変換行列）を設定
				// ※ materialResource は現在、球とモデルで共通のものを使っています。モデル専用のマテリアルを作ることもできます。
				commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU[2]); // modelDataから読み込んだテクスチャ
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModel->GetGPUVirtualAddress());

				// 2. モデルの頂点・インデックスバッファを設定
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewModel);
				commandList->IASetIndexBuffer(&indexBufferViewModel);

				// 3. モデルを描画
				commandList->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
			}

			/*********************************************************
			*スプライトの描画
			*********************************************************/

			if(isSprite) {
				// 1. スプライト用のリソースを設定
				commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress()); // マテリアル
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress()); // 座標変換行列
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU[0]); // テクスチャ（ここではuvChecker.pngを使用）

				// 2. スプライトの頂点・インデックスバッファを設定
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
				commandList->IASetIndexBuffer(&indexBufferViewSprite);

				// 3. スプライトを描画（インデックス6個）
				commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
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
			commandQueue.Get()->ExecuteCommandLists(1, commandLists);

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

	xAudio2.Reset();

	SoundUnload(&soundData1);

	// MapしたリソースをすべてUnmapする
	/*if(vertexResource) {
		vertexResource->Unmap(0, nullptr);
	}*/
	if(wvpResource) {
		wvpResource->Unmap(0, nullptr);
	}
	if(materialResource) {
		materialResource->Unmap(0, nullptr);
	}
	if(vertexResourceSprite) {
		vertexResourceSprite->Unmap(0, nullptr);
	}
	if(transformationMatrixResourceSprite) {
		transformationMatrixResourceSprite->Unmap(0, nullptr);
	}
	if(directionalLightResource) {
		directionalLightResource->Unmap(0, nullptr);
	}
	if(materialResourceSprite) {
		materialResourceSprite->Unmap(0, nullptr);
	}
	if(viewProjectionResource) {
		viewProjectionResource->Unmap(0, nullptr);
	}
	if(transformationMatrixResourceModel) {
		transformationMatrixResourceModel->Unmap(0, nullptr);
	}
	if(transformationMatrixResourceSphere) {
		transformationMatrixResourceSphere->Unmap(0, nullptr);
	}
	if(vertexResourceModel) {
		vertexResourceModel->Unmap(0, nullptr);
	}
	if(vertexResourceSphere) {
		vertexResourceSphere->Unmap(0, nullptr);
	}
	if(indexResourceSphere) {
		indexResourceSphere->Unmap(0, nullptr);
	}
	if(indexResourceSprite) {
		indexResourceSprite->Unmap(0, nullptr);
	}

	/*********************************************************
	*DirectX12のオブジェクト解放処理
	*********************************************************/

	// ======== 各種解放 ============
	// 順序に注意（ImGuiより先に使ってる srv/dsvHeap は後）
	CloseHandle(fenceEvent);

	// ======== COMの終了処理 ============
	CoUninitialize();

	// ======== ImGui解放 ============
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// ======== Liveオブジェクトチェック ============ ←ここに移動
	/*Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
	if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		Log("Reporting Live Objects...\n");
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		HRESULT hr_report = debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		Log(std::format("ReportLiveObjects() returned: 0x{:08X}\n", static_cast<uint32_t>(hr_report)));
	}*/


	return 0;
}