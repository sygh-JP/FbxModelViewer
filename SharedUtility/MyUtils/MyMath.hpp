#pragma once

#include "MyUtil.h"
#include "MyMathTypes.hpp"
#include "MyMathConsts.hpp"


namespace MyMath
{
	inline float ToRadian(float degree) { return degree * F_PI / 180.0f; }
	inline float ToDegree(float radian) { return radian * 180.0f / F_PI; }
	inline double ToRadian(double degree) { return degree * D_PI / 180.0; }
	inline double ToDegree(double radian) { return radian * 180.0 / D_PI; }

	// D3DX9 の D3DXToRadian() / D3DXToDegree() は単精度浮動小数点数のリテラルを使ったマクロだったが、
	// D3DX10 の D3DXToRadian() / D3DXToDegree() はなぜか倍精度浮動小数点数のリテラルを使ったマクロになっている。
	// どのみちマクロはタイプセーフでないので、使うべきではない。
	// XNA Math および DirectXMath の XMConvertToRadians(), XMConvertToDegrees() はインライン関数による実装となっている。
#pragma deprecated("D3DXToRadian")
#pragma deprecated("D3DXToDegree")

	//! @brief  x が 2 のべき乗であるか否かを調べる。<br>
	template<typename T> bool IsModulo2(T x)
	{ return x != 0 && (x & (x - 1)) == 0; }

#if 0
	inline uint32_t CalcStrideInBytes(uint64_t widthInPixel, uint64_t bitsPerPixel)
	{
		uint64_t line = (widthInPixel * bitsPerPixel) / 8;
		if ((line % 4) != 0)
		{
			line = (line / 4 + 1) * 4;
		}
		return static_cast<uint32_t>(line);
	}
#else
	//! @brief  ストライドを計算する。4 バイト (32bit) の倍数への切り上げ。<br>
	inline uint32_t CalcStrideInBytes(uint32_t widthInPixel, uint32_t bitsPerPixel)
	{
		_ASSERTE((uint64_t(widthInPixel) * uint64_t(bitsPerPixel) + uint64_t(31)) <= (std::numeric_limits<uint32_t>::max)());
		// https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/ns-wingdi-bitmapinfoheader
		return ((((widthInPixel * bitsPerPixel) + uint32_t(31)) & ~uint32_t(31)) >> uint32_t(3));
	}
#endif


	inline void SetVector4XYZ(Vector4F& vOut, const Vector3F& vIn)
	{
		vOut.x = vIn.x;
		vOut.y = vIn.y;
		vOut.z = vIn.z;
	}

	//! @brief  3次元ベクトルと w 成分から4次元ベクトルを作成する。<br>
	inline void CreateVector4(Vector4F& vOut, const Vector3F& vIn, float w)
	{
		vOut.x = vIn.x;
		vOut.y = vIn.y;
		vOut.z = vIn.z;
		vOut.w = w;
	}

	inline Vector4F CreateVector4(const Vector3F& vIn, float w)
	{
		return Vector4F(vIn.x, vIn.y, vIn.z, w);
	}

	// 古い Windows API への依存を断つため、変換メソッドは COLORREF シンボルに直接関係しないようにする。

	// リトルエンディアン表記が基準なので注意。COLORREF からの変換に使える。
	inline Vector4F ConvertRGBXToColor4F(uint32_t colorVal)
	{
		return MyMath::Vector4F(
			((colorVal) & 0xFF) / 255.0f,
			((colorVal >> 8) & 0xFF) / 255.0f,
			((colorVal >> 16) & 0xFF) / 255.0f,
			1.0f);
	}

	// リトルエンディアン表記が基準なので注意。COLORREF からの変換に使える。
	inline Vector3F ConvertRGBXToColor3F(uint32_t colorVal)
	{
		return MyMath::Vector3F(
			((colorVal) & 0xFF) / 255.0f,
			((colorVal >> 8) & 0xFF) / 255.0f,
			((colorVal >> 16) & 0xFF) / 255.0f
			);
	}

	// リトルエンディアン表記が基準なので注意。COLORREF への変換に使える。
	inline uint32_t ConvertColor3FToRGBX(const Vector3F& colorVal)
	{
		return
			(uint32_t(MyUtils::Clamp(colorVal.x * 255.0f, 0.0f, 255.0f))) |
			(uint32_t(MyUtils::Clamp(colorVal.y * 255.0f, 0.0f, 255.0f)) << 8) |
			(uint32_t(MyUtils::Clamp(colorVal.z * 255.0f, 0.0f, 255.0f)) << 16);
	}

	// リトルエンディアン表記が基準なので注意。COLORREF への変換に使える。
	inline uint32_t ConvertColor4FToRGBX(const Vector4F& colorVal)
	{
		return
			(uint32_t(MyUtils::Clamp(colorVal.x * 255.0f, 0.0f, 255.0f))) |
			(uint32_t(MyUtils::Clamp(colorVal.y * 255.0f, 0.0f, 255.0f)) << 8) |
			(uint32_t(MyUtils::Clamp(colorVal.z * 255.0f, 0.0f, 255.0f)) << 16);
	}


	inline uint8_t ToGrayscaleUI8(uint8_t r, uint8_t g, uint8_t b)
	{
		// 0.298912 * R + 0.586611 * G + 0.114478 * B
		const double graylevel =
			0.298912 * r +
			0.586611 * g +
			0.114478 * b;
		//return static_cast<uint8_t>(MyUtils::Clamp<double>(graylevel, 0, 255));
		// uint8_t が入力であれば、単純な切り捨て演算で OK。
		return static_cast<uint8_t>(graylevel);
	}


	// 線形補間用ヘルパーテンプレート。
	template<typename T> T Lerp(T x, T y, float s)
	{
		return static_cast<T>(x + s * (y - x));
	}

	template<typename T> T Lerp(T x, T y, double s)
	{
		return static_cast<T>(x + s * (y - x));
	}

	inline ColorRgba LerpColor(ColorRgba x, ColorRgba y, float s)
	{
		return ColorRgba(
#if 0
			static_cast<uint8_t>(x.R + s * (y.R - x.R)),
			static_cast<uint8_t>(x.G + s * (y.G - x.G)),
			static_cast<uint8_t>(x.B + s * (y.B - x.B)),
			static_cast<uint8_t>(x.A + s * (y.A - x.A))
#else
			Lerp(x.R, y.R, s),
			Lerp(x.G, y.G, s),
			Lerp(x.B, y.B, s),
			Lerp(x.A, y.A, s)
#endif
			);
	}


	inline bool IsEqualVector2(const Vector2F& vIn1, const Vector2F& vIn2)
	{
		return DirectX::XMVector2Equal(DirectX::XMLoadFloat2(&vIn1), DirectX::XMLoadFloat2(&vIn2));
	}

	inline bool IsEqualVector3(const Vector3F& vIn1, const Vector3F& vIn2)
	{
		return DirectX::XMVector3Equal(DirectX::XMLoadFloat3(&vIn1), DirectX::XMLoadFloat3(&vIn2));
	}

	inline bool IsEqualVector4(const Vector4F& vIn1, const Vector4F& vIn2)
	{
		return DirectX::XMVector4Equal(DirectX::XMLoadFloat4(&vIn1), DirectX::XMLoadFloat4(&vIn2));
	}

	// http://msdn.microsoft.com/ja-jp/library/windows/apps/jj863300.aspx
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ee418725.aspx
	// ARM では XMVECTOR 型を初期化子リストを使って初期化することはできない。
	// 定数ベクトルを作成する場合、XMVectorSet(), XMVectorSetInt() もしくは XMVECTORF32, XMVECTORU32 を使う。
	// 旧 DirectX SDK のサンプルには Windows/Xbox 360 向けに XNA Math を使ったコードがいくつかあるが、
	// ARM に関して移植性のないコードが含まれているものもあるので注意。
	// e.g.
	// "%ProgramFiles(x86)%\Microsoft DirectX SDK (June 2010)\Samples\C++\Direct3D11\CascadedShadowMaps11\xnacollision.cpp"
	// ちなみにこの XNACollision に相当するライブラリが実は DirectXCollision として Windows SDK 8.x に含まれている。
	// これを使うとよい。

	inline DirectX::XMVECTOR CreateEpsilon4F()
	{
		//return DirectX::XMVectorSet(FLT_EPSILON, FLT_EPSILON, FLT_EPSILON, FLT_EPSILON);
		return DirectX::g_XMEpsilon;
	}

	inline bool IsEpsilonEqualVector2(const Vector2F& vIn1, const Vector2F& vIn2)
	{
		return DirectX::XMVector2NearEqual(DirectX::XMLoadFloat2(&vIn1), DirectX::XMLoadFloat2(&vIn2),
			CreateEpsilon4F());
	}

	inline bool IsEpsilonEqualVector3(const Vector3F& vIn1, const Vector3F& vIn2)
	{
		return DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&vIn1), DirectX::XMLoadFloat3(&vIn2),
			CreateEpsilon4F());
	}

	inline bool IsEpsilonEqualVector4(const Vector4F& vIn1, const Vector4F& vIn2)
	{
		return DirectX::XMVector4NearEqual(DirectX::XMLoadFloat4(&vIn1), DirectX::XMLoadFloat4(&vIn2),
			CreateEpsilon4F());
	}


	// 射影行列から視錐台を作成する。
	inline void CreateFrustumFromProjectionMatrix(BoundingFrustumF& outFrustum, const Matrix4x4F& inMatrix)
	{
		BoundingFrustumF::CreateFromMatrix(outFrustum, DirectX::XMLoadFloat4x4(&inMatrix));
	}


	inline void AddVector3(Vector3F& vOut, const Vector3F& vIn1, const Vector3F& vIn2)
	{
#if 0
		//return Vector3F(vIn1.x + vIn2.x, vIn1.y + vIn2.y, vIn1.z + vIn2.z);
		vOut.x = vIn1.x + vIn2.x;
		vOut.y = vIn1.y + vIn2.y;
		vOut.z = vIn1.z + vIn2.z;
#else
		DirectX::XMStoreFloat3(&vOut, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&vIn1), DirectX::XMLoadFloat3(&vIn2)));
#endif
	}

	inline void SubtractVector3(Vector3F& vOut, const Vector3F& vIn1, const Vector3F& vIn2)
	{
#if 0
		//return Vector3F(vIn1.x - vIn2.x, vIn1.y - vIn2.y, vIn1.z - vIn2.z);
		vOut.x = vIn1.x - vIn2.x;
		vOut.y = vIn1.y - vIn2.y;
		vOut.z = vIn1.z - vIn2.z;
#else
		DirectX::XMStoreFloat3(&vOut, DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&vIn1), DirectX::XMLoadFloat3(&vIn2)));
#endif
	}

	inline void MultiplyVector3(Vector3F& vOut, float s, const Vector3F& vIn)
	{
#if 0
		//return Vector3F(s * vIn.x, s * vIn.y, s * vIn.z);
		vOut.x = s * vIn.x;
		vOut.y = s * vIn.y;
		vOut.z = s * vIn.z;
#else
		DirectX::XMStoreFloat3(&vOut, DirectX::XMVectorMultiply(DirectX::XMVectorSet(s, s, s, s), DirectX::XMLoadFloat3(&vIn)));
#endif
	}

	inline float GetVector2LengthSq(const Vector2F& vIn)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(DirectX::XMVectorSet(vIn.x, vIn.y, 0, 0)));
	}

	inline float GetVector2Length(const Vector2F& vIn)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(vIn.x, vIn.y, 0, 0)));
	}

	inline float GetVector3LengthSq(const Vector3F& vIn)
	{
#if 0
		return vIn.x * vIn.x + vIn.y * vIn.y + vIn.z * vIn.z;
#else
		return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMVectorSet(vIn.x, vIn.y, vIn.z, 0)));
#endif
	}

	inline float GetVector3Length(const Vector3F& vIn)
	{
		// XMLoadFloat3() を使うと、w 要素はゼロではなく不定になるので注意。
		// XMVector3Length() に渡す場合は問題ないかも（たぶん w が無視される）。
		// DirectX::XMVector3Length() は SIMD 出力ベクトルを返すので、XMVectorGetX() や XMVectorGetXPtr() などを使わねばならず、扱いづらい。
		// HACK: ベンチマークをとって、CRT 数学関数を使うよりも DirectXMath のほうが高速であることが判明すればそちらを使う。
		// 組み込みの sqrt 演算命令を使うようなので、リリース ビルドでは DirectXMath のほうが高速化するとは思うが……
#if 0
		return sqrt(GetVector3LengthSq(vIn));
#else
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(vIn.x, vIn.y, vIn.z, 0)));
#endif
	}

	inline float GetVector2DistanceSq(const Vector2F& vIn1, const Vector2F& vIn2)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(
			DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&vIn1), DirectX::XMLoadFloat2(&vIn2))));
	}

	inline float GetVector2Distance(const Vector2F& vIn1, const Vector2F& vIn2)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector2Length(
			DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&vIn1), DirectX::XMLoadFloat2(&vIn2))));
	}

	inline float GetVector3DistanceSq(const Vector3F& vIn1, const Vector3F& vIn2)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(
			DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&vIn1), DirectX::XMLoadFloat3(&vIn2))));
	}

	inline float GetVector3Distance(const Vector3F& vIn1, const Vector3F& vIn2)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(
			DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&vIn1), DirectX::XMLoadFloat3(&vIn2))));
	}


	inline void NormalizeVector2(Vector2F* pOut, const Vector2F* pIn)
	{
		DirectX::XMStoreFloat2(pOut, DirectX::XMVector2Normalize(DirectX::XMLoadFloat2(pIn)));
	}

	inline void NormalizeVector3(Vector3F* pOut, const Vector3F* pIn)
	{
		DirectX::XMStoreFloat3(pOut, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(pIn)));
	}

	inline void NormalizeVector4(Vector4F* pOut, const Vector4F* pIn)
	{
		DirectX::XMStoreFloat4(pOut, DirectX::XMVector4Normalize(DirectX::XMLoadFloat4(pIn)));
	}

	inline void NormalizeQuaternion(Vector4F* pOut, const Vector4F* pIn)
	{
		DirectX::XMStoreFloat4(pOut, DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(pIn)));
	}


	// D3DXVec2CCW() 相当。
	inline float CrossVector2(const Vector2F* pIn0, const Vector2F* pIn1)
	{
		DirectX::XMVectorGetX(DirectX::XMVector2Cross(DirectX::XMLoadFloat2(pIn0), DirectX::XMLoadFloat2(pIn1)));
	}

	// D3DXVec3Cross() 相当。
	inline void CrossVector3(Vector3F* pOut, const Vector3F* pIn0, const Vector3F* pIn1)
	{
		DirectX::XMStoreFloat3(pOut, DirectX::XMVector3Cross(DirectX::XMLoadFloat3(pIn0), DirectX::XMLoadFloat3(pIn1)));
	}

	// D3DXVec4Cross() 相当。
	inline void CrossVector4(Vector4F* pOut, const Vector4F* pIn0, const Vector4F* pIn1, const Vector4F* pIn2)
	{
		DirectX::XMStoreFloat4(pOut,
			DirectX::XMVector4Cross(DirectX::XMLoadFloat4(pIn0), DirectX::XMLoadFloat4(pIn1), DirectX::XMLoadFloat4(pIn2)));
	}


	inline void LerpVector2(Vector2F& outVal, const Vector2F& in0, const Vector2F& in1, float t)
	{
		DirectX::XMStoreFloat2(&outVal, DirectX::XMVectorLerp(DirectX::XMLoadFloat2(&in0), DirectX::XMLoadFloat2(&in1), t));
	}

	inline void LerpVector3(Vector3F& outVal, const Vector3F& in0, const Vector3F& in1, float t)
	{
		DirectX::XMStoreFloat3(&outVal, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&in0), DirectX::XMLoadFloat3(&in1), t));
	}

	inline void LerpVector4(Vector4F& outVal, const Vector4F& in0, const Vector4F& in1, float t)
	{
		DirectX::XMStoreFloat4(&outVal, DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&in0), DirectX::XMLoadFloat4(&in1), t));
	}

	inline void SlerpQuaternion(QuaternionF& outVal, const QuaternionF& in0, const QuaternionF& in1, float t)
	{
		DirectX::XMStoreFloat4(&outVal, DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&in0), DirectX::XMLoadFloat4(&in1), t));
	}


	inline void CreateMatrixTranslation(MatrixF* pOut, const Vector3F* pTranslation)
	{
		const DirectX::XMMATRIX mTranslation = DirectX::XMMatrixTranslation(pTranslation->x, pTranslation->y, pTranslation->z);
		DirectX::XMStoreFloat4x4(pOut, mTranslation);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixTranslation(MatrixF* pOut, const Vector3F* pTranslation)
	{
		const DirectX::XMMATRIX mTranslation = DirectX::XMMatrixTranslation(pTranslation->x, pTranslation->y, pTranslation->z);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mTranslation));
	}

	//! @brief  z-x-y つまり Roll-Pitch-Yaw の順序で回転。<br>
	inline void CreateMatrixRotationZXY(MatrixF* pOut, const Vector3F* pRatationAmountInRad)
	{
		// NOTE: XMMATRIX 型の戻り値をローカル変数に格納する際は、C++11 の auto で型推論させないほうがいいらしい。
		// VC++ 2012 の Release ビルドでは不正な最適化がかかるらしく、不可解なアクセス違反の原因になる。
		const DirectX::XMMATRIX mRotation = DirectX::XMMatrixRotationRollPitchYaw(
			pRatationAmountInRad->x, // Pitch
			pRatationAmountInRad->y, // Yaw
			pRatationAmountInRad->z // Roll
			);
		DirectX::XMStoreFloat4x4(pOut, mRotation);
		// D3DX Math のときは D3DXMatrixRotationYawPitchRoll() という名前だったが、
		// どちらも回転の順序はロール、ピッチ、ヨーの順であり、z-x-y となる。
		// ただしパラメータの順序が D3DX Math と XNA Math/DirectXMath とで異なるので、置き換える場合は注意。
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixRotationPitchYawRoll(MatrixF* pOut, const Vector3F* pRatationAmountInRad)
	{
		const DirectX::XMMATRIX mRotation = DirectX::XMMatrixRotationRollPitchYaw(
			pRatationAmountInRad->x,
			pRatationAmountInRad->y,
			pRatationAmountInRad->z);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mRotation));
	}

	inline void CreateMatrixScaling(MatrixF* pOut, const Vector3F* pScaling)
	{
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixScaling(pScaling->x, pScaling->y, pScaling->z));
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixScaling(MatrixF* pOut, const Vector3F* pScaling)
	{
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(pScaling->x, pScaling->y, pScaling->z)));
	}

	inline void TransformVectorByMatrix(Vector4F* pOut, const Vector3F* pIn, const MatrixF* pTransformMatrix)
	{
		const auto vOut = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(pIn), DirectX::XMLoadFloat4x4(pTransformMatrix));
		DirectX::XMStoreFloat4(pOut, vOut);
	}

	inline void TransformVectorByMatrix(Vector3F* pOut, const Vector3F* pIn, const MatrixF* pTransformMatrix)
	{
		const auto vOut = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(pIn), DirectX::XMLoadFloat4x4(pTransformMatrix));
		DirectX::XMStoreFloat3(pOut, vOut);
	}

	inline void InverseMatrix(MatrixF* pOut, const MatrixF* pIn)
	{
		DirectX::XMVECTOR vDet; // D3DX とは違い、NULL 指定は不可能らしい。
		const DirectX::XMMATRIX mOut = DirectX::XMMatrixInverse(&vDet, DirectX::XMLoadFloat4x4(pIn));
		DirectX::XMStoreFloat4x4(pOut, mOut);
	}

	inline void MultiplyMatrix(MatrixF* pOut, const MatrixF* pIn1, const MatrixF* pIn2)
	{
		const DirectX::XMMATRIX mOut = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(pIn1), DirectX::XMLoadFloat4x4(pIn2));
		DirectX::XMStoreFloat4x4(pOut, mOut);
	}

	inline void MultiplyMatrix(MatrixF* pOut, const MatrixF* pIn1, const MatrixF* pIn2, const MatrixF* pIn3)
	{
		const DirectX::XMMATRIX mOut12 = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(pIn1), DirectX::XMLoadFloat4x4(pIn2));
		const DirectX::XMMATRIX mOut123 = DirectX::XMMatrixMultiply(mOut12, DirectX::XMLoadFloat4x4(pIn3));
		DirectX::XMStoreFloat4x4(pOut, mOut123);
	}


	inline void CreateMatrixLookAtLH(MatrixF* pOut, const Vector3F* pCameraEye, const Vector3F* pCameraAt, const Vector3F* pCameraUp)
	{
		// Eye と At が一致してしまうと、DirectXMath のアサーションが失敗するので注意。
		// もちろんレンダリング結果も不正になる。
		// ちなみに ～RH は ～LH を使って実装されている。
		const DirectX::XMMATRIX mView = DirectX::XMMatrixLookAtLH(
			DirectX::XMLoadFloat3(pCameraEye), DirectX::XMLoadFloat3(pCameraAt), DirectX::XMLoadFloat3(pCameraUp));
		DirectX::XMStoreFloat4x4(pOut, mView);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixLookAtLH(MatrixF* pOut, const Vector3F* pCameraEye, const Vector3F* pCameraAt, const Vector3F* pCameraUp)
	{
		const DirectX::XMMATRIX mView = DirectX::XMMatrixLookAtLH(
			DirectX::XMLoadFloat3(pCameraEye), DirectX::XMLoadFloat3(pCameraAt), DirectX::XMLoadFloat3(pCameraUp));
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mView));
	}

	inline void CreateMatrixLookAtRH(MatrixF* pOut, const Vector3F* pCameraEye, const Vector3F* pCameraAt, const Vector3F* pCameraUp)
	{
		const DirectX::XMMATRIX mView = DirectX::XMMatrixLookAtRH(
			DirectX::XMLoadFloat3(pCameraEye), DirectX::XMLoadFloat3(pCameraAt), DirectX::XMLoadFloat3(pCameraUp));
		DirectX::XMStoreFloat4x4(pOut, mView);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixLookAtRH(MatrixF* pOut, const Vector3F* pCameraEye, const Vector3F* pCameraAt, const Vector3F* pCameraUp)
	{
		const DirectX::XMMATRIX mView = DirectX::XMMatrixLookAtRH(
			DirectX::XMLoadFloat3(pCameraEye), DirectX::XMLoadFloat3(pCameraAt), DirectX::XMLoadFloat3(pCameraUp));
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mView));
	}

	inline void CreateMatrixPerspectiveFovLH(MatrixF* pOut, float fovAngleInRad, float aspectRatio, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixPerspectiveFovLH(fovAngleInRad, aspectRatio, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, mProj);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixPerspectiveFovLH(MatrixF* pOut, float fovAngleInRad, float aspectRatio, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixPerspectiveFovLH(fovAngleInRad, aspectRatio, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mProj));
	}

	inline void CreateMatrixPerspectiveFovRH(MatrixF* pOut, float fovAngleInRad, float aspectRatio, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixPerspectiveFovRH(fovAngleInRad, aspectRatio, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, mProj);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixPerspectiveFovRH(MatrixF* pOut, float fovAngleInRad, float aspectRatio, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixPerspectiveFovRH(fovAngleInRad, aspectRatio, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mProj));
	}

	// D3DXMatrixOrthoLH() 相当。
	inline void CreateMatrixOrthoLH(MatrixF* pOut, float width, float height, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, mProj);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixOrthoLH(MatrixF* pOut, float width, float height, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mProj));
	}

	// D3DXMatrixOrthoRH() 相当。
	inline void CreateMatrixOrthoRH(MatrixF* pOut, float width, float height, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicRH(width, height, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, mProj);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixOrthoRH(MatrixF* pOut, float width, float height, float nearPlane, float farPlane)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicRH(width, height, nearPlane, farPlane);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mProj));
	}

	// D3DXMatrixOrthoOffCenterLH() 相当。
	inline void CreateMatrixOrthoOffCenterLH(MatrixF* pOut, float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
		DirectX::XMStoreFloat4x4(pOut, mProj);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixOrthoOffCenterLH(MatrixF* pOut, float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mProj));
	}

	// D3DXMatrixOrthoOffCenterRH() 相当。
	inline void CreateMatrixOrthoOffCenterRH(MatrixF* pOut, float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, nearZ, farZ);
		DirectX::XMStoreFloat4x4(pOut, mProj);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixOrthoOffCenterRH(MatrixF* pOut, float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		const DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, nearZ, farZ);
		DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixTranspose(mProj));
	}

	// Ortho2D は LH も RH もない？　3行3列の成分の符号が違うだけであれば、2D 変換には影響を及ぼさないはず？

	inline void CreateMatrixOrtho2DLH(MatrixF* pOut, float left, float right, float bottom, float top)
	{
		_ASSERTE(!"Not Implemented!!");
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixOrtho2DLH(MatrixF* pOut, float left, float right, float bottom, float top)
	{
		_ASSERTE(!"Not Implemented!!");
	}

	// OpenGL 固定機能（GLU）で言うと、gluOrtho2D() に相当する。
	// GLM で言うと、glm::ortho() に相当する。
	// スクリーン座標を指定したレンダリングに便利な正射影行列だが、
	// D3DX Math, XNA Math, DirectXMath にはなぜか相当のヘルパーがないので自作する必要がある。
	inline void CreateMatrixOrtho2DRH(MatrixF* pOut, float left, float right, float bottom, float top)
	{
		*pOut = IDENTITY_MATRIXF;
		pOut->_11 = 2 / (right - left);
		pOut->_22 = 2 / (top - bottom);
		pOut->_33 = -1;
		pOut->_41 = -(right + left) / (right - left);
		pOut->_42 = -(top + bottom) / (top - bottom);
	}

	//! @brief  転置行列を求めるバージョン。<br>
	inline void CreateTrMatrixOrtho2DRH(MatrixF* pOut, float left, float right, float bottom, float top)
	{
		*pOut = IDENTITY_MATRIXF;
		pOut->_11 = 2 / (right - left);
		pOut->_22 = 2 / (top - bottom);
		pOut->_33 = -1;
		pOut->_14 = -(right + left) / (right - left);
		pOut->_24 = -(top + bottom) / (top - bottom);
	}

	//! @brief  ビューポート行列を作成。<br>
	//! 
	//! スクリーン座標系での交差判定などに使える。<br>
	//! なお、OpenGL や Direct3D では固定機能でもプログラマブル シェーダーでも、<br>
	//! この行列の詳細だけは隠ぺいされ、通常は計算で求めることはなく、ビューポートのパラメータを API で指定する方式になっている。<br>
	//! minZ と maxZ は OpenGL のビューポート パラメータには存在しないが、通例 0 と 1 をそれぞれ指定すればよい。<br>
	inline void CreateMatrixViewport(MatrixF* pOut, float x, float y, float width, float height, float minZ, float maxZ)
	{
		*pOut = IDENTITY_MATRIXF;

		pOut->_11 = +width * 0.5f;
		pOut->_22 = -height * 0.5f;
		pOut->_33 = maxZ - minZ;
		pOut->_14 = x + width * 0.5f;
		pOut->_24 = y + height * 0.5f;
		pOut->_34 = minZ;
	}


	inline Vector3F GetTranslationComponentFromMatrix(const MatrixF* pInMat)
	{
		return Vector3F(pInMat->m[3][0], pInMat->m[3][1], pInMat->m[3][2]);
	}

	inline void RotateVector3ByQuaternion(Vector3F* pOutVec, const Vector3F* pInVec, const QuaternionF* pInQuat)
	{
		DirectX::XMStoreFloat3(pOutVec, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(pInVec), DirectX::XMLoadFloat4(pInQuat)));
	}

	inline void JointQuaternion(QuaternionF* pOutQuat, const QuaternionF* pInQuat1, const QuaternionF* pInQuat2)
	{
		// パラメータの順序に注意。
		// Microsoft.Xna.Framework.Quaternion.Concatenate() 相当。
		// Microsoft.Xna.Framework.Quaternion.op_Multiply() とは逆。
		// DirectX::SimpleMath::Quaternion::Concatenate() とは逆。
		// DirectX::SimpleMath の operator* (const Quaternion& Q1, const Quaternion& Q2) 相当。
		// glm::cross() や glm::detail::operator*(const quat&, const quat&) とは逆。
		// http://blogs.msdn.com/b/ito/archive/2009/04/30/more-bones-03.aspx
		DirectX::XMStoreFloat4(pOutQuat, DirectX::XMQuaternionMultiply(DirectX::XMLoadFloat4(pInQuat1), DirectX::XMLoadFloat4(pInQuat2)));
	}

	inline void CreateConjugateQuaternion(QuaternionF* pOutQuat, const QuaternionF* pInQuat)
	{
#if 0
		pOutQuat->x = -pInQuat->x;
		pOutQuat->y = -pInQuat->y;
		pOutQuat->z = -pInQuat->z;
		pOutQuat->w = +pInQuat->w;
#else
		// SSE を使うと共役クォータニオンは定数との内積で求まるので、命令数が減るか？
		DirectX::XMStoreFloat4(pOutQuat, DirectX::XMQuaternionConjugate(DirectX::XMLoadFloat4(pInQuat)));
#endif
	}

	inline void InverseQuaternion(QuaternionF* pOutQuat, const QuaternionF* pInQuat)
	{
		// 逆クォータニオンは共役クォータニオンをノルム2乗で割ったもの。
		// 単位クォータニオンであれば、共役クォータニオンが逆クォータニオンとなる。
		DirectX::XMStoreFloat4(pOutQuat, DirectX::XMQuaternionInverse(DirectX::XMLoadFloat4(pInQuat)));
	}

	// 行列とクォータニオンの相互変換は、それなりに計算負荷が高いので注意。
	inline void CreateMatrixFromRotationQuaternion(MatrixF* pOutMat, const QuaternionF* pInQuat)
	{
		const DirectX::XMMATRIX mOut = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(pInQuat));
		DirectX::XMStoreFloat4x4(pOutMat, mOut);
	}

	inline void CreateQuaternionFromRotationMatrix(QuaternionF* pOutQuat, const MatrixF* pInMat)
	{
		DirectX::XMStoreFloat4(pOutQuat, DirectX::XMQuaternionRotationMatrix(DirectX::XMLoadFloat4x4(pInMat)));
	}

	// D3DXMatrixDecompose() の代替である XMMatrixDecompose() による行列分解は、スケーリングを含む場合一意性に欠けるので、
	// 座標変換行列に含まれるデータが平行移動と回転のみであることが分かりきっている場合は使わない。

} // end of namespace


namespace MyMath
{
	// 回転と平行移動のみを管理するデュアル クォータニオン情報。
	// 下記を参考に命名＆実装。
	// http://blogs.msdn.com/b/ito/archive/2009/05/05/more-bones-05.aspx
	class QuatTransform final
	{
	public:
		QuaternionF RotationQ;
		Vector3F TranslationV;
		float Dummy; // シェーダー定数パディング用。
	public:
		QuatTransform()
			: RotationQ(NO_ROTATION_QUATERNIONF)
			, TranslationV(ZERO_VECTOR3F)
			, Dummy()
		{}
		QuatTransform(const QuaternionF& rotation, const Vector3F& translation)
			: RotationQ(rotation)
			, TranslationV(translation)
			, Dummy()
		{}
		explicit QuatTransform(const MatrixF& inMat)
			: RotationQ(NO_ROTATION_QUATERNIONF)
			, TranslationV(ZERO_VECTOR3F)
			, Dummy()
		{
			// C++11 委譲コンストラクタは、VC 2012 ではサポートされていない。VC 2013 ではサポートされる予定。
			this->FromMatrix(inMat);
		}
	public:
		void FromMatrix(const MatrixF& inMat)
		{
			CreateQuaternionFromRotationMatrix(&this->RotationQ, &inMat);
			this->TranslationV = GetTranslationComponentFromMatrix(&inMat);
		}
		void ToMatrix(MatrixF& outMat) const
		{
			CreateMatrixFromRotationQuaternion(&outMat, &this->RotationQ);
			outMat._41 = this->TranslationV.x;
			outMat._42 = this->TranslationV.y;
			outMat._43 = this->TranslationV.z;
		}
		static QuatTransform Lerp(const QuatTransform& value1, const QuatTransform& value2, float t)
		{
			QuatTransform newVal;
			SlerpQuaternion(newVal.RotationQ, value1.RotationQ, value2.RotationQ, t);
			LerpVector3(newVal.TranslationV, value1.TranslationV, value2.TranslationV, t);
			return newVal;
		}
		// QuatTransform 内の回転と平行移動の評価順は、「回転した後に平行移動」となっている。
		static QuatTransform Multiply(const QuatTransform& value1, const QuatTransform& value2)
		{
			QuatTransform newVal;
			RotateVector3ByQuaternion(&newVal.TranslationV, &value1.TranslationV, &value2.RotationQ);
			AddVector3(newVal.TranslationV, newVal.TranslationV, value2.TranslationV);
			JointQuaternion(&newVal.RotationQ, &value1.RotationQ, &value2.RotationQ);
			return newVal;
		}
		// 1 に 2 の逆変換を適用する。
		static QuatTransform MultiplyInverse(const QuatTransform& value1, const QuatTransform& value2)
		{
			QuatTransform newVal;
			QuaternionF inv2;
			InverseQuaternion(&inv2, &value2.RotationQ);
			JointQuaternion(&newVal.RotationQ, &value1.RotationQ, &inv2);
			SubtractVector3(newVal.TranslationV, value1.TranslationV, value2.TranslationV);
			RotateVector3ByQuaternion(&newVal.TranslationV, &newVal.TranslationV, &inv2);
			return newVal;
		}
	};

	static_assert(sizeof(QuatTransform) % 16 == 0, "Not aligned!!");
} // end of namespace


// HACK: ボーンは Math ではないので、別の名前空間に移動する。MyPhysics のほうがいい。
namespace MyMath
{
	//! @brief  ボーン影響度情報などのテンプレート。<br>
	template<typename TIndex, typename TWeight> class TInfluencePair final
	{
	public:
		TIndex Index; //!< ターゲットに影響を与えるインデックス。<br>
		TWeight Weight; //!< ターゲットに影響を与えるウェイト（重み）。<br>
	public:
		TInfluencePair()
			: Index(), Weight()
		{}
	public:
		TInfluencePair(TIndex index, TWeight weight)
			: Index(index), Weight(weight)
		{}
	};


	// ローカル ボーン行列を使ってグローバル ボーン行列を計算する場合、
	// 名前文字列もしくはボーン インデックス整数で、ボーン階層を表すデータ構造を作る必要がある。
	// BoneAnimInfo を管理するデータ構造が配列の場合、インデックスで階層を作ったほうがよさげ。
	// BoneAnimInfo を管理するデータ構造が名前をキーとするマップの場合、名前文字列で階層を作ったほうがよさげ。
	// いっそ BoneAnimInfo に ParentIndex と ChildrenIndices を持たせたほうがよいのかも？

#if 0
	class BoneHierarchy final
	{
		class Node
		{
		public:
			int Index;
			int ParentIndex;
			std::vector<int> ChildrenIndices;
		public:
			Node()
				: Index(), ParentIndex()
			{}
		};
		typedef std::shared_ptr<Node> TNodePtr;
	};
#endif


	//! @brief  ボーン スケルトン情報。<br>
	//! あるボーンの親子関係を保持する。<br>
	//! ポインタで連結するよりも、インデックス ベースで関連付け・管理できたほうがシリアライズ・逆シリアライズしやすい。<br>
	class BoneSkeletonInfo final : boost::noncopyable
	{
	public:
		typedef std::shared_ptr<BoneSkeletonInfo> TSharedPtr;
		typedef std::shared_ptr<const BoneSkeletonInfo> TConstSharedPtr;
	public:
		static const int InvalidBoneIndex = -1;
	private:
		std::wstring m_strBoneName; //!< ボーン名。<br>

		int m_parentBoneIndex = InvalidBoneIndex;
		TIntArray m_childrenBoneIndices;

	public:
		BoneSkeletonInfo()
			//: m_parentBoneIndex(InvalidBoneIndex)
		{
		}

		//! @brief  ボーン名を設定する。<br>
		void SetBoneName(const char* pName) { m_strBoneName = MyUtils::SafeConvertUtf8toUtf16(pName); }
		void SetBoneName(const wchar_t* pName) { m_strBoneName = pName ? pName : L""; }

		const std::wstring& GetBoneNameW() const { return m_strBoneName; }

		void SetParentBoneIndex(int index) { m_parentBoneIndex = index; }
		int GetParentBoneIndex() const { return m_parentBoneIndex; }
		// 親がいなければルートとみなす。
		bool GetIsRootBone() const { return m_parentBoneIndex == InvalidBoneIndex; }

		void AddChildBoneIndex(int index) { m_childrenBoneIndices.push_back(index); }
		const TIntArray& GetChildrenBoneIndices() const { return m_childrenBoneIndices; }
		// 子がいなければリーフとみなす。
		bool GetIsLeafBone() const { return m_childrenBoneIndices.empty(); }
	};

	typedef std::vector<BoneSkeletonInfo::TSharedPtr> TBoneSkeletonInfoPtrsArray;
	//typedef std::unordered_map<std::wstring, BoneSkeletonInfo::TSharedPtr> TStrToBoneSkeletonInfoPtrMap;


	class BoneInitialPoseInfo final
	{
	private:
		MyMath::MatrixF m_globalInitAttitudeMatrix; //!< グローバル初期姿勢行列 Gini。<br>
		MyMath::MatrixF m_localInitAttitudeMatrix; //!< ローカル初期姿勢行列 Lini。<br>
		MyMath::QuatTransform m_globalInitAttitudeQuat;
		MyMath::QuatTransform m_localInitAttitudeQuat;
		// シェーダー定数節約のためのデュアル クォータニオン用データ（スケーリング情報なし）も事前計算しておく。
		// クォータニオンであれば、フレーム間・モーション間で球面線形補間できることもメリット。
	public:
		BoneInitialPoseInfo()
			: m_globalInitAttitudeMatrix(MyMath::ZERO_MATRIXF)
			, m_localInitAttitudeMatrix(MyMath::ZERO_MATRIXF)
		{
		}

	public:
		const MyMath::MatrixF& GetGlobalInitAttitudeMatrix() const { return m_globalInitAttitudeMatrix; }
		void SetGlobalInitAttitudeMatrix(const MyMath::MatrixF& mat)
		{
			m_globalInitAttitudeMatrix = mat;
			m_globalInitAttitudeQuat.FromMatrix(mat);
		}

		const MyMath::MatrixF& GetLocalInitAttitudeMatrix() const { return m_localInitAttitudeMatrix; }
		void SetLocalInitAttitudeMatrix(const MyMath::MatrixF& mat)
		{
			m_localInitAttitudeMatrix = mat;
			m_localInitAttitudeQuat.FromMatrix(mat);
		}

		const MyMath::QuatTransform& GetGlobalInitAttitudeQuat() const { return m_globalInitAttitudeQuat; }
		//void SetGlobalInitAttitudeQuat(const MyMath::QuatTransform& quat) { m_globalInitAttitudeQuat = quat; }

		const MyMath::QuatTransform& GetLocalInitAttitudeQuat() const { return m_localInitAttitudeQuat; }
		//void SetLocalInitAttitudeQuat(const MyMath::QuatTransform& quat) { m_localInitAttitudeQuat = quat; }
	};

	typedef std::vector<BoneInitialPoseInfo> TBoneInitialPoseInfoArray;


	//! @brief  ボーン アニメーション情報。<br>
	//! 
	//! 複数のモーションをパーツ別（ボーン別）にブレンドするとき、どのモーションの初期姿勢をベースとするかによって結果が変わってくるはず。<br>
	//! XNA のアニメーション ライブラリ、XNA ACL はこのパーツ別ブレンドをサポートしているが、初期姿勢の設定は特になかった？<br>
	//! 独自のスキンメッシュ ファイル フォーマットを設計する場合、情報の過不足がないようにする必要がある。<br>
	class BoneAnimInfo final : boost::noncopyable
	{
	public:
		typedef std::shared_ptr<BoneAnimInfo> TSharedPtr;
		typedef std::shared_ptr<const BoneAnimInfo> TConstSharedPtr;
	private:
		//std::wstring m_strBoneName; //!< ボーン名。<br>
		//std::wstring m_strAnimTrackName; //!< アニメーション トラック名。<br>
		//MyMath::MatrixF m_globalInitAttitudeMatrix; //!< グローバル初期姿勢行列 Gini。<br>
		//MyMath::MatrixF m_localInitAttitudeMatrix; //!< ローカル初期姿勢行列 Lini。<br>
		//MyMath::QuatTransform m_globalInitAttitudeQuat;
		//MyMath::QuatTransform m_localInitAttitudeQuat;

		//! @brief  （該当するボーンに対する）フレームごとのグローバル（絶対）姿勢行列 Gn。要素数はフレーム数。<br>
		std::vector<MyMath::MatrixF> m_globalFrameAttitudeMatrices;
		//! @brief  （該当するボーンに対する）フレームごとのローカル（相対）姿勢行列 Ln。要素数はフレーム数。<br>
		//! 自身の G に親の G^-1 すなわちボーン オフセット行列をかけることで求まるはず。Gini を考慮しないといけない？<br>
		std::vector<MyMath::MatrixF> m_localFrameAttitudeMatrices;

		std::vector<MyMath::QuatTransform> m_globalFrameAttitudeQuats;
		std::vector<MyMath::QuatTransform> m_localFrameAttitudeQuats;

		//std::vector<MyMath::MatrixF> m_invGlobalAttMatricesPerFrame; //!< フレームごとのグローバル姿勢行列の逆行列 G^-1。<br>

	public:
		BoneAnimInfo()
			//: m_globalInitAttitudeMatrix(MyMath::ZERO_MATRIXF)
			//, m_localInitAttitudeMatrix(MyMath::ZERO_MATRIXF)
		{
		}

#if 0
		//! @brief  ボーン名を設定する。<br>
		void SetBoneName(const char* pName) { m_strBoneName = MyUtils::SafeConvertUtf8toUtf16(pName); }
		void SetBoneName(const wchar_t* pName) { m_strBoneName = pName ? pName : L""; }

		const std::wstring& GetBoneNameW() const { return m_strBoneName; }

		void SetAnimTrackName(const char* pName) { m_strAnimTrackName = MyUtils::SafeConvertUtf8toUtf16(pName); }
		void SetAnimTrackName(const wchar_t* pName) { m_strAnimTrackName = pName ? pName : L""; }

		const std::wstring& GetAnimTrackNameW() const { return m_strAnimTrackName; }
#endif

		//const MyMath::MatrixF& GetGlobalInitAttitudeMatrix() const { return m_globalInitAttitudeMatrix; }
		//void SetGlobalInitAttitudeMatrix(const MyMath::MatrixF& mat) { m_globalInitAttitudeMatrix = mat; }

		//const MyMath::MatrixF& GetLocalInitAttitudeMatrix() const { return m_localInitAttitudeMatrix; }
		//void SetLocalInitAttitudeMatrix(const MyMath::MatrixF& mat) { m_localInitAttitudeMatrix = mat; }

		//const MyMath::QuatTransform& GetGlobalInitAttitudeQuat() const { return m_globalInitAttitudeQuat; }
		//void SetGlobalInitAttitudeQuat(const MyMath::QuatTransform& quat) { m_globalInitAttitudeQuat = quat; }

		//const MyMath::QuatTransform& GetLocalInitAttitudeQuat() const { return m_localInitAttitudeQuat; }
		//void SetLocalInitAttitudeQuat(const MyMath::QuatTransform& quat) { m_localInitAttitudeQuat = quat; }

		void AddGlobalFrameAttitudeMatrix(const MyMath::MatrixF& mat)
		{
			_ASSERTE(m_globalFrameAttitudeMatrices.size() == m_globalFrameAttitudeQuats.size());
			m_globalFrameAttitudeMatrices.push_back(mat);
			m_globalFrameAttitudeQuats.push_back(MyMath::QuatTransform(mat));
		}

		void AddLocalFrameAttitudeMatrix(const MyMath::MatrixF& mat)
		{
			_ASSERTE(m_localFrameAttitudeMatrices.size() == m_localFrameAttitudeQuats.size());
			m_localFrameAttitudeMatrices.push_back(mat);
			m_localFrameAttitudeQuats.push_back(MyMath::QuatTransform(mat));
		}

		const std::vector<MyMath::MatrixF>& GetGlobalFrameAttitudeMatrices() const
		{ return m_globalFrameAttitudeMatrices; }

		const std::vector<MyMath::MatrixF>& GetLocalFrameAttitudeMatrices() const
		{ return m_localFrameAttitudeMatrices; }

		const std::vector<MyMath::QuatTransform>& GetGlobalFrameAttitudeQuats() const
		{ return m_globalFrameAttitudeQuats; }

		const std::vector<MyMath::QuatTransform>& GetLocalFrameAttitudeQuats() const
		{ return m_localFrameAttitudeQuats; }

		//void AddGlobalFrameAttitudeQuat(const MyMath::QuatTransform& quat)
		//{ m_globalFrameAttitudeQuats.push_back(quat); }

		//void AddLocalFrameAttitudeQuat(const MyMath::QuatTransform& quat)
		//{ m_localFrameAttitudeQuats.push_back(quat); }

#if 0
		void AddInvGlobalAttitudeMatrixPerFrame(const MyMath::MatrixF& mat)
		{ m_invGlobalAttMatricesPerFrame.push_back(mat); }

		const std::vector<MyMath::MatrixF>& GetInvGlobalAttitudeMatrixPerFrame() const
		{ return m_invGlobalAttMatricesPerFrame; }
#endif
	};

	typedef std::vector<BoneAnimInfo::TSharedPtr> TBoneAnimInfoPtrsArray;
	typedef std::vector<std::shared_ptr<TBoneAnimInfoPtrsArray>> TBoneAnimInfoPtrsArrayPtrsArray;
	//typedef std::unordered_map<std::wstring, BoneAnimInfo::TSharedPtr> TStrToBoneAnimInfoPtrMap;


	// SpecularPower は GL_SHININESS [0, 128] や D3DMATERIAL9.Power [0, +∞] のような昔ながらの固定機能における反射強度を想定している。
	// [0, 1] に正規化されていないので注意。
	// https://msdn.microsoft.com/en-us/library/dd373944.aspx
	// https://msdn.microsoft.com/ja-jp/library/ee422436.aspx

	const float MinSpecularPowerValue = 1.0e-5f;
	const float MaxSpecularPowerValue = 100;


	class MyGradientColorStop final
	{
	public:
		float Offset; // 0.0～1.0 の範囲。
		ColorRgba Color;
	public:
		MyGradientColorStop()
			: Offset()
			, Color(ColorRgbaWhite)
		{}
		MyGradientColorStop(float offset, ColorRgba color)
			: Offset(offset)
			, Color(color)
		{}
	public:
		bool operator ==(const MyGradientColorStop& other) const
		{
			return
				this->Offset == other.Offset &&
				this->Color == other.Color;
		}

		bool operator !=(const MyGradientColorStop& other) const
		{
			return !(*this == other);
		}
	};

	class MyGradientOpacityStop final
	{
	public:
		float Offset; // 0.0～1.0 の範囲。
		float Opacity;
	public:
		MyGradientOpacityStop()
			: Offset()
			, Opacity(1)
		{}
		MyGradientOpacityStop(float offset, float opacity)
			: Offset(offset)
			, Opacity(opacity)
		{}
	public:
		bool operator ==(const MyGradientOpacityStop& other) const
		{
			return
				this->Offset == other.Offset &&
				this->Opacity == other.Opacity;
		}

		bool operator !=(const MyGradientOpacityStop& other) const
		{
			return !(*this == other);
		}
	};

	typedef std::vector<MyGradientColorStop> TMyGradientColorStopArray;
	typedef std::vector<MyGradientOpacityStop> TMyGradientOpacityStopArray;


	extern bool CompareGradientColorStopArrays(const TMyGradientColorStopArray& a, const TMyGradientColorStopArray& b);
	extern bool CompareGradientOpacityStopArrays(const TMyGradientOpacityStopArray& a, const TMyGradientOpacityStopArray& b);


	enum MySpecularType
	{
		MySpecularType_None,
		MySpecularType_BlinnPhong,
		MySpecularType_CookTorrance,
	};

	enum MyReflexType
	{
		MyReflexType_None,
		MyReflexType_ReflectOnly,
		MyReflexType_RefractOnly,
		MyReflexType_Fresnel,
	};


	//! @brief  旧 D3DMATERIAL9 のようなマテリアル集合。ただし POD 型ではない。<br>
	//! 
	//! 他にもいずれ扱うパラメータ情報が増える予定なので、カプセル化を行ない、また主にスタックでなくヒープ上に作成することを想定している。<br>
	//! 他のメッシュ ファイル フォーマットに依存しない、独自のメッシュ ファイル形式におけるシリアライズ対象クラス。<br>
	//! シェーダープログラムではこのマテリアル パッケージから必要なデータを取得して GPU へ送る。<br>
	class MyMaterial final
	{
	public:
		typedef std::shared_ptr<MyMaterial> TSharedPtr;
		typedef std::shared_ptr<const MyMaterial> TConstSharedPtr;

	private:
		Vector4F m_diffuse; // xyz は ColorRGB、w は Level 値（Factor 値）。
		Vector4F m_ambient; // xyz は ColorRGB、w は Level 値（Factor 値）。
		Vector4F m_specular; // xyz は ColorRGB、w は Level 値（Factor 値）。
		Vector4F m_emissive; // xyz は ColorRGB、w は Level 値（Factor 値）。
		//float m_transparency; // オフライン レンダリングでは透明度のほうが一般的だが、リアルタイム レンダリングでは Transparency よりも Opacity のほうが一般的。
		Vector4F m_opacity;
		float m_specularPower;
		float m_roughness;
		float m_reflectivity;
		float m_indexOfRefraction;
		float m_translucency; // 半透明度。SSS で使用する。
		MySpecularType m_specularType;
		MyReflexType m_reflexType;

		// HACK: 実際に屈折マッピングするには隣接マテリアルとの屈折率比ηが必要となるため、
		// メッシュのマテリアルとしての屈折率だけをシェーダーに送っても意味がない。
		// http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20081220
		// 屈折率が1の空気中に存在する透明物質のレンダリングであればマテリアルの屈折率だけでもいいが、
		// 例えばもしガラス素材のオブジェクトが水中にある場合、ガラスの屈折率 1.5 を送った上で、
		// 隣接環境（水）の屈折率 1.3333 との比を使って屈折計算を行なう。
		// なお、色付きのガラスや液体などの透明物質用に、Transparent Color を別途用意しておく。
		// （Transparent Color は Shade や FBX には存在するが、LightWave や Metasequoia には存在しない）
		// ちなみに Maya も LightWave も、スペキュラーハイライトは透明度に影響を受けない。
		// http://me.autodesk.jp/wam/maya/docs/Maya2010/index.html?url=Shading_Nodes_Common_surface_material_attributes.htm,topicNumber=d0e522117
		// http://shade.e-frontier.co.jp/special/archives/maiko/lesson_003/f_003.html
		// スペキュラーの色は、特に金属の場合は地の色に左右される。Cook-Torrance などでモデル化する際に必要となる。

		bool m_sproutsFur; //!< ファーを生やすか否か。<br>

		// Photoshop のグラデーション エディタでは ColorRGB と Opacity の GradientStop を別々に制御できるが、
		// トゥーン シェーディングのグラデーションは色だけでよい。
		TMyGradientColorStopArray m_toonGradientColorStops;

	private:
		std::wstring m_strName; //!< マテリアル名。<br>
		std::wstring m_strTexFileNameDiffuseMap;
		std::wstring m_strTexFileNameNormalMap;
		// Windows 環境でアプリケーションの内部文字列エンコードに std::string と UTF-8 を使うことはできなくもないが、
		// Visual Studio デバッガーなどの対応がされていないなどの問題があるため、ワイド文字列 std::wstring を使う。
		// FBX SDK 経由で取得できるマルチバイト文字列はおそらく UTF-8 エンコード。
		// Windows 環境では UTF-16LE に直す処理が必要。

	public:
		MyMaterial()
			: m_diffuse(MyMath::ZERO_VECTOR4F)
			, m_ambient(MyMath::ZERO_VECTOR4F)
			, m_specular(MyMath::ZERO_VECTOR4F)
			, m_emissive(MyMath::ZERO_VECTOR4F)
			, m_opacity(MyMath::COLOR4F_WHITE)
			, m_specularPower(MinSpecularPowerValue)
			, m_roughness()
			, m_reflectivity()
			, m_indexOfRefraction(1)
			, m_translucency()
			, m_specularType()
			, m_reflexType()
			, m_sproutsFur()
		{}

		// 内部実装をいつでも変更できるように、あえて const & にはしないで値型を返す。
	public:
		Vector4F GetDiffuse() const { return m_diffuse; }
		Vector4F GetAmbient() const { return m_ambient; }
		Vector4F GetSpecular() const { return m_specular; }
		Vector4F GetEmissive() const { return m_emissive; }
		Vector4F GetOpacity() const { return m_opacity; }

		void SetDiffuse(const Vector4F& inVal) { m_diffuse = inVal; }
		void SetAmbient(const Vector4F& inVal) { m_ambient = inVal; }
		void SetSpecular(const Vector4F& inVal) { m_specular = inVal; }
		void SetEmissive(const Vector4F& inVal) { m_emissive = inVal; }

		void SetDiffuseColorRGB(const Vector3F& inVal) { SetVector4XYZ(m_diffuse, inVal); }
		void SetAmbientColorRGB(const Vector3F& inVal) { SetVector4XYZ(m_ambient, inVal); }
		void SetSpecularColorRGB(const Vector3F& inVal) { SetVector4XYZ(m_specular, inVal); }
		void SetEmissiveColorRGB(const Vector3F& inVal) { SetVector4XYZ(m_emissive, inVal); }
		void SetTransparentColorRGB(const Vector3F& inVal) { SetVector4XYZ(m_opacity, inVal); }

		void SetDiffuseLevel(float inVal) { m_diffuse.w = inVal; }
		void SetAmbientLevel(float inVal) { m_ambient.w = inVal; }
		void SetSpecularLevel(float inVal) { m_specular.w = inVal; }
		void SetEmissiveLevel(float inVal) { m_emissive.w = inVal; }

		float GetTransparency() const { return 1 - m_opacity.w; }
		void SetTransparency(float val) { m_opacity.w = 1 - val; }

		float GetOpacityAlpha() const { return m_opacity.w; }
		void SetOpacityAlpha(float val) { m_opacity.w = val; }

		float GetReflectivity() const { return m_reflectivity; }
		void SetReflectivity(float val) { m_reflectivity = val; }

		float GetIndexOfRefraction() const { return m_indexOfRefraction; }
		void SetIndexOfRefraction(float val) { m_indexOfRefraction = val; }

		float GetTranslucency() const { return m_translucency; }
		void SetTranslucency(float val) { m_translucency = val; }

		MySpecularType GetSpecularType() const { return m_specularType; }
		void SetSpecularType(MySpecularType val) { m_specularType = val; }

		MyReflexType GetReflexType() const { return m_reflexType; }
		void SetReflexType(MyReflexType val) { m_reflexType = val; }

		float GetSpecularPower() const { return m_specularPower; }
		// ゼロは無効（HLSL の pow() 計算結果が不正になる）なので、クランプ。
		void SetSpecularPower(float val) { m_specularPower = MyUtils::Clamp(val, MinSpecularPowerValue, MaxSpecularPowerValue); }

		float GetRoughness() const { return m_roughness; }
		void SetRoughness(float val) { m_roughness = val; }

		bool GetSproutsFur() const { return m_sproutsFur; }
		void SetSproutsFur(bool val) { m_sproutsFur = val; }

		TMyGradientColorStopArray& GetToonGradientColorStops() { return m_toonGradientColorStops; }
		const TMyGradientColorStopArray& GetToonGradientColorStops() const { return m_toonGradientColorStops; }

		const std::wstring& GetMaterialName() const { return m_strName; }
		void SetMaterialName(const wchar_t* pName) { m_strName = pName ? pName : L""; }

		const std::wstring& GetTexFileNameDiffuseMap() const { return m_strTexFileNameDiffuseMap; }
		void SetTexFileNameDiffuseMap(const wchar_t* pName) { m_strTexFileNameDiffuseMap = pName ? pName : L""; }

		const std::wstring& GetTexFileNameNormalMap() const { return m_strTexFileNameNormalMap; }
		void SetTexFileNameNormalMap(const wchar_t* pName) { m_strTexFileNameNormalMap = pName ? pName : L""; }
	};

	//typedef std::shared_ptr<MyMaterial> TMyMaterialPtr;
	typedef std::vector<MyMaterial::TSharedPtr> TMyMaterialPtrsArray;
	typedef std::unordered_map<std::wstring, MyMath::MyMaterial::TSharedPtr> TMyNameToMaterialTable;


	class MyGlobalMaterialTable final
	{
	public:
		static const int InvalidMaterialIndex = -1;
	private:
		MyMath::TMyNameToMaterialTable m_nameToMaterialTable;
		TStrToIntMap m_nameToMaterialIndexTable;
		MyMath::TMyMaterialPtrsArray m_materialsArray;
		// シーン内のすべてのマテリアルを一元管理できるようにするためのテーブルと配列。
		// マテリアル オブジェクト自体はポインタ経由でシャローコピーされるだけなので、テーブルと配列とで共有される。
		// 名前で検索する場合はテーブルが便利だが、
		// Metasequoia や LightWave のようにリスト ビューで表示する場合、テーブルよりも配列化されていたほうが都合がよい。
		// std::advance() を使って unordered_map に疑似ランダム アクセスすることもできるが、
		// unordered_map は順序不定なので、マテリアルの名前でソートした配列を別途リスト表示用に作っておいたほうがよい。
		// トゥーン シェーディング グラデーション参照テクスチャを作成し、描画時にライン番号（V 座標）を指定するときにも使える。
		// ちなみに Metasequoia での正確なマテリアル順序は FBX にコンバートしたときに失われてしまうので、再現不可能。
		// メッシュごとのマテリアル順序（ローカル インデックス）を維持したまま、モデル全体に共通するグローバル インデックスを振り直すことはできるが……
		// LightWave エクスポートした場合も名前順ソートされた状態になる模様。
		// なお、さらに最適化するならば、unordered_map の代わりに boost::flat_map を使うことを検討していいかも。
		// flat_map は Loki の AssocVector と同じアルゴリズムらしい。
		// なお、unordered_map は空間的オーバーヘッドにより vector よりも std::advance() によるインデックス アクセスは遅くなると思われるが、
		// unordered_map の探索時間自体 vector 並みに高速（理論的には定数時間）なのでマテリアルの数があまりに多くなければ無視できるレベルのはず。
		// それよりもリスト表示する際に順序が不定なことのほうが問題。

		// NOTE: もし要素数を変更するような操作を行なう場合、必ずラッパーメソッドを用意して、
		// テーブルと配列が確実に連動するようにし、さらに関連するビュー（ペイン）に変更通知を出せるようにする。

		// 現在選択されているマテリアル（ターゲット）を表すインデックス。
		// 複数選択には非対応。
		int m_currentTargetMaterialIndex = InvalidMaterialIndex;

	public:
		MyGlobalMaterialTable()
			//: m_currentTargetMaterialIndex(InvalidMaterialIndex)
		{}

	public:
		//const MyMath::TMyNameToMaterialTable& GetGlobalNameToMaterialTable() const { return m_nameToMaterialTable; }
		//const TStrToIntMap& GetGlobalNameToMaterialIndexTable() const { return m_nameToMaterialIndexTable; }
		int GetGlobalMaterialIndexByName(const std::wstring& strName) const
		{
			auto it = m_nameToMaterialIndexTable.find(strName);
			if (it != m_nameToMaterialIndexTable.end())
			{
				return it->second;
			}
			return InvalidMaterialIndex;
		}
	public:
		const MyMath::TMyMaterialPtrsArray& GetGlobalMaterialsArray() const { return m_materialsArray; }
	public:
		const MyMath::MyMaterial* GetCurrentTargetMaterial() const
		{
			return
				(0 <= m_currentTargetMaterialIndex && size_t(m_currentTargetMaterialIndex) < m_materialsArray.size())
				? m_materialsArray[m_currentTargetMaterialIndex].get() : nullptr;
		}
		MyMath::MyMaterial* GetCurrentTargetMaterial()
		{ return const_cast<MyMath::MyMaterial*>(static_cast<const MyGlobalMaterialTable&>(*this).GetCurrentTargetMaterial()); }

	public:
		void SetTargetMaterialIndex(int index)
		{
			m_currentTargetMaterialIndex = index;
		}
		const MyMath::MyMaterial* GetGlobalMaterialByIndex(int index) const
		{
			if (0 <= index && static_cast<size_t>(index) < m_materialsArray.size())
			{
				return m_materialsArray[index].get();
			}
			return nullptr;
		}

	public:
		void CreateGlobalMaterialsArray(const TMyNameToMaterialTable& nameToMaterialTable)
		{
			m_nameToMaterialTable = nameToMaterialTable;

			// マップと配列を同期する。
			m_currentTargetMaterialIndex = InvalidMaterialIndex;
			m_materialsArray.clear();
			for (auto m : m_nameToMaterialTable)
			{
				m_materialsArray.push_back(m.second);
			}
			typedef MyMath::MyMaterial::TConstSharedPtr TSortTarget;
			// 名前順でソート。
			std::sort(m_materialsArray.begin(), m_materialsArray.end(),
				[](const TSortTarget& x, const TSortTarget& y) { return x->GetMaterialName() < y->GetMaterialName(); });

			for (size_t i = 0; i < m_materialsArray.size(); ++i)
			{
				m_nameToMaterialIndexTable[m_materialsArray[i]->GetMaterialName()] = static_cast<int>(i);
			}
		}

		void ClearGlobalMaterialsArray()
		{
			m_currentTargetMaterialIndex = InvalidMaterialIndex;
			m_materialsArray.clear();
			m_nameToMaterialTable.clear();
			m_nameToMaterialIndexTable.clear();
		}
	};


	//! @brief  D3DX10_ATTRIBUTE_RANGE の代わり。<br>
	class MyAttributeRange
	{
	public:
		uint32_t  AttribId;
		uint32_t  FaceStart;
		uint32_t  FaceCount;
		uint32_t  VertexStart;
		uint32_t  VertexCount;

	public:
		MyAttributeRange()
			: AttribId()
			, FaceStart()
			, FaceCount()
			, VertexStart()
			, VertexCount()
		{}
	};

	typedef std::vector<MyAttributeRange> TMyAttributeRangeArray;


	const uint32_t OneQuadIndexCount = 6U;
	const uint16_t OneQuadIndicesArray012[OneQuadIndexCount] =
	{
		0, 1, 2, 2, 1, 3, // 頂点定義が (LT, RT, LB, RB) の場合、CW になる。
	};
	const uint16_t OneQuadIndicesArray021[OneQuadIndexCount] =
	{
		0, 2, 1, 1, 2, 3, // 頂点定義が (LT, RT, LB, RB) の場合、CCW になる。
	};

} // end of namespace


namespace MyMath
{
	class RectF
	{
	public:
		float X, Y, Width, Height;
	public:
		RectF()
			: X(), Y(), Width(), Height() {}

		RectF(float x, float y, float width, float height)
		: X(x), Y(y), Width(width), Height(height) {}

		float GetLeft() const { return this->X; }
		float GetTop() const { return this->Y; }
		float GetRight() const { return this->X + this->Width; }
		float GetBottom() const { return this->Y + this->Height; }
	};

	//typedef std::map<wchar_t, RectF> TCharCodeUVMap;
	//typedef std::unordered_map<wchar_t, RectF> TCharCodeUVMap;

	typedef std::vector<RectF> TCharCodeUVMap;

	// 0x00～0x7f の ASCII だけならば、map でなく文字コードをインデックスとする vector のほうが高速で省メモリ。
	// 多言語対応するならば unordered_map にしてハッシュ管理したほうがよい。ただし UTF-16 はサロゲート ペアがあるので注意。
	// サロゲート ペアまで考慮した上でマップを使って管理する場合、キーを wchar_t でなく文字列にするか、2 個の wchar_t を保持する構造体を使う。
	// また、Unicode 結合文字のことまで考慮するならば、あらかじめ全文字が描かれたテクスチャを用意するのはあきらめて、
	// 素直に DirectWrite などのプラットフォーム API を使ってフォント メトリクス データが必要になったタイミングで
	// 動的にテクスチャ DIB を生成してテクスチャ内容を書き換える手法をとったほうがよい。
	// イベント シーンなどで会話テキストが多用される場面では、フォント メトリクスを使って都度テクスチャと UV マップを生成する手法を採るとよい。
	// 一方、HUD などで繰り返し使用され、文字種別が ASCII ＋かな／カナのみなどに制限されていたり、フォント サイズが固定だったりする場合は
	// シーン初期化時に生成した固定テクスチャと UV マップを使いまわす方法を採るとよい。

} // end of namespace


namespace MyMath
{
	extern void CreateGaussianWeightsArray(float outArray[], size_t arraySize, size_t kernelHalf, float dispersion);


	//! @brief  Xorshift による乱数を提供するクラス。<br>
	//! 
	//! シフト演算の結果が言語／処理系に依存しない uint で行なうのがコツ。<br>
	//! HLSL にほぼコピー＆ペーストで移植できるように、互換性のある形にしておく。<br>
	//! HLSL には Perlin ノイズを生成する noise() 組み込み関数が一応あるが、<br>
	//! テクスチャ シェーダーおよび D3DX9 専用でしかも動的な用途ではない。<br>
	//! GLSL にも規格で noise() 関数が定義されてあるが、実際に実装しているドライバーがないらしい？<br>
	//! GPU で乱数をリアルタイムに動的生成する場合、なんらかのアルゴリズムを自前で実装する必要がある。<br>
	class Xorshift128Random final
	{
		// ちなみに OpenCL-C には C/C++ でおなじみの FLT_EPSILON や INT_MAX が事前定義されているらしい。
	public:
		static const int IntMax = 0x7FFFFFFF;
		static const uint UintMax = 0xFFFFFFFFU;

		// 経験的な値。根拠はない。
		static const uint MaxVal = 10000;
	public:
		static uint4 CreateNext(uint4 random)
		{
			const uint t = (random.x ^ (random.x << 11));
			random.x = random.y;
			random.y = random.z;
			random.z = random.w;
			random.w = (random.w = (random.w ^ (random.w >> 19)) ^ (t ^ (t >> 8)));
			return random;
		}

		static uint GetRandomComponentUI(uint4 random)
		{
			return random.w;
		}

		// 0.0～1.0 の範囲の実数乱数にする。
		static float GetRandomComponentUF(uint4 random)
		{
			//return float(GetRandomComponentUI(random)) / float(UintMax)); // NG.
			return float(GetRandomComponentUI(random) % MaxVal) / float(MaxVal);
		}

		// -1.0～+1.0 の範囲の実数乱数にする。
		static float GetRandomComponentSF(uint4 random)
		{
			return 2.0f * GetRandomComponentUF(random) - 1.0f;
		}

		// 1要素を符号なし 32bit 整数とした場合の Xorshift 乱数の最大値は、理論的には UINT32_MAX だが、
		// 周期が非常に長いため、逆に短い期間の乱数を [0.0, 1.0] 範囲の実数乱数に変換するときはまず比較的小さな数で剰余を求める。

		static uint4 CreateInitialNumber(uint seed)
		{
			// Xorshift の初期値は「すべてゼロ」でなければ何でも良いらしい。
			// http://d.hatena.ne.jp/jetbead/20110912/1315844091
			// http://ogawa-sankinkoutai.seesaa.net/article/108848981.html
			// http://meme.biology.tohoku.ac.jp/klabo-wiki/index.php?%B7%D7%BB%BB%B5%A1%2FC%2B%2B

			if (seed == 0)
			{
				seed += 11;
			}

			uint4 temp;

			temp.w = seed;
			temp.x = (seed << 16) + (seed >> 16);
			temp.y = temp.w + temp.x;
			temp.z = temp.x ^ temp.y;

			return temp;
		}
	};

} // end of namespace
