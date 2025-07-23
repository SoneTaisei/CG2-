#pragma once
#include"Structs.h"
class DebugCamera {
private:
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


public:
	/// <summary>
	///	初期化
	/// </summary>
	void Initialize(Matrix4x4 viewMatrix, Matrix4x4 projectionMatrix, const int kClientWidth, const int kClientHeight);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	Matrix4x4 GetViewMatrix() const { return viewMatrix_; }
	Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }

};

