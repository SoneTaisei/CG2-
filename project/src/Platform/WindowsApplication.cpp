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

    CreatePipelines();

    // ViewProjectionリソースの作成
    UINT viewProjectionSize = (sizeof(ViewProjection) + 255) & ~255;
    viewProjectionResource_ = CreateBufferResource(device_.Get(), viewProjectionSize);
    viewProjectionResource_->Map(0, nullptr, reinterpret_cast<void **>(&viewProjectionData_));

    const UINT directionalLightBufferSize = (sizeof(DirectionalLight) + 255) & ~255u;
    directionalLightResource_ = CreateBufferResource(device_.Get(), directionalLightBufferSize);
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
    sceneManager_->Initialize(commandList_);

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

    const UINT materialBufferSize = (sizeof(Material) + 255) & ~255u;
    materialResource = CreateBufferResource(device_.Get(), materialBufferSize);
    materialData = nullptr;
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData->lightingType = 0;
    materialResource->Unmap(0, nullptr);

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
            // 描画前準備
            UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            commandList_->ResourceBarrier(1, &barrier);

           // 描画先のRTVとDSVを設定
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
            commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

            //画面クリア
            float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
            commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

            ID3D12DescriptorHeap *pHeaps[] = { TextureManager::GetInstance()->GetSrvDescriptorHeap() };
            commandList_->SetDescriptorHeaps(1, pHeaps);
            commandList_->SetGraphicsRootSignature(rootSignature_.Get());
            commandList_->SetPipelineState(graphicsPipelineState_.Get()); // まず3D用をデフォルトに
            commandList_->RSSetViewports(1, &viewport_);
            commandList_->RSSetScissorRects(1, &scissorRect_);
            commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList_->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
            commandList_->SetGraphicsRootConstantBufferView(3, viewProjectionResource_->GetGPUVirtualAddress());
            commandList_->SetGraphicsRootConstantBufferView(4, directionalLightResource_->GetGPUVirtualAddress());

            // (ここにゲームの描画コマンド)
            // --- 描画処理 (Draw) ---
            sceneManager_->Draw(viewProjectionData_->viewProjectionMatrix);

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

    debugCamera_.reset();

    sceneManager_.reset();

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

void WindowsApplication::CreatePipelines() {
    HRESULT hr;

    // dxcCompilerを初期化
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    assert(SUCCEEDED(hr));

    // --------------------------------------------------------------------
    // 1. RootSignature の作成 (シェーダーとデータのやり取りのルール決め)
    // --------------------------------------------------------------------
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 3; // t0
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[5] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // b0 : for Material
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; // b0 : for WVP
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Constants.Num32BitValues = sizeof(TransformMatrix) / 4;
    rootParameters[1].Constants.ShaderRegister = 0;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // t0 : for Texture
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // b1 : for ViewProjection
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // Vertex Shaderで使う
    rootParameters[3].Descriptor.ShaderRegister = 1;

    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // b2 : for DirectionalLight
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].Descriptor.ShaderRegister = 1; // レジスタ番号をずらす

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // ※注意：main.cppのrootParametersにはViewProjectionやLightも含まれていましたが、
    // SceneManagerで描画する設計の場合、これらは描画時に外部から設定する方が一般的です。
    // ここでは、ModelのDrawに必要な最低限のパラメータで構成し直しています。
    // もし固定で使いたい場合は、main.cppの定義に戻してください。

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MinLOD = 0;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0; // s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if(FAILED(hr)) {
        Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));


    // --------------------------------------------------------------------
    // 2. パイプラインステートオブジェクト(PSO)の作成 (描画ルールの結合)
    // --------------------------------------------------------------------
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
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // シェーダーのコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"shaders/Object3D.VS.hlsl", L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
    assert(vertexShaderBlob != nullptr);
    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"shaders/Object3D.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
    assert(pixelShaderBlob != nullptr);

    // PSOのデスクリプション
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // 深度設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 3Dモデル用のPSOを作成
    hr = device_->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
    assert(SUCCEEDED(hr));

    // スプライト用のPSOを作成（深度テストを無効にする）
    D3D12_GRAPHICS_PIPELINE_STATE_DESC spritePipelineStateDesc = graphicsPipelineStateDesc;
    spritePipelineStateDesc.DepthStencilState.DepthEnable = false;
    hr = device_->CreateGraphicsPipelineState(&spritePipelineStateDesc, IID_PPV_ARGS(&spritePipelineState_));
    assert(SUCCEEDED(hr));

    // --------------------------------------------------------------------
    // 3. 描画領域と深度バッファの設定
    // --------------------------------------------------------------------
    // 深度バッファのリソース作成
    depthStencilResource_ = CreateDepthStencilTextureResource(device_.Get(), kWindowWidth_, kWindowHeight_);
    // DSV用のディスクリプタヒープ作成
    dsvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
    // DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    // DSVを作成
    device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    // ビューポート設定
    viewport_ = {};
    viewport_.Width = kWindowWidth_;
    viewport_.Height = kWindowHeight_;
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    // シザー矩形設定
    scissorRect_ = {};
    scissorRect_.left = 0;
    scissorRect_.right = kWindowWidth_;
    scissorRect_.top = 0;
    scissorRect_.bottom = kWindowHeight_;
}
