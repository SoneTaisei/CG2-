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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	/* メッセージに応じてゲーム固有の処理を行う*/
	switch(msg) {
		/* ウィンドウが破壊された*/
	case WM_DESTROY:
		/* OSに応じて、アプリ固有の終了を伝える*/
		PostQuitMessage(0);
		return 0;
	}

	/* 標準のメッセージ処理を行う*/
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

/* windowsアプリでのエントリーポイント(main関数)*/
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WNDCLASS wc{};
	/* ウィンドウプロシージャ*/
	wc.lpfnWndProc = WindowProc;
	/* ウィンドウクラス名*/
	wc.lpszClassName = L"MyDreamGame";
	/* インスタンスハンドル*/
	wc.hInstance = GetModuleHandle(nullptr);
	/* カーソル*/
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	/* ウィンドウクラスを登録する*/
	RegisterClass(&wc);

	/* 出力ウィンドウの文字出力*/
	OutputDebugStringA("Hello,DirectX\n");

	/* クライアント領域サイズ*/
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	/* ウィンドウサイズを表す構造体にクライアントを入れる*/
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	/* クライアント領域をもとに実際のサイズにwrcを変更してもらう*/
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	/* ログのディレクトリ用意*/
	std::filesystem::create_directories("logs");

	/* 現在の時刻を取得*/
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	/* ログファイルの名前にコンマ何秒はいらないので、削って秒にする*/
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	/* 日本時間（PCの設定時間）に変換*/
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
	/* formatを使って年月日_時分秒の文字列に変換*/
	std::string dateString = std::format("{:%Y%%d_%H%M%S}", localTime);
	/* 時刻を使ってファイルを決定*/
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	/* ファイルを作って書き込み準備*/
	std::ofstream logStream(logFilePath);

	/* ウィンドウの生成*/
	HWND hwnd = CreateWindow(
		wc.lpszClassName,    /*使用するクラス名*/
		L"MyDreamGame",      /*タイトルバーの文字（なんでもいい）*/
		WS_OVERLAPPEDWINDOW, /*よく見るウィンドウスタイル*/
		CW_USEDEFAULT,       /*表示X座標（Windowsに任せる）*/
		CW_USEDEFAULT,       /*表示X座標（WindowsOSに任せる）*/
		wrc.right - wrc.left,/*ウィンドウ横幅*/
		wrc.bottom - wrc.top,/*ウィンドウ縦幅*/
		nullptr,             /*親ウィンドウハンドル*/
		nullptr,             /*メニューハンドル*/
		wc.hInstance,        /*インスタンスハンドル*/
		nullptr);            /*オプション*/

	/* ウィンドウを表示する*/
	ShowWindow(hwnd, SW_SHOW);

	/* DXGIファクトリーの生成*/
	IDXGIFactory7 *dxgiFactory = nullptr;
	/* HRESULTはWindows系のエラーコードであり関数が成功したかどうかをSUCCEEDEDマクロで判定できる*/
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	/*エラーが出たらassert*/
	assert(SUCCEEDED(hr));

	/* 使用するアダプタ用の変数*/
	IDXGIAdapter4 *useAdapter = nullptr;
	/* いいアダプターを選択させる*/
	for(UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference
	(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		/* アダプター情報を取得*/
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		/* ソフトウェアアダプタでなければ採用*/
		if(!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			/* 採用したアダプタの情報を*/
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	ID3D12Device *device = nullptr;
	/* 機能レベルとログ出力用の文字列*/
	D3D_FEATURE_LEVEL featureLevels[] = {
	  D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char *featureLevelStrings[] = { "12.2","12.1","12.0" };
	/* 高い順に生成できるか試していく*/
	for(size_t i = 0; i < _countof(featureLevels); ++i) {
		/* 採用したアダプターデバイスを生成*/
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		/* 指定した機能レベルでデバイスが生成できたかを確認*/
		if(SUCCEEDED(hr)) {
			/* 生成できたのでログ出力を行ってループを抜ける*/
			Log(std::format("FeatureLevel : {}\n", std::string(featureLevelStrings[i])));

			break;
		}
	}
	/* デバイスの生成がうまくいかなかったので起動できない*/
	assert(device != nullptr);
	Log("Complete create D3D12Devie!!!\n");

	MSG msg{};
	/* ウィンドウのxボタンが押されるまでループ*/
	while(msg.message != WM_QUIT) {
		/* Windowにメッセージが来てたら最優先で処理させる*/
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			/* ゲームの処理*/

		}
	}

	return 0;
}