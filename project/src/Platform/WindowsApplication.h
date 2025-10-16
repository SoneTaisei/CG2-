#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Scene/SceneManager.h"
#include <memory>
#include "Graphics/DebugCamera.h" 
#include "Utility/Utilityfunctions.h"

class WindowsApplication {
public:
    // ウィンドウプロシージャ
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    // 定数
    static const int kWindowWidth_ = 1280;
    static const int kWindowHeight_ = 720;

public:
    void Initialize();
    void Run();
    void Finalize();

private:
    void CreateDxInstance();
    void CreateFinalRenderTargets();
    void CreatePipelines();

private:
    HWND hwnd_ = nullptr;
    WNDCLASS wc_{};

    // DirectX関連
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> spritePipelineState_;
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

    // フェンス
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    uint64_t fenceValue_ = 0;
    HANDLE fenceEvent_;

    // RTV
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];

    std::unique_ptr<SceneManager> sceneManager_;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_;
    // ViewProjection用のリソースとデータ
    Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionResource_;
    ViewProjection *viewProjectionData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight *directionalLightData_ = nullptr;
};

