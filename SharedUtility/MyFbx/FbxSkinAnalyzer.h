#pragma once

#include "FbxAnimTimeInfo.h"
#include "MyUtil.h"
#include "MyFbx.h"


namespace MyFbx
{
#if 0
	class BoneInfluence final
	{
	public:
		int Index; //!< ターゲット頂点に影響を与えるボーンのインデックス。<br>
		float Weight; //!< ターゲット頂点に影響を与えるボーンのウェイト（重み）。<br>
	public:
		BoneInfluence()
			: Index(), Weight()
		{}
	public:
		BoneInfluence(int index, float weight)
			: Index(index), Weight(weight)
		{}
	};
#endif

	typedef MyMath::TInfluencePair<int, float> BoneInfluence;

	typedef std::vector<BoneInfluence> TBoneInfluenceArray;
	typedef std::shared_ptr<TBoneInfluenceArray> TBoneInfluenceArrayPtr;
	typedef std::shared_ptr<const TBoneInfluenceArray> TBoneInfluenceArrayConstPtr;
	typedef std::unordered_map<int, TBoneInfluenceArrayPtr> TVertexIndexToBoneInfluenceArrayPtrMap;
	// C++11 の右辺値参照（ムーブ コンストラクタ）に対応していれば std::unordered_map<int, <TBoneInfluenceArray>> としてもよい。
	// VC++ は右辺値参照に 2010 以降で対応しているが、今回はあえてスマートポインタを使う。

	// FBX ロード時は影響度情報の数を決め打ちせず、可変長で読み込めたほうがよい。
	// Direct3D / OpenGL スキニング シェーダー向けメッシュに変換するときは適宜情報を切り捨てる。
#if 0
	//! @brief  頂点単位のボーン影響度情報パッケージ。<br>
	class BoneInfluenceInfo final
	{
	public:
		enum
		{
			//MAX_INFL_BONE_NUM = 8,
			MAX_INFL_BONE_NUM = 4,
		};
	private:
		int m_targetVertexIndex; //!< ターゲットとなる頂点インデックス。<br>
		int inflNum_; //!< 現在の影響度の総数（ターゲット頂点に影響を与える関連ボーンの数）。<br>
		int boneIndices_[MAX_INFL_BONE_NUM]; //!< ターゲット頂点に影響を与えるボーンのインデックス配列。<br>
		float weights_[MAX_INFL_BONE_NUM]; //!< ターゲット頂点に影響を与えるボーンのウェイト配列。<br>
		bool m_isOverCapacity; //!< 読み込み元のデータがキャパシティ MAX_INFL_BONE_NUM を越えているか否か。<br>

		// 実装が固定長配列のため、メモリブロックをまるごとコピーできる。
		// したがってコピーコンストラクタおよび代入演算子は明示的に定義しないでもよい。
		// コンパイラに自動生成させたほうが安全かつ楽。
		// HACK: FBX 解析時は Index と Weight の構造体配列を std::vector として管理し、可変長ですべて取得して、
		// OpenGL や Direct3D 用のメッシュを作成する際に調整したほうがよい。
	public:
		BoneInfluenceInfo()
			: m_targetVertexIndex()
			, inflNum_()
			, boneIndices_()
			, weights_()
			, m_isOverCapacity()
		{
		}

		void SetVertexIndex(int index) { m_targetVertexIndex = index; }
		int GetVertexIndex() const { return m_targetVertexIndex; }
		int GetAssociatedBoneCount() const { return inflNum_; }
		int GetBoneIndex(int i) const { return boneIndices_[i]; }
		float GetWeight(int i) const { return weights_[i]; }
		bool GetIsOverCapacity() const { return m_isOverCapacity; }

		//! @brief  新しいボーン インデックスとウェイトを末尾に追加する。<br>
		bool Add(int boneIndex, float weight)
		{
			if (inflNum_ >= MAX_INFL_BONE_NUM)
			{
				m_isOverCapacity = true;
				return false;
			}
			boneIndices_[inflNum_] = boneIndex;
			weights_[inflNum_] = weight;
			++inflNum_;

			return true;
		}

#pragma region // std::set あるいは std::map で使用される演算子オーバーロード。//
		// これらの演算子は friend でなくメンバで実装する。ちなみに通常 friend にすべきはプリミティブ型を引数に取る演算子オーバーロード。//
	public:
		bool operator ==(const BoneInfluenceInfo& src) const
		{ return (this->m_targetVertexIndex == src.m_targetVertexIndex); }

		bool operator !=(const BoneInfluenceInfo& src) const
		{ return (this->m_targetVertexIndex != src.m_targetVertexIndex); }

		bool operator <(const BoneInfluenceInfo& src) const
		{ return (this->m_targetVertexIndex < src.m_targetVertexIndex); }
#pragma endregion
	};
#endif


	class MyFbxSkinAnalyzer
	{
	public:
		typedef std::shared_ptr<MyFbxSkinAnalyzer> TSharedPtr;
		typedef std::shared_ptr<const MyFbxSkinAnalyzer> TConstSharedPtr;
	public:
		MyFbxSkinAnalyzer();

		virtual ~MyFbxSkinAnalyzer();

		// 解析
		void Analyze(FbxScene* scene, FbxSkin* skin, const MyFbxAnimTimeInfo& animTimeInfo);

		//! @brief  ボーン影響度情報の数を取得する。<br>
		size_t GetBoneInflInfoNum() const { return m_boneInflMap.size(); }

		//! @brief  ボーン影響度情報を取得する。<br>
		//! @param[in]  vertexIndex  制御点インデックス（頂点インデックス）。<br>
		TBoneInfluenceArrayConstPtr GetBoneInflInfo(int vertexIndex) const;

		const MyMath::TBoneSkeletonInfoPtrsArray& GetBoneSkeletonInfoArray() const { return m_boneSkeletonInfoArray; }
		const MyMath::TBoneInitialPoseInfoArray& GetBoneInitialPoseInfoArray() const { return m_boneInitialPoseInfoArray; }
		const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& GetBoneAnimInfoArrayArray() const { return m_boneAnimInfoArrayArray; }

		void SetSkinName(const char* pName) { m_strSkinName = pName ? MyUtil::ConvertUtf8toUtf16(pName) : L""; }
		void SetSkinName(const wchar_t* pName) { m_strSkinName = pName ? pName : L""; }

		const std::wstring& GetSkinNameW() const { return m_strSkinName; }

		int GetRootBoneIndex() const { return m_rootBoneIndex; }

	private:
		int m_rootBoneIndex;
		std::wstring m_strSkinName;
		TVertexIndexToBoneInfluenceArrayPtrMap m_boneInflMap; //!< ボーン影響度情報マップ。<br>
		MyMath::TBoneSkeletonInfoPtrsArray m_boneSkeletonInfoArray; //!< ボーン スケルトン情報配列。要素数＝ボーン（クラスター）数。<br>
		MyMath::TBoneInitialPoseInfoArray m_boneInitialPoseInfoArray; //!< ボーン初期姿勢情報配列。要素数＝ボーン（クラスター）数。<br>
		MyMath::TBoneAnimInfoPtrsArrayPtrsArray m_boneAnimInfoArrayArray; //!< ボーン アニメーション情報配列のジャグ配列。要素数＝アニメーション トラック数。各配列の要素数＝ボーン（クラスター）数。<br>
	};
} // end of namespace
