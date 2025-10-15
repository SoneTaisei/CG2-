#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

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

    // フェンス
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    uint64_t fenceValue_ = 0;
    HANDLE fenceEvent_;

    // RTV
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
};

