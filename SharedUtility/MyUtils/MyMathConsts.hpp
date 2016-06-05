#pragma once

#include "MyMathTypes.hpp"


namespace MyMath
{
	const float F_PI = 3.14159265358979323846f;
	const double D_PI = 3.14159265358979323846;

	const float F_2PI = 2.0f * F_PI;
	const double D_2PI = 2.0 * D_PI;

	const float F_PIDIV2 = 0.5f * F_PI;
	const double D_PIDIV2 = 0.5 * D_PI;

	const float F_PIDIV4 = 0.25f * F_PI;
	const double D_PIDIV4 = 0.25 * D_PI;

	// <DirectXColors.h> には DirectX::Colors::White などの定義済みカラーが定義されているが、型が XMVECTORF32 なので注意。

	const MyMath::Vector4F COLOR4F_WHITE(1,1,1,1);
	const MyMath::Vector4F COLOR4F_BLACK(0,0,0,1);
	const MyMath::Vector4F COLOR4F_TRANSPARENT(0,0,0,0);

	// OpenGL ヘルパーの GLM ではデフォルト コンストラクタによりゼロ ベクトルや単位行列が生成されるが、
	// D3DX Math, XNA Math, DirectXMath のデフォルト コンストラクタは怠惰で、ゴミデータが入ったままになる。

	const Vector2I ZERO_VECTOR2I(0, 0);
	const Vector3I ZERO_VECTOR3I(0, 0, 0);
	const Vector4I ZERO_VECTOR4I(0, 0, 0, 0);
	const Vector2F ZERO_VECTOR2F(0, 0);
	const Vector3F ZERO_VECTOR3F(0, 0, 0);
	const Vector4F ZERO_VECTOR4F(0, 0, 0, 0);
	const Vector3F FLTMAX_VECTOR3F(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	const Vector3F FLTMIN_VECTOR3F(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	const QuaternionF NO_ROTATION_QUATERNIONF(0, 0, 0, 1);
	const MatrixF ZERO_MATRIXF(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
	const MatrixF IDENTITY_MATRIXF(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	const Matrix4x4F ZERO_MATRIX4X4F(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
	const Matrix4x4F IDENTITY_MATRIX4X4F(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	//const Matrix4x3F ZERO_MATRIX4X3F(0,0,0, 0,0,0, 0,0,0, 0,0,0);
	//const Matrix4x3F IDENTITY_MATRIX4X3F(1,0,0, 0,1,0, 0,0,1, 0,0,0);

	const ColorRgba ColorRgbaWhite(0xFF, 0xFF, 0xFF);
	const ColorRgba ColorRgbaBlack(0, 0, 0);
}

//#define USE_LEFT_HAND_COORD_SYS
// 一般的なモデリング ツールや OpenGL のデフォルトに合わせるならば、右手系を採用しておいたほうがいい。
