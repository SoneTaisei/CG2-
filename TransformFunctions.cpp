#include "TransformFunctions.h"
#include<cmath>
#include"assert.h"

Matrix4x4 TransformFunctions::MakeRoteXMatrix(float radian) {
	Matrix4x4 result{};

	result = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,std::cos(radian),std::sin(radian),0.0f,
		0.0f,-std::sin(radian),std::cos(radian),0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	return result;
}

Matrix4x4 TransformFunctions::MakeRoteYMatrix(float radian) {
	Matrix4x4 result{};

	result = {
		std::cos(radian),0.0f,-std::sin(radian),0.0f,
		0.0f,1.0f,0.0f,0.0f,
		std::sin(radian),0.0f,std::cos(radian),0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	return result;
}

Matrix4x4 TransformFunctions::MakeRoteZMatrix(float radian) {
	Matrix4x4 result{};

	result = {
		std::cos(radian),std::sin(radian),0.0f,0.0f,
		-std::sin(radian),std::cos(radian),0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	return result;
}

/*行列の計算
*********************************************************/

// 平行移動行列
Matrix4x4 TransformFunctions::MakeTranslateMatrix(const Vector3 &translate) {
	Matrix4x4 result = {};

	result = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		translate.x,translate.y,translate.z,1.0f
	};

	return result;
}

// 拡大縮小行列
Matrix4x4 TransformFunctions::MakeScaleMatrix(const Vector3 &scale) {
	Matrix4x4 result = {};

	result = {
		scale.x,0.0f,0.0f,0.0f,
		0.0f,scale.y,0.0f,0.0f,
		0.0f,0.0f,scale.z,0.0f,
		0.0f,0.0f,0.0f,1.0f,
	};

	return result;
}

Matrix4x4 TransformFunctions::Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2) {
	Matrix4x4 result = {};

	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			for(int k = 0; k < 4; k++) {
				result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
			}
		}
	}

	return result;
}

// 座標変換
Vector3 TransformFunctions::Transform(const Vector3 &vector, const Matrix4x4 &matrix) {
	Vector3 result;

	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];

	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;

	return result;
}

// アフィン変換
Matrix4x4 TransformFunctions::MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate, const Vector3 &translate) {
	Matrix4x4 result{};

	// 回転行列
	Matrix4x4 rotZ = MakeRoteZMatrix(rotate.z);
	Matrix4x4 rotX = MakeRoteXMatrix(rotate.x);
	Matrix4x4 rotY = MakeRoteYMatrix(rotate.y);

	// 回転行列の合成
	Matrix4x4 rotateMatrix = Multiply(Multiply(rotX, rotY), rotZ);

	// 拡大縮小
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	// 平行移動
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	result = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

	return result;

}
