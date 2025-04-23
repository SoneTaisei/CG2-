#pragma once

struct Matrix4x4 {
	float m[4][4];
};

struct Vector3 {
	float x;
	float y;
	float z;
};

class TransformFunctions {
public:
	static Matrix4x4 MakeRoteXMatrix(float radian);
	static Matrix4x4 MakeRoteYMatrix(float radian);
	static Matrix4x4 MakeRoteZMatrix(float radian);
	static Matrix4x4 MakeTranslateMatrix(const Vector3 &translate);
	static Matrix4x4 MakeScaleMatrix(const Vector3 &scale);
	static Matrix4x4 Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2);
	static Vector3 Transform(const Vector3 &vector, const Matrix4x4 &matrix);
	static Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate, const Vector3 &translate);
};

