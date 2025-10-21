#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <d3dcompiler.h>

class SpriteCommon {
public:
	void Initialize();
private:
	// ルートシグネチャの作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> spritePipelineState = nullptr;

private:
	// ルートシグネチャの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void CreateGraphicsPipeline();
};

