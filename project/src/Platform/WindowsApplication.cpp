#include "Platform/WindowsApplication.h"
#include "Utility/Utilityfunctions.h" // LogやConvertStringなど
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

// ImGuiの外部リンケージ
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


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
    CreateDxInstance();


    // キーボードとコントローラーの初期化
    KeyboardInput::GetInstance()->Initialize(wc_.hInstance, hwnd_);
    GamepadInput::GetInstance()->Initialize(wc_.hInstance, hwnd_);

    // スワップチェーンの生成とRTVの作成
    CreateFinalRenderTargets();

    // SpriteクラスとModelクラスの静的初期化
    Model::StaticInitialize(device_.Get());
    TextureManager::GetInstance()->Initialize(device_);

    // ImGuiの初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX12_Init(device_.Get(),
                        swapChainDesc_.BufferCount, // ここは元のコードのswapChainDescを参照する必要がある
                        rtvDesc_.Format, // 同上
                        TextureManager::GetInstance()->GetSrvDescriptorHeap(),
                        TextureManager::GetInstance()->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
                        TextureManager::GetInstance()->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart()
    );

    // 音声の初期化
    AudioManager::Initialize();

    // フェンスの初期化
    HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    assert(SUCCEEDED(hr));

    fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent_ != nullptr);
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
            // --- 更新処理 (Update) ---
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // (ここにImGuiのUI記述やゲームの更新処理)
            ImGui::ShowDemoWindow();


            // --- 描画処理 (Draw) ---
            // 描画前準備
            UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            commandList_->ResourceBarrier(1, &barrier);

            // 描画先のRTVを設定
            commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);

            //画面クリア
            float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
            commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

            // (ここにゲームの描画コマンド)

            // ImGuiの描画
            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());

            // 描画後処理
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            commandList_->ResourceBarrier(1, &barrier);

            // コマンドリストを確定させて実行
            HRESULT hr = commandList_->Close();
            assert(SUCCEEDED(hr));
            ID3D12CommandList *commandLists[] = { commandList_.Get() };
            commandQueue_->ExecuteCommandLists(1, commandLists);

            // 画面表示
            swapChain_->Present(1, 0);

            // GPUの完了を待つ
            fenceValue_++;
            commandQueue_->Signal(fence_.Get(), fenceValue_);
            if(fence_->GetCompletedValue() < fenceValue_) {
                fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
                WaitForSingleObject(fenceEvent_, INFINITE);
            }

            // 次のフレームの準備
            hr = commandAllocator_->Reset();
            assert(SUCCEEDED(hr));
            hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
            assert(SUCCEEDED(hr));
        }
    }
}

void WindowsApplication::Finalize() {
    // ImGui解放
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // その他マネージャクラスの解放
    AudioManager::Finalize();
    TextureManager::GetInstance()->Finalize();

    // フェンスイベントのハンドルを閉じる
    CloseHandle(fenceEvent_);

    // COMの終了処理
    CoUninitialize();
}

void WindowsApplication::CreateDxInstance() {
    HRESULT hr;
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    assert(SUCCEEDED(hr));

    // アダプタの選別など... (main.cppから移植)
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter;
    for(UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));
        if(!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
            break;
        }
        useAdapter = nullptr;
    }
    assert(useAdapter != nullptr);

    // デバイス生成
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
    for(size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
        if(SUCCEEDED(hr)) {
            break;
        }
    }
    assert(device_ != nullptr);

    // コマンドキュー、アロケータ、リストの生成... (main.cppから移植)
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
    assert(SUCCEEDED(hr));
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
    assert(SUCCEEDED(hr));
    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
    assert(SUCCEEDED(hr));
}

void WindowsApplication::CreateFinalRenderTargets() {
    HRESULT hr;
    // スワップチェーンの生成
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = kWindowWidth_;
    swapChainDesc.Height = kWindowHeight_;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc_ = swapChainDesc;

    hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), hwnd_, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1 **>(swapChain_.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // RTV用ディスクリプタヒープの生成
    rtvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

    // RTVの作成
    for(int i = 0; i < 2; ++i) {
        hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
        assert(SUCCEEDED(hr));
        rtvHandles_[i] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), i);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
        rtvDesc_ = rtvDesc;
    }
}