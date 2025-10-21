#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "Utility/Utilityfunctions.h"

class Sprite {
public:
    // 静的初期化・終了処理
    static void StaticInitialize(
        ID3D12Device *device,
        int windowWidth, int windowHeight,
        const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> &textureSrvHandles);
    static void StaticFinalize();

    // 描画前処理
    static void PreDraw(ID3D12GraphicsCommandList *commandList);

    // 静的描画関数
    static void Draw(
        float destX, float destY,         // 描画先の左上座標
        float width, float height,        // スプライトの幅と高さ
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,           // テクスチャハンドル (インデックス)
        float scaleX = 1.0f, float scaleY = 1.0f, // 拡縮率
        float angle = 0.0f,               // 回転角度 (Z軸周り、ラジアン)
        const Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } // 色
    );

private:
    // 静的メンバ変数
    static ID3D12Device *sDevice_;
    static ID3D12GraphicsCommandList *sCommandList_;
    static Microsoft::WRL::ComPtr<ID3D12Resource> sVertexResource_;
    static D3D12_VERTEX_BUFFER_VIEW sVertexBufferView_;
    static Microsoft::WRL::ComPtr<ID3D12Resource> sIndexResource_;
    static D3D12_INDEX_BUFFER_VIEW sIndexBufferView_;
    static Microsoft::WRL::ComPtr<ID3D12Resource> sMaterialResource_;
    static Microsoft::WRL::ComPtr<ID3D12Resource> sTransformationMatrixResource_;

    // マップ済みポインタ
    static Material *sMaterialData_;
    static Matrix4x4 *sTransformationMatrixData_;

    // 射影行列
    static Matrix4x4 sProjectionMatrix_;

    // テクスチャハンドルの配列
    static std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> sTextureSrvHandles_;

    // 定数
    static const int kVertexCount = 4;
    static const int kIndexCount = 6;

    static int sWindowWidth_;
    static int sWindowHeight_;
};

