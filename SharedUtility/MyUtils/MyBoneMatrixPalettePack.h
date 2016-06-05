#pragma once

#include "MyMath.hpp"

// HLSL であればマクロ定数をシェーダープログラムと直接 C/C++ ヘッダー経由で共有することは可能。
// なお、定数をシェーダープログラムのコンパイル時（アプリ実行時）に動的に共有することはできなくはないが、
// HLSL の実行時コンパイルには D3DCompiler ランタイムが必要になる。

namespace MyCpuGpuCommon
{
	// GPU の定数バッファとして直接送信するデータ構造は、float4 単位（16バイト単位）でアライメントする必要があるので注意。


	class MyBoneMatrixPalettePack final
	{
	public:
		//static const int MAX_ANIM_BONE_NUM = 56;
		static const int MAX_ANIM_BONE_NUM = 120;
	public:
		MyBoneMatrixPalettePack()
		{}
	public:
		std::array<MyMath::MatrixF, MAX_ANIM_BONE_NUM> BoneMatrices;
	public:
		void IdentifyAllMatrices()
		{
			std::fill_n(this->BoneMatrices.begin(), this->BoneMatrices.size(), MyMath::IDENTITY_MATRIXF);
		}
		void TransposeAllMatrices()
		{
			std::for_each(this->BoneMatrices.begin(), this->BoneMatrices.end(),
				[](MyMath::MatrixF& m) { XMStoreFloat4x4(&m, XMMatrixTranspose(XMLoadFloat4x4(&m))); });
		}
	};


	class MyBoneQuatPalettePack final
	{
	public:
		//static const int MAX_ANIM_BONE_NUM = 56;
		static const int MAX_ANIM_BONE_NUM = 120;
	public:
		MyBoneQuatPalettePack()
		{}
	public:
		std::array<MyMath::QuatTransform, MAX_ANIM_BONE_NUM> BoneQuats;
	public:
		void IdentifyAllQuats()
		{
			std::fill_n(this->BoneQuats.begin(), this->BoneQuats.size(), MyMath::QuatTransform());
		}
		void ConvertToMatrices(MyMath::MatrixF outMatrices[], size_t outArraySize) const
		{
			_ASSERTE(outArraySize >= this->BoneQuats.size());
			for (size_t i = 0; i < this->BoneQuats.size(); ++i)
			{
				this->BoneQuats[i].ToMatrix(outMatrices[i]);
			}
		}
		template<size_t OutArraySize> void ConvertToMatrices(std::array<MyMath::MatrixF, OutArraySize>& outMatrices) const
		{
			this->ConvertToMatrices(&outMatrices[0], outMatrices.size());
		}
	};

	static_assert(sizeof(MyBoneMatrixPalettePack) % 16 == 0, "Not aligned!!");
	static_assert(sizeof(MyBoneQuatPalettePack) % 16 == 0, "Not aligned!!");
}
