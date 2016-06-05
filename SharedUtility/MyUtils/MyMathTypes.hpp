#pragma once


namespace MyMath
{
#pragma region // typedefs //

	// FBX SDK の組込の Vector, Matrix, Quaternion はメンバが double なので、D3D や GLSL で直接扱うには不向き。
	// 編集目的でなければ、FBX ファイルを読み込んだタイミングで double --> float 変換を行なっておいたほうがよい。

	// DirectXMath のヘッダーを見るかぎり、
	// _M_AMD64, _M_IX86, _M_ARM,
	// あるいは _M_PPCBE (PowerPC Big Endian) のいずれかが定義されていれば、
	// Windows/Xbox 360 以外のプラットフォームでもそのまま利用できる可能性がある。
	// iOS や Android 上の OpenGL 開発にも GLM の代わりに使えるかもしれない。

	// HACK: D3DX Math/XNA Math/DirectXMath の型のデフォルト コンストラクタは怠惰なため、
	// それらを含む型はきちんとコンストラクタを定義してゼロ初期化しておいたほうがよい。
	// DirectXTK 付属の SimpleMath にて定義された派生型は、きちんとしたコンストラクタを持っているので、そちらを使う。
	// 他の派生クラスが定義されていない整数ベクトル型などに関しても、後で派生型に置き換えられるように一応 typedef しておく。

	//typedef D3DXVECTOR2 Vector2F;
	//typedef D3DXVECTOR3 Vector3F;
	//typedef D3DXVECTOR4 Vector4F;
	//typedef D3DXMATRIX MatrixF;
	//typedef D3DXCOLOR Color4F;
	//typedef D3DXQUATERNION QuaternionF;

	typedef DirectX::XMINT2 Vector2I;
	typedef DirectX::XMINT3 Vector3I;
	typedef DirectX::XMINT4 Vector4I;

	typedef DirectX::XMUINT2 Vector2UI;
	typedef DirectX::XMUINT3 Vector3UI;
	typedef DirectX::XMUINT4 Vector4UI;

#if 0
	typedef DirectX::XMFLOAT2 Vector2F;
	typedef DirectX::XMFLOAT3 Vector3F;
	typedef DirectX::XMFLOAT4 Vector4F;
	typedef DirectX::XMFLOAT4 QuaternionF;

	typedef DirectX::XMFLOAT4X4 MatrixF;
	typedef DirectX::XMFLOAT4X4 Matrix4x4F;
	typedef DirectX::XMFLOAT4X3 Matrix4x3F;
#else
	typedef DirectX::SimpleMath::Vector2 Vector2F;
	typedef DirectX::SimpleMath::Vector3 Vector3F;
	typedef DirectX::SimpleMath::Vector4 Vector4F;
	typedef DirectX::SimpleMath::Quaternion QuaternionF;

	typedef DirectX::SimpleMath::Matrix MatrixF;
	typedef DirectX::SimpleMath::Matrix Matrix4x4F;
	typedef DirectX::XMFLOAT3X3 Matrix3x3F;
	typedef DirectX::XMFLOAT4X3 Matrix4x3F;
#endif

#pragma region // HLSL 互換型。グローバル名前空間では定義しない。 //
	typedef Vector2I int2;
	typedef Vector3I int3;
	typedef Vector4I int4;

	typedef uint32_t uint;
	typedef Vector2UI uint2;
	typedef Vector3UI uint3;
	typedef Vector4UI uint4;

	typedef Vector2F float2;
	typedef Vector3F float3;
	typedef Vector4F float4;

	typedef MatrixF matrix;
	typedef Matrix4x4F float4x4;
#pragma endregion

	// 列優先の 4x3 は行優先の 3x4 と比べてキャッシュ効率がよくない。使うことはおそらくない。
	// ……と思っていたが、HLSL のデフォルトは列優先（column_major）で、実際は 3x4 より 4x3 のほうがレジスタを節約できるらしい。
	// DirectXMath の XMFLOAT4X3 はそのためのストレージ型らしい。
	// ただし CPU で（SSE を使わずに）ベタに計算する際にそのまま使う場合、4x3 がキャッシュ ヒット的に非効率なのは確か。
	// http://social.msdn.microsoft.com/Forums/ja-JP/0d4427c1-ae7d-43a8-8ce3-55371121e50b/float4x4?forum=xnagameja

	//typedef DirectX::XMFLOAT4 Color4F;
	//typedef DirectX::PackedVector::XMCOLOR ColorBgra;

	// DirectXMath には XNA Math の XMCOLOR (BGRA) に相当するものが存在しないらしい。
	// 必要であれば別途構造体を作ったほうが良い。
	// ……と思っていたが、DirectX::PackedVector::XMCOLOR という相当型が用意されていた。

	// XMVECTOR, XMMATRIX は SSE ベースの途中計算で使用する目的の、基本的にスタックで確保するべき非透過の変数型。
	// なお、XMFLOAT4 や XMFLOAT4X4 には最低限の演算子オーバーロードしか定義されていないが、
	// XMVECTOR, XMMATRIX には算術用途のものも定義されている。
	// XMFLOAT4 や XMFLOAT4X4 はメンバー単位でアクセスしやすいのがメリット。透過型なのでストレージ用途に向いている。
	// DirectXTK には旧 D3DX のような各種演算子オーバーロードを完備した C++ 向けラッパークラスが <SimpleMath.h> にて用意されている。

	typedef DirectX::BoundingSphere BoundingSphereF;
	typedef DirectX::BoundingFrustum BoundingFrustumF;
	typedef DirectX::BoundingBox BoundingBoxF; // AABB
	typedef DirectX::BoundingOrientedBox BoundingOrientedBoxF; // OBB


#pragma endregion


	// RGBA の並び順は Direct3D/OpenGL の基本ネイティブ フォーマットで、最初期からサポートされている。
	// なお、GDI の DIB ビットマップは BGRX/BGRA で、RGBQUAD 構造体の集合になる。Direct2D や WPF のビットマップはこの並び順（厳密には PBGRA）。
	// COLORREF は RGBX になっている。
	// Gdiplus::Color は該当フィールド（メンバー変数）の名前が Argb だが、実際は BGRA 順なので RGBQUAD と同じになっている。
	// Direct3D 9 でよく使われていた D3DCOLOR 型も実は RGBA ではなく BGRA 順に並んでいる。
	class ColorRgba final
	{
	public:
		uint8_t R, G, B, A;
	public:
		ColorRgba()
			: R(), G(), B(), A()
		{}
		ColorRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
			: R(r), G(g), B(b), A(a)
		{}
		ColorRgba(uint8_t r, uint8_t g, uint8_t b)
			: R(r), G(g), B(b), A(0xFF)
		{}
	public:
		bool operator ==(const ColorRgba& other) const
		{
			return
				this->R == other.R &&
				this->G == other.G &&
				this->B == other.B &&
				this->A == other.A;
		}

		bool operator !=(const ColorRgba& other) const
		{
			return !(*this == other);
		}
	};

	class ColorBgra final
	{
	public:
		uint8_t B, G, R, A;
	public:
		ColorBgra()
			: B(), G(), R(), A()
		{}
		ColorBgra(uint8_t b, uint8_t g, uint8_t r, uint8_t a)
			: B(b), G(g), R(r), A(a)
		{}
		ColorBgra(uint8_t b, uint8_t g, uint8_t r)
			: B(b), G(g), R(r), A(0xFF)
		{}
	public:
		bool operator ==(const ColorBgra& other) const
		{
			return
				this->B == other.B &&
				this->G == other.G &&
				this->R == other.R &&
				this->A == other.A;
		}

		bool operator !=(const ColorBgra& other) const
		{
			return !(*this == other);
		}
	};
} // end of namespace
