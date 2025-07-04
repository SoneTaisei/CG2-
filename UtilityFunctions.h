#pragma once
#include"Structs.h"
#include"ResourceObject.h"
#include"D3DResourceLeakChacker.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

void Log(const std::string &message);

std::wstring ConvertString(const std::string &str);

std::string ConvertString(const std::wstring &str);

LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception);

IDxcBlob *CompileShader(
	// CompilerするShaderファイルへのパス
	const std::wstring &filePath,
	// Compilerに使用するProfile
	const wchar_t *profile,
	// 初期化で生成したものを3つ
	IDxcUtils *dxcUtils,
	IDxcCompiler3 *dxcCompiler,
	IDxcIncludeHandler *includeHandler);

ID3D12Resource *CreateBufferResource(ID3D12Device *device, size_t sizeInBytes);

// DescriptorHeapの作成関数
ID3D12DescriptorHeap *CreateDescriptorHeap(
	ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

// Textureデータを読む
DirectX::ScratchImage LoadTexture(const std::string &filePath);

// DirectX12のTextureResourceを作る
ID3D12Resource *CreateTextureResource(ID3D12Device *device, const DirectX::TexMetadata &metadata);

// 戻り値を破損してはならないのでこれを付ける
[[nodiscard]]
// TextureResouorceにデータを転送する
ID3D12Resource *UploadTextureData(ID3D12Resource *texture, const DirectX::ScratchImage &mipImages, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);

ID3D12Resource *CreateDepthStencilTextureResource(ID3D12Device *device, int32_t width, int32_t height);

// DescriptorHandleを取得する(CPU)
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap, uint32_t descriptorSize, uint32_t index);

// DescriptorHandleを取得する(GPU)
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap, uint32_t descriptorSize, uint32_t index);

void CreateSphereMesh(std::vector<VertexData> &vertices, std::vector<uint32_t> &indices, float radius, int latDiv, int lonDiv);

/// <summary>
/// objファイルを読む関数 
/// </summary>
/// <param name="directoryPath"></param>
/// <param name="filename"></param>
/// <returns></returns>
ModelData LoadObjFile(const std::string &directoryPath, const std::string &filename);

MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

SoundData SoundLoadWave(const char *filename);

void SoundUnload(SoundData *soundData);

void SoundPlayWave(IXAudio2 *xAudio2, const SoundData &soundData);

/*キー入力の取得
*********************************************************/

// 押されている時
bool IsKeyHeld(BYTE keys);

// キーが離された瞬間
bool IsKeyReleased(BYTE keys, BYTE preKeys);

// キーが押された瞬間
bool IsKeyPressed(BYTE keys, BYTE preKeys);

// 押されていないとき
bool IsKeyUp(BYTE keys);

