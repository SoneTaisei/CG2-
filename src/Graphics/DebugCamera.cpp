#include "DebugCamera.h"
#include "Utility/TransformFunctions.h"
#include <algorithm> // std::clamp を使うために必要

void DebugCamera::Initialize(Matrix4x4 viewMatrix, Matrix4x4 projectionMatrix, const int kClientWidth, const int kClientHeight) {
	viewMatrix_ = viewMatrix;
	projectionMatrix_ = projectionMatrix;
	kClientWidth_ = kClientWidth;
	kClientHeight_ = kClientHeight;
}

void DebugCamera::Update() {
	// --- 定数 ---
	const float moveSpeed = 0.02f; // 平行移動の速度
	const float zoomSpeed = 0.4f;  // ズームの速度
	const float rotateSpeed = 0.005f; // 回転の速度

	ImGuiIO &io = ImGui::GetIO();

	// ImGuiのウィンドウが操作されているときは、カメラを動かさない
	if(io.WantCaptureMouse) {
		return;
	}

	// --- マウス入力によるカメラの回転 ---
	if(io.MouseDown[1]) { // 右クリック
		rotation_.y += io.MouseDelta.x * rotateSpeed;
		rotation_.x += io.MouseDelta.y * rotateSpeed;
		// X軸の回転範囲を制限（-90度から+90度）
		rotation_.x = std::clamp(rotation_.x, -1.57f, 1.57f);
	}

	// --- 回転からカメラの基底ベクトルを計算 ---
	Matrix4x4 rotationMatrix = TransformFunctions::Multiply(
		TransformFunctions::MakeRoteXMatrix(rotation_.x),
		TransformFunctions::MakeRoteYMatrix(rotation_.y)
	);

	// カメラの各軸の向き（ローカル座標系）
	Vector3 forward = { rotationMatrix.m[2][0], rotationMatrix.m[2][1], rotationMatrix.m[2][2] };
	Vector3 right = { rotationMatrix.m[0][0], rotationMatrix.m[0][1], rotationMatrix.m[0][2] };
	Vector3 up = { rotationMatrix.m[1][0], rotationMatrix.m[1][1], rotationMatrix.m[1][2] };

	// --- マウス入力によるカメラの移動 ---
	// 1. ズーム（前後に移動）
	if(io.MouseWheel != 0.0f) {
		// forwardベクトル（カメラの向き）に対して移動
		translation_ = TransformFunctions::AddV(translation_, TransformFunctions::MultiplyV(io.MouseWheel * zoomSpeed, forward));
	}
	// 2. 平行移動
	if(io.MouseDown[2]) { // 中クリック
		// rightベクトル（右方向）とupベクトル（上方向）に移動
		translation_ = TransformFunctions::AddV(translation_, TransformFunctions::MultiplyV(-io.MouseDelta.x * moveSpeed, right));
		translation_ = TransformFunctions::AddV(translation_, TransformFunctions::MultiplyV(io.MouseDelta.y * moveSpeed, up));
	}

	// --- ビュー行列と射影行列の再計算 ---
	// 1. ビュー行列（Inverseを使わない安全な方法）
	Matrix4x4 translateMatrix = TransformFunctions::MakeTranslateMatrix({ -translation_.x, -translation_.y, -translation_.z });
	Matrix4x4 rotateMatrixInv = TransformFunctions::Transpose(rotationMatrix);
	viewMatrix_ = TransformFunctions::Multiply(translateMatrix, rotateMatrixInv);

	// 2. 射影行列
	projectionMatrix_ =
		TransformFunctions::MakePerspectiveFovMatrix(0.45f, float(kClientWidth_) / float(kClientHeight_), 0.1f, 100.0f);
}