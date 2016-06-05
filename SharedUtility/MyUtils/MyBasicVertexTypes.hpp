#pragma once

// めったに更新されない基本頂点型を定義する。

#include "MyMath.hpp"

namespace MyVertexTypes
{
	// NOTE: 定数バッファは 16 バイト アライメントする必要があるが、頂点バッファはバイト単位で配置できる。
	// また、頂点バッファはバイト アドレス バッファにすることもできるが、構造化バッファにすることはできない。

	struct MyVertexT final
	{
		MyMath::Vector2F TexCoord;
	};

	struct MyVertexP final
	{
		MyMath::Vector3F Position;
	};

	struct MyVertexPC final
	{
		MyMath::Vector3F Position;
		MyMath::Vector4F Color;
	};

	struct MyVertexPT final
	{
		MyMath::Vector3F Position;
		MyMath::Vector2F TexCoord;
	};

	struct MyVertexPCT final
	{
		MyMath::Vector3F Position;
		MyMath::Vector4F Color;
		MyMath::Vector2F TexCoord;
	};

#if 0
	struct MyVertexP4CT final
	{
		MyMath::Vector4F Position;
		MyMath::Vector4F Color;
		MyMath::Vector2F TexCoord;
	};
#endif

	struct MyVertexPNT final
	{
		MyMath::Vector3F Position;
		MyMath::Vector4F Normal;
		MyMath::Vector2F TexCoord;
	};


	struct MyVertexPCNT final
	{
		MyMath::Vector3F Position;
		MyMath::Vector4F Color;
		MyMath::Vector3F Normal;
		MyMath::Vector2F TexCoord;
	};

#if 0
	// シェーダー側での1頂点あたりのボーン影響度の最大数は通例4。
	// HACK: Direct3D/OpenGL のハードウェア スキニングは通例、頂点当たりの影響ボーン数は4個までなのだが、どうする？
	// 頂点レイアウトおよびシェーダーを修正すれば、対応数を増やすことは一応可能。
	// もし対応数を超える影響度を無視する場合、ファイル読み込み・変換時に警告を出したほうがよい。
	// なお、無視した結果、影響度の総和が1でなくなるために不都合が生じるが、
	// ローダー側の正規化云々で対処するより、元データを修正するよう警告したほうがよい。
	const int MaxBoneNumPerVertex = 4;
#else
	// シェーダー側で uint4 x2 と float4 x2 を使うことで、1頂点あたりのボーン影響度の最大数は8つまで拡張できる。
	const int MaxBoneNumPerVertex = 8;
#endif

	//! @brief  スキニング用の頂点データ型。<br>
	class MySkinVertex final
	{
	public:
		MyMath::Vector3F Position; //!< 位置ベクトル。<br>
		MyMath::Vector3F Normal; //!< 法線ベクトル。<br>
		MyMath::Vector2F TexCoord; //!< ディフューズ テクスチャ UV。<br>
		uint8_t BoneIndices[MaxBoneNumPerVertex]; //!< 頂点に関連付けられたボーンのインデックス配列。<br>
		float BoneWeights[MaxBoneNumPerVertex]; //!< 頂点に関連付けられたボーンの重み配列。<br>

	public:
		MySkinVertex()
			: Position(MyMath::ZERO_VECTOR3F)
			, Normal(MyMath::ZERO_VECTOR3F)
			, TexCoord(MyMath::ZERO_VECTOR2F)
			, BoneIndices()
			, BoneWeights()
		{
		}
	};

	typedef std::vector<MySkinVertex> TMySkinVertexArray;


	// 法線インデックス・UV インデックスが頂点インデックスと別物の場合、法線や UV も含めて分離する必要があるため、
	// MyOriginalDataPerSkinVertex は使わなくなった。
#if 0
	//! @brief  アプリケーション定義のオリジナル頂点データ型。<br>
	//! 描画に不都合な「共有された頂点」を分離するために一時的に使われる。<br>
	class MyOriginalDataPerSkinVertex final
	{
	public:
		MyMath::Vector3F Position; //!< 位置ベクトル。<br>
		uint8_t BoneIndices[MaxBoneNumPerVertex]; //!< 頂点に関連付けられたボーンのインデックス配列。<br>
#if 0
		uint8_t BoneWeights[MaxBoneNumPerVertex]; //!< 頂点に関連付けられたボーンの重み配列。<br>
#else
		float BoneWeights[MaxBoneNumPerVertex]; //!< 頂点に関連付けられたボーンの重み配列。<br>
#endif

	public:
		MyOriginalDataPerSkinVertex()
			: Position(MyMath::ZERO_VECTOR3F)
			, BoneIndices()
			, BoneWeights()
		{
		}
	};
#endif

	//! @brief  アプリケーション定義の追加頂点データ型。<br>
	//! ファイルから読み込んだメッシュ データに含まれることがある、描画に不都合な「共有された頂点」を分離するために一時的に使われる。<br>
	class MyExtraDataPerSkinVertex final
	{
	public:
		MyMath::Vector3F Normal; //!< 法線ベクトル。<br>
		MyMath::Vector2F TexCoord; //!< ディフューズ テクスチャ UV。<br>

	public:
		MyExtraDataPerSkinVertex()
			: Normal(MyMath::ZERO_VECTOR3F)
			, TexCoord(MyMath::ZERO_VECTOR2F)
		{
		}

		//! @brief  「共有された頂点」を分離するために使われる比較演算子オーバーロード。<br>
		bool operator ==(const MyExtraDataPerSkinVertex& other) const
		{
			return
				MyMath::IsEpsilonEqualVector2(this->TexCoord, other.TexCoord) &&
				MyMath::IsEpsilonEqualVector3(this->Normal, other.Normal);
		}

		bool operator !=(const MyExtraDataPerSkinVertex& other) const
		{
			return !(*this == other);
		}
	};
}
