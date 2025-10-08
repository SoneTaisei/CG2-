#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>

// 以下のヘッダーファイルは、ご自身のプロジェクト構成に合わせてインクルードしてください。
#include "Utility/Utilityfunctions.h" // Transform, Matrix4x4, VertexData, TransformMatrix などを含むヘッダー
#include "Model/Model.h"      // LoadObjFileが返すModelData構造体を含むヘッダー

class Model {
public:
    /// <summary>
    /// 静的初期化処理。D3D12デバイスを保持します。
    /// </summary>
    /// <param name="device">D3D12デバイス</param>
    static void StaticInitialize(ID3D12Device *device);

    /// <summary>
    /// 描画前処理。コマンドリストを保持します。
    /// </summary>
    /// <param name="commandList">コマンドリスト</param>
    static void PreDraw(ID3D12GraphicsCommandList *commandList);

    /// <summary>
    /// OBJファイルからモデルを生成します。
    /// </summary>
    /// <param name="directoryPath">ファイルのあるディレクトリパス</param>
    /// <param name="filename">ファイル名</param>
    /// <returns>生成されたモデルのインスタンス</returns>
    static Model *CreateFromObj(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    /// 球体モデルを生成します。
    /// </summary>
    /// <returns>生成された球体モデルのインスタンス</returns>
    static Model *CreateSphere();

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="transform">ワールド変換情報</param>
    /// <param name="viewProjectionMatrix">ビュープロジェクション行列</param>
    /// <param name="textureHandle">テクスチャのSRVハンドル</param>
    void Draw(const Transform &transform, const Matrix4x4 &viewProjectionMatrix, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

    /// <summary>
    /// モデルの情報を取得します（テクスチャファイルパスなどに使用）。
    /// </summary>
    /// <returns>モデルデータ</returns>
    const ModelData &GetModelData() const { return modelData_; }


private:
    // プライベートコンストラクタで、ファクトリ関数からの生成を強制
    Model() = default;

    // 頂点バッファ・インデックスバッファの作成
    void CreateBuffers();

private:
    // --- 静的メンバ変数 ---
    static ID3D12Device *sDevice_;
    static ID3D12GraphicsCommandList *sCommandList_;

    // --- メンバ変数 ---
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    ModelData modelData_; // 頂点、インデックス、マテリアル情報などを保持
};