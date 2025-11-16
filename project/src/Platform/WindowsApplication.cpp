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

    // SpriteクラスとModelクラスの静的初期化
    Model::StaticInitialize(device);
    TextureManager::GetInstance()->Initialize(device);

    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandles;
    for(size_t i = 0; i < TextureManager::GetInstance()->GetTextureCount(); ++i) {
        textureHandles.push_back(TextureManager::GetInstance()->GetGpuHandle(static_cast<uint32_t>(i)));
    }
    Sprite::StaticInitialize(device, kWindowWidth_, kWindowHeight_, textureHandles);

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

     // SceneManagerの生成と初期化
    sceneManager_ = std::make_unique<SceneManager>();
    sceneManager_->Initialize(commandList);

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

    // システムタイマーの分解能を上げる
    timeBeginPeriod(1);

    // TimeManager を初期化
    //TimeManager::GetInstance().Initialize();

#ifdef _DEBUG
// リソースリークチェッカーのインスタンスを作成
    leakChecker_ = std::make_unique<D3DResourceLeakChecker>();
#endif
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
