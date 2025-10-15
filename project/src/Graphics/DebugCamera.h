#pragma once
#include"Utility/Structs.h"
class DebugCamera {
private:
	// 初期状態
	Vector3 initialRotation_;
	Vector3 initialTranslation_;
	
	// x,y,z軸周りのローカル回転角度
	Vector3 rotation_ = { 0,0,0 };
	// ローカル座標
	Vector3 translation_ = { 0,0,-5.0f };
	// ビュー行列
	Matrix4x4 viewMatrix_;
	// 射影行列
	Matrix4x4 projectionMatrix_;
	
	int kClientWidth_;
	int kClientHeight_;

	/// <summary>
	/// ビュー行列と射影行列を更新する
	/// </summary>
	void UpdateMatrix();

public:
	/// <summary>
	///	初期化
	/// </summary>
	void Initialize(Matrix4x4 viewMatrix, Matrix4x4 projectionMatrix, const int kClientWidth, const int kClientHeight);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// カメラの状態を初期値にリセットする
	/// </summary>
	void Reset();

	Matrix4x4 GetViewMatrix() const { return viewMatrix_; }
	Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }
	Vector3 GetTranslation() const { return translation_; }

};

