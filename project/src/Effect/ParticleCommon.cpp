#include "ParticleCommon.h"
#include <cassert>
#include <format>
#include <dxcapi.h>
#include "Graphics/TextureManager.h"

#pragma comment(lib, "dxcompiler.lib")

void ParticleCommon::Initialize(ID3D12Device *device) {
    assert(device);
    device_ = device;

    CreateRootSignature();
    CreatePipelineState();
    CreateMesh();
}

void ParticleCommon::PreDraw(ID3D12GraphicsCommandList *commandList) {
    assert(commandList);
    commandList_ = commandList;

    // パイプラインステートの設定
    commandList_->SetGraphicsRootSignature(rootSignature_.Get());
    commandList_->SetPipelineState(pipelineState_.Get());
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // DescriptorHeapの設定 (TextureManagerから借りる)
    ID3D12DescriptorHeap *descriptorHeaps[] = { TextureManager::GetInstance()->GetSrvDescriptorHeap() };
    commandList_->SetDescriptorHeaps(1, descriptorHeaps);
}

void ParticleCommon::CreateRootSignature() {
    // WindowsApplication.cpp に書いてあった RootSignature 作成コードをここに移動
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_ROOT_PARAMETER rootParameters[4] = {};

    // [0] Material (CBV b0)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // [1] Instancing Data (DescriptorTable t0)
    D3D12_DESCRIPTOR_RANGE descriptorRangeInstancing[1] = {};
    descriptorRangeInstancing[0].BaseShaderRegister = 0; // t0
    descriptorRangeInstancing[0].NumDescriptors = 1;
    descriptorRangeInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeInstancing;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

    // [2] Texture (DescriptorTable t3)
    D3D12_DESCRIPTOR_RANGE descriptorRangeTexture[1] = {};
    descriptorRangeTexture[0].BaseShaderRegister = 3; // t3
    descriptorRangeTexture[0].NumDescriptors = 1;
    descriptorRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeTexture;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

    // [3] DirectionalLight (CBV b1)
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if(errorBlob) {
        OutputDebugStringA((char *)errorBlob->GetBufferPointer());
        assert(false);
    }
    device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
}

void ParticleCommon::CreatePipelineState() {
    // シェーダーコンパイルとPSO作成 (WindowsApplication.cppの内容を移植)
    IDxcUtils *dxcUtils = nullptr;
    IDxcCompiler3 *dxcCompiler = nullptr;
    IDxcIncludeHandler *includeHandler = nullptr;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    dxcUtils->CreateDefaultIncludeHandler(&includeHandler);

    auto CompileShader = [&](const std::wstring &filePath, const wchar_t *profile) {
        IDxcBlobEncoding *sourceBlob = nullptr;
        if(FAILED(dxcUtils->LoadFile(filePath.c_str(), nullptr, &sourceBlob))) return Microsoft::WRL::ComPtr<ID3DBlob>();

        LPCWSTR arguments[] = { filePath.c_str(), L"-E", L"main", L"-T", profile, L"-Zi", L"-Qembed_debug", L"-Od", L"-Zpr" };

        DxcBuffer buffer = {};
        buffer.Ptr = sourceBlob->GetBufferPointer();
        buffer.Size = sourceBlob->GetBufferSize();
        buffer.Encoding = DXC_CP_UTF8;

        IDxcResult *result = nullptr;
        dxcCompiler->Compile(&buffer, arguments, _countof(arguments), includeHandler, IID_PPV_ARGS(&result));

        IDxcBlobUtf8 *errorBlob = nullptr;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), nullptr);
        if(errorBlob && errorBlob->GetStringLength() > 0) {
            OutputDebugStringA((char *)errorBlob->GetStringPointer());
            assert(false);
        }
        Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob = nullptr;
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        result->Release();
        sourceBlob->Release();
        return shaderBlob;
        };

    auto vsBlob = CompileShader(L"shaders/Particle.VS.hlsl", L"vs_6_0");
    auto psBlob = CompileShader(L"shaders/Particle.PS.hlsl", L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_.Get();

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    psoDesc.InputLayout = { inputElements, _countof(inputElements) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    // 加算合成にする場合は以下を有効化
    // psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
    // psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    // psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    // psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    // 半透明の場合はデプス書き込みをOFFにすることが多い
    // psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;

    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));

    dxcUtils->Release();
    dxcCompiler->Release();
    includeHandler->Release();
}

void ParticleCommon::CreateMesh() {
    // 共通の四角形ポリゴンを作成
    vertices_ = {
        { {-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 左上
        { { 1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 右上
        { {-1.0f,-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 左下
        { {-1.0f,-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 左下
        { { 1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 右上
        { { 1.0f,-1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 右下
    };

    vertexResource_ = CreateBufferResource(device_, sizeof(ParticleVertexData) * vertices_.size());

    ParticleVertexData *data = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&data));
    std::memcpy(data, vertices_.data(), sizeof(ParticleVertexData) * vertices_.size());
    vertexResource_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(ParticleVertexData) * vertices_.size());
    vertexBufferView_.StrideInBytes = sizeof(ParticleVertexData);
}