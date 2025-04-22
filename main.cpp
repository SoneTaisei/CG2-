#include <Windows.h>
#include<format>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <chrono>

#include <d3d12.h>
#pragma comment(lib,"d3d12.lib")
#include <Dxgi1_6.h>

#pragma comment(lib,"dxgi.lib")
#include <cassert>

// debug用の処理を作るときに使う呼び出し
#include <dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")

// StringCcPrintfWを使うときに作る呼び出し
#include<strsafe.h>

// 最後の警告を作るときに使う呼び出し
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#include<dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

/*********************************************************
*構造体
*********************************************************/

struct Vector4 {
  float x;
  float y;
  float z;
  float w;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  // メッセージに応じてゲーム固有の処理を行う
  switch(msg) {
    // ウィンドウが破壊された
  case WM_DESTROY:
    // OSに応じて、アプリ固有の終了を伝える
    PostQuitMessage(0);
    return 0;
  }

  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string &message) {
  OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string &str) {
  if(str.empty()) {
    return std::wstring();
  }

  auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]), static_cast<int>(str.size()), NULL, 0);
  if(sizeNeeded == 0) {
    return std::wstring();
  }
  std::wstring result(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
  return result;
}

std::string ConvertString(const std::wstring &str) {
  if(str.empty()) {
    return std::string();
  }

  auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
  if(sizeNeeded == 0) {
    return std::string();
  }
  std::string result(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
  return result;
}

std::string str0{ "STRING" };

std::string str1{ std::to_string(10) };

static LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {
  // 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
  SYSTEMTIME time;
  GetLocalTime(&time);
  wchar_t filePath[MAX_PATH] = { 0 };
  CreateDirectory(L"./Dumps", nullptr);
  StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
  HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
  // processId(exeID)とクラッシュ(例外)の発生したthreadIDを取得
  DWORD processId = GetCurrentProcessId();
  DWORD threadId = GetCurrentThreadId();
  // 設定情報を入力
  MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
  minidumpInformation.ThreadId = threadId;
  minidumpInformation.ExceptionPointers = exception;
  minidumpInformation.ClientPointers = TRUE;
  // Dumpを出力。MiniDumNormalは最低限の情報を出力するフラグ
  MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
  // ほかに関連付けられているSEH例外ハンドラがあれば実行。通常プロセスを終了する。
  return EXCEPTION_EXECUTE_HANDLER;
}

IDxcBlob *CompileShader(
  // CompilerするShaderファイルへのパス
  const std::wstring &filePath,
  // Compilerに使用するProfile
  const wchar_t *profile,
  // 初期化で生成したものを3つ
  IDxcUtils *dxcUtils,
  IDxcCompiler3 *dxcCompiler,
  IDxcIncludeHandler *includeHandler) {

  /*********************************************************
  *1. hlslファイルを読む
  *********************************************************/

  // これからシェーダーをコンパイルする旨をログに出す
  Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
  // hlslファイルを読む
  IDxcBlobEncoding *shaderSource = nullptr;
  HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
  // あきらめなかったら止める
  assert(SUCCEEDED(hr));
  // 読み込んだファイルの内容を設定する
  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8;// UTF8の文字コードであることを通知

  /*********************************************************
  *2.Compileする
  *********************************************************/

  LPCWSTR arguments[] = {
    filePath.c_str(),// コンパイル対象のhlslファイル名
    L"-E",L"main",// エントリーポイントの指定。基本的にmain以外にはしない
    L"-T",profile,// ShaderProfileの設定
    L"-Zi",L"-Qembed_debug",// デバッグ用の情報を埋め込む
    L"-Od",// 最適化を外しておく
    L"-Zpr",// メモリレイアウトは行優先
  };
  // 実際にShaderをコンパイルする
  IDxcResult *shaderResult = nullptr;
  hr = dxcCompiler->Compile(
    &shaderSourceBuffer,
    arguments,
    _countof(arguments),
    includeHandler,
    IID_PPV_ARGS(&shaderResult)
  );
  // コンパイラエラーではなくdxcが起動できないなどの致命的なエラー
  assert(SUCCEEDED(hr));

  /*********************************************************
  *3.警告・エラーが出ていないか確認
  *********************************************************/

  IDxcBlobUtf8 *shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if(shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());
    //警告・エラー絶対ダメ
    assert(false);
  }

  /*********************************************************
  *4.Compile結果を受け取って返す
  *********************************************************/

  // コンパイル結果から実行用のバイナリ部分を取得
  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
  assert(SUCCEEDED(hr));
  // 成功押したログを出す
  Log(ConvertString(std::format(L"Compile Succeeded, path:{},profile:{}\n", filePath, profile)));
  // もう使わないリソースを開放
  shaderSource->Release();
  shaderResult->Release();
  // 実行用のバイナリを返却
  return shaderBlob;
}

// windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  // 予期せぬ事態(Unhandled)に補足する関数を登録
  SetUnhandledExceptionFilter(ExportDump);

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
  std::string dateString = std::format("{:%Y%%d_%H%M%S}", localTime);
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
#endif

  // ウィンドウを表示する
  ShowWindow(hwnd, SW_SHOW);

  // DXGIファクトリーの生成
  IDXGIFactory7 *dxgiFactory = nullptr;
  // HRESULTはWindows系のエラーコードであり関数が成功したかどうかをSUCCEEDEDマクロで判定できる
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
  //エラーが出たらassert
  assert(SUCCEEDED(hr));

  // 使用するアダプタ用の変数
  IDXGIAdapter4 *useAdapter = nullptr;
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

  ID3D12Device *device = nullptr;
  // 機能レベルとログ出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
  };
  const char *featureLevelStrings[] = { "12.2","12.1","12.0" };
  // 高い順に生成できるか試していく
  for(size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプターデバイスを生成
    hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
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
  ========================================================*/
  ID3D12InfoQueue *infoQueue = nullptr;
  if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // 一番危険なエラーの時にとまる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    // エラーの時にとまる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    // 警告時にとまる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    // 解放
    infoQueue->Release();
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
  *画面を青くする
  *********************************************************/

  // コマンドキューと生成する
  ID3D12CommandQueue *commandQueue = nullptr;
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
  // コマンドキューの生成がうまくいかなかったら起動できない
  assert(SUCCEEDED(hr));

  // コマンドアロケータを生成する
  ID3D12CommandAllocator *commandAllocator = nullptr;
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
  // コマンドアロケータの生成がうまくいかなかったら生成できない
  assert(SUCCEEDED(hr));

  // コマンドリストを生成する
  ID3D12GraphicsCommandList *commandList = nullptr;
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
  // コマンドリストの生成がうまくいかないと起動できない
  assert(SUCCEEDED(hr));

  // スワップチェーンを生成する
  IDXGISwapChain4 *swapChain = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.Width = kClientWidth;
  swapChainDesc.Height = kClientHeight;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// ウィンドウの色
  swapChainDesc.SampleDesc.Count = 1;// マルチサンプル市内
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// 描画のターゲットとして利用する
  swapChainDesc.BufferCount = 2;// ダブルバッファ
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;// モニタに移ったら中身を破棄
  // コマンドキュー、ウィンドウハンドルの設定を渡して生成する
  hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1 **>(&swapChain));
  assert(SUCCEEDED(hr));

  // ディスクリプタヒープの生成
  ID3D12DescriptorHeap *rtvDescriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
  rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;// レンダーターゲットビュー用
  rtvDescriptorHeapDesc.NumDescriptors = 2;// ダブルバッファように
  hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
  // ディスクリプタヒープが作れなかったので起動できない
  assert(SUCCEEDED(hr));

  // SwapChainからResourceを引っ張ってくる
  ID3D12Resource *swapChainResources[2] = { nullptr };
  for(int i = 0; i < 2; ++i) {
    hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
    // うまく取得できなければ起動できない
    assert(SUCCEEDED(hr));
  }

  // RTVの設定
  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;// 出力結果をSRGBに変換して書き込む
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;// 2Dテクスチャとして書き込む
  // ディスクリプタの先頭を取得
  D3D12_CPU_DESCRIPTOR_HANDLE rtvStarHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  // RTVを2つ作るのでディスクリプタを2つ用意
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
  // まず1つ目をつくる。1つ目は最初のところに作る。作る場所をこちらで指定しないといけない
  rtvHandles[0] = rtvStarHandle;
  device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
  // 2つ目のディスクリプタハンドルを得る
  rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  // 2つ目を作る
  device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

  // これから書き込むバックバッファのインデックスを取得
  UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
  // TransitionBarrierの設定
  D3D12_RESOURCE_BARRIER barrier{};
  // 今回のバリアはTransition
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  // Noneにしておく
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  // バリアを張る対象のリソース。現在のバックバッファに対して行う
  barrier.Transition.pResource = swapChainResources[backBufferIndex];
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
  ID3D12Fence *fence = nullptr;
  uint64_t fenceValue = 0;
  hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
  assert(SUCCEEDED(hr));

  // FenceのSignalを待つためのイベントを作成する
  HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  // dxcCompilerを初期化
  IDxcUtils *dxcUtils = nullptr;
  IDxcCompiler3 *dxcCompiler = nullptr;
  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  assert(SUCCEEDED(hr));

  // 現時点でincludeはしないが、includeに対応するための設定を行っておく
  IDxcIncludeHandler *includeHandler = nullptr;
  hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
  assert(SUCCEEDED(hr));

  // FenceEventをassertする
  assert(fenceEvent != nullptr);

  // GPUにコマンドリストの実行を行わせる
  ID3D12CommandList *commandLists[] = { commandList };
  commandQueue->ExecuteCommandLists(1, commandLists);
  // GPUとOSに画面の交換を行うように通知する
  swapChain->Present(1, 0);
  // Fenceの値を更新
  fenceValue++;
  // GPUがここまでたどり着いたときに、Fenceの値を指定した値を代入するようにSignalを送る
  commandQueue->Signal(fence, fenceValue);
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
  hr = commandList->Reset(commandAllocator, nullptr);
  assert(SUCCEEDED(hr));

  /*********************************************************
  *PSOを生成する
  *********************************************************/

  // RootSignature作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  // シリアライズしてバイナリする
  ID3DBlob *signatureBlob = nullptr;
  ID3DBlob *errorBlob = nullptr;
  hr = D3D12SerializeRootSignature(&descriptionRootSignature,
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

  // InputLayout
  D3D12_INPUT_ELEMENT_DESC inputElementDess[1] = {};
  inputElementDess[0].SemanticName = "POSITION";
  inputElementDess[0].SemanticIndex = 0;
  inputElementDess[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDess[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = inputElementDess;
  inputLayoutDesc.NumElements = _countof(inputElementDess);

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
  // 利用するとぽ路地のタイプ。三角形を選択
  graphicsPipelineStateDesc.PrimitiveTopologyType =
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // どのように画面に色を打ち込むかの設定(気にしなくていい)
  graphicsPipelineStateDesc.SampleDesc.Count = 1;
  graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  // 実際に生成
  ID3D12PipelineState *graphicsPipelineState = nullptr;
  hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
    IID_PPV_ARGS(&graphicsPipelineState));
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
  vertexResourceDesc.Width = sizeof(Vector4) * 3;
  // バッファの場合はこれらは1にする決まり
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  vertexResourceDesc.SampleDesc.Count = 1;
  // バッファの場合はこれにする決まり
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  // 実際に頂点リソースを作る
  ID3D12Resource *vertexResource = nullptr;
  hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
    &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
    IID_PPV_ARGS(&vertexResource));
  assert(SUCCEEDED(hr));

  // 頂点バッファビューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  // リソースの先頭アドレスから使う
  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点3つ分のサイズ
  vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
  // 1頂点当たりのサイズ
  vertexBufferView.StrideInBytes = sizeof(Vector4);

  // 頂点リソースにデータを書き込む
  Vector4 *vertexData = nullptr;
  // 書き込むためのアドレスを取得
  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
  // 左下
  vertexData[0] = { -0.5f,-0.5f,0.0f,1.0f };
  // 上
  vertexData[1] = { 0.0f,0.5f,0.0f,1.0f };
  // 右下
  vertexData[2] = { 0.5f,-0.5f,0.0f,1.0f };

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


  MSG msg{};
  // ウィンドウのxボタンが押されるまでループ
  while(msg.message != WM_QUIT) {
    // Windowにメッセージが来てたら最優先で処理させる
    if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      // ゲームの処理
      backBufferIndex = swapChain->GetCurrentBackBufferIndex();

      // バリア Present → RenderTarget
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.pResource = swapChainResources[backBufferIndex];
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
      commandList->ResourceBarrier(1, &barrier);

      // 描画先セット＆クリア
      commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], FALSE, nullptr);
      float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
      commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

      // 描画コマンド
      commandList->RSSetViewports(1, &viewport);
      commandList->RSSetScissorRects(1, &scissorRect);
      commandList->SetGraphicsRootSignature(rootSignature);
      commandList->SetPipelineState(graphicsPipelineState);
      commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      commandList->DrawInstanced(3, 1, 0, 0);

      // バリア RenderTarget → Present
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
      commandList->ResourceBarrier(1, &barrier);

      // 1. コマンドリストを閉じる
      hr = commandList->Close();
      assert(SUCCEEDED(hr));

      // 2. コマンドリストを実行
      ID3D12CommandList *commandLists[] = { commandList };
      commandQueue->ExecuteCommandLists(1, commandLists);

      // 3. 画面を表示
      swapChain->Present(1, 0);

      // --- GPUが終わるのを待つ ←ここ追加 ---
      fenceValue++;
      commandQueue->Signal(fence, fenceValue);
      if(fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
      }

      // 4. 次のフレームに向けて準備（リセット）
      commandAllocator->Reset();
      commandList->Reset(commandAllocator, nullptr);
    }
  }

  /*********************************************************
  *DirectX12のオブジェクト解放処理
  *********************************************************/

  CloseHandle(fenceEvent);
  fence->Release();
  rtvDescriptorHeap->Release();
  swapChainResources[0]->Release();
  swapChainResources[1]->Release();
  swapChain->Release();
  commandList->Release();
  commandAllocator->Release();
  commandQueue->Release();
  device->Release();
  useAdapter->Release();
  dxgiFactory->Release();
#ifdef _DEBUG
  debugController->Release();
#endif // _DEBUG
  CloseWindow(hwnd);

  // DirectX12のオブジェクト
  vertexResource->Release();
  graphicsPipelineState->Release();
  signatureBlob->Release();
  if(errorBlob) {
    errorBlob->Release();
  }
  rootSignature->Release();
  pixelShaderBlob->Release();
  vertexShaderBlob->Release();

  /*********************************************************
  *リソースが残っていないかのチェックをする
  *********************************************************/

  IDXGIDebug1 *debug;
  if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
    debug->Release();
  }

  return 0;
}