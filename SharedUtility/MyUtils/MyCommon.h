#pragma once


#include "MyUtil.h"
#include "MyMath.hpp"


namespace MyCommon
{
	// OpenGL/Direct3D で共通のトポロジー種別。
	enum TopologyType
	{
		TopologyType_TriangleList,
		TopologyType_TriangleListAdj,
		TopologyType_TrianglePatchList,
	};


	// フレーム時刻を進めるという意味で Advance を使ったのは、
	// ID3DXAnimationController::AdvanceTime() へのアナロジー。

#if 0
	class MyFrameCounter
	{
	private:
		size_t m_currentFrameIndex = 0;
		size_t m_totalFrameCount = 1;

	public:
		MyFrameCounter()
			//: m_currentFrameIndex()
			//, m_totalFrameCount(1)
		{
		}

		size_t GetCurrentFrameIndex() const { return m_currentFrameIndex; }

		void SetCurrentFrameIndex(size_t index)
		{
			if (index < m_totalFrameCount)
			{
				m_currentFrameIndex = index;
			}
			else
			{
				_ASSERTE(false);
			}
		}

		void ResetFrame()
		{
			m_currentFrameIndex = 0;
		}

		void AdvanceFrame()
		{
			// 最終フレームに到達していない場合、カウンターを進める。
			if (m_currentFrameIndex + 1 < m_totalFrameCount)
			{
				++m_currentFrameIndex;
			}
		}

		void AdvanceLoopFrame()
		{
			m_currentFrameIndex = (m_currentFrameIndex + 1) % m_totalFrameCount;
		}

		void RewindFrame()
		{
			// 先頭フレームに到達していない場合、カウンターを巻き戻す。
			if (m_currentFrameIndex > 0)
			{
				--m_currentFrameIndex;
			}
		}

		void RewindLoopFrame()
		{
			m_currentFrameIndex = (m_currentFrameIndex == 0)
				? this->GetLastFrameIndex()
				: (m_currentFrameIndex - 1);
		}

		bool SetTotalFrameCount(size_t maxFrameCount)
		{
			if (maxFrameCount <= 0)
			{
				_ASSERTE(false);
				return false;
			}
			m_totalFrameCount = maxFrameCount;
			if (m_currentFrameIndex >= m_totalFrameCount)
			{
				// フレーム数を再設定した結果、範囲外になった場合、先頭に戻す。
				// HACK: 末尾に設定するべき？
				this->ResetFrame();
			}
			return true;
		}

		size_t GetTotalFrameCount() const
		{ return m_totalFrameCount; }

		size_t GetLastFrameIndex() const
		{ return m_totalFrameCount - 1; }
	};
#endif

	static const int InvalidAnimIndex = -1;
	static const int InvalidNodeIndex = -1;

	//! @brief  アニメーションのトラック情報（マスターデータ）を管理するクラス。<br>
	//! 同一のスキンメッシュを使う、すべてのゲーム オブジェクト間で共通のデータ。<br>
	class MyAnimTrackInfoTable final : boost::noncopyable
	{
	public:
		typedef std::shared_ptr<MyAnimTrackInfoTable> TSharedPtr;

	private:
		TStrArray m_animTrackNamesArray; //!< アニメーション トラックの名前配列。<br>
		TStrToIntMap m_animNameToIndexMap; //!< アニメーションの名前でインデックスを逆検索できるマップ。シリアライズの対象外。<br>

	public:
		MyAnimTrackInfoTable()
		{}

		void Clear()
		{
			m_animTrackNamesArray.clear();
			m_animNameToIndexMap.clear();
		}

		const TStrArray& GetAnimTrackNamesArray() const { return m_animTrackNamesArray; }

		void SetAnimTrackNamesArray(const TStrArray& animTrackNamesArray)
		{
			m_animTrackNamesArray = animTrackNamesArray;
			m_animNameToIndexMap.clear();
			for (size_t i = 0; i < animTrackNamesArray.size(); ++i)
			{
				m_animNameToIndexMap[animTrackNamesArray[i]] = static_cast<int>(i);
			}
		}

		size_t GetAnimCount() const
		{ return m_animTrackNamesArray.size(); }

		const TStrToIntMap& GetAnimNameToIndexMap() const { return m_animNameToIndexMap; }

		int FindAnimIndex(const std::wstring& animName) const
		{
			auto it = m_animNameToIndexMap.find(animName);
			return (it != m_animNameToIndexMap.end()) ? it->second : InvalidAnimIndex;
		}
	};


	typedef MyMath::TInfluencePair<int, float> AnimInfluence;


	class NodeMovementInfo
	{
	public:
		MyMath::Vector3F Position;
		MyMath::Vector3F Velocity;
		MyMath::Vector3F Accel;
		bool IsFirst = true;
	public:
		NodeMovementInfo()
		{}
	};

	// ばね定数や減衰定数は本来ノード間をつなぐエッジのプロパティだが、簡単のためノードに与える。
	// ノードの接続は方向性を持たない無向グラフで、接続インデックスのリストは本来エッジが持つべき情報だが、簡単のためノードに与える。
	class NodePhysicalPropInfo
	{
	public:
		static const size_t MaxConnectedNodeCount = 8;
	public:
		float Mass = 0;
		float Spring = 0;
		float Damping = 0;
		std::array<int, MaxConnectedNodeCount> ConnectedNodeIndices = { InvalidNodeIndex };
		// TODO: めり込みを防ぐためのコリジョン情報を与える。コリジョン物体（カプセル形状）自体もボーンによって変位するので、コリジョンの名前やインデックス番号などを保持するようにする。
	public:
		NodePhysicalPropInfo()
		{}
	};

	// アニメーションを適用する対象オブジェクトごとのシーク位置を、同時にブレンド可能なアニメーションの数だけ管理する。
	// どのタイミングでブレンドを開始するか、はスクリプトなどで制御できるようになるとよい。
	// バレットタイムのように、シーン時間（ゲーム時間）とは関係なく早送りやスローモーション（アニメーション速度の調整）ができるとよい。
	// アニメーション終了時に呼ばれるコールバック関数を登録できるとよい。
	// HACK: MyFrameCounter クラス自体にコールバック インターフェイス ポインタの配列を持たせる？
	// XNA ACL では BlendFactor 値（重み）と ElapsedTime 値（経過時間）の更新はライブラリ側ではなくクライアント側で行なって
	// コミットする方式になっていた。
	// また、アニメーション1つにつき AnimationController を1つ作成する方式で、
	// BonePose (つまりボーン) ごとに CurrentController と CurrentBlendController の2つを割り当て、
	// 前者をベースに後者をブレンドする方式になっていた模様。
	// AnimationController.AnimationEnded で終了時のコールバック処理を登録することもできる（ただしこれが GC 発生の原因になっていた模様）。
	// ミキサーには、ブレンド対象オブジェクトのインスタンスというシリアライズしにくい情報をアサインするのではなく、
	// アニメーションのインデックス整数値をアサインするようにしたほうがよい。
	// HACK: アニメーションをループするか、それとも最終フレームで止めるか否かは、
	// XNA ACL の IsLooping のようなフラグのプロパティを持たせて管理する？
	// 
	// 最大フレーム数はアニメーションとボーンによって異なるので、
	// 主にブレンド状態を管理するこのクラスに MyFrameCounter を持たせるのは得策ではない。
	// ブレンドソースは2つのみに絞ったほうが、仕様がシンプルになり、球面線形補間もしやすい。
	// また、2つのブレンドであれば重み値は1つだけで済む。
	// スクリプト言語との連携を意識して、整数型には size_t を使わない。できるかぎり int にする。
	// 柔軟性や拡張性の観点からは可変長配列のほうが有利だが、シリアライズや処理速度の観点からは固定長配列のほうが有利。
	class NodeAnimTrackBlendInfo
	{
	public:
		static const size_t MaxAnimTrackBlendSlotCount = 2;
		struct PerAnimInfo
		{
			int AnimTrackIndex;
			int AnimFrameIndex;
		};
	public:
		std::array<PerAnimInfo, MaxAnimTrackBlendSlotCount> AnimTrackInfos = {};
		float BlendFactor = 0; //!< Anim[1] のブレンド比率。0 のときは Anim[0] が支配的。1 のときは Anim[1] が支配的。<br>
	public:
		NodeAnimTrackBlendInfo()
		{}
	};


	class MyAnimMixer final : boost::noncopyable
	{
	public:
		using TSharedPtr = std::shared_ptr<MyAnimMixer>;

	public:
		NodeMovementInfo MovementInfo;
		NodePhysicalPropInfo PhysicalPropInfo;
		NodeAnimTrackBlendInfo AnimTrackBlendInfo;

	public:
		MyAnimMixer()
		{
		}
	};

	typedef std::vector<MyAnimMixer::TSharedPtr> TMyAnimMixerPtrsArray;
	typedef std::shared_ptr<TMyAnimMixerPtrsArray> TMyAnimMixerPtrsArrayPtr;
	typedef std::vector<TMyAnimMixerPtrsArrayPtr> TMyAnimMixerPtrsArrayPtrsArray;

	inline TMyAnimMixerPtrsArrayPtr CreateAnimMixersArray(size_t elemCount)
	{
		auto outArray = std::make_shared<TMyAnimMixerPtrsArray>(elemCount);
#if 1
		for (auto& elem : *outArray)
		{
			elem = std::make_shared<MyAnimMixer>();
		}
#else
		for (size_t i = 0; i < outArray->size(); ++i)
		{
			(*outArray)[i] = std::make_shared<MyAnimMixer>();
		}
#endif
		return outArray;
	}

	inline void SetSingleAnim(TMyAnimMixerPtrsArrayPtrsArray& animMixerArrayArray, int animIndex)
	{
		NodeAnimTrackBlendInfo blendInfo;
		blendInfo.AnimTrackInfos[0].AnimTrackIndex = animIndex;
		blendInfo.AnimTrackInfos[1].AnimTrackIndex = InvalidAnimIndex;
		for (auto& pArray : animMixerArrayArray)
		{
			_ASSERTE(pArray);
			for (auto& mixer : *pArray)
			{
				_ASSERTE(mixer);
				mixer->AnimTrackBlendInfo = blendInfo;
			}
		}
	}

#if 0
	extern size_t GetTotalFrameCountFromBoneAnimInfoArrayArray(const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray, size_t animIndex, size_t boneIndex);
	extern size_t GetMaxTotalFrameCountInAllBonesFromBoneAnimInfoArrayArray(const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray, size_t animIndex);
#endif

	//! @brief  OpenGL / Direct3D などのデバイスに依存しない、ModelMesh ごとのスキニング情報・マテリアル情報を管理するクラス。<br>
	class MyModelMeshDetailInfo final : boost::noncopyable
	{
	public:
		using TSharedPtr = std::shared_ptr<MyModelMeshDetailInfo>;

	private:
		// 複数アニメーションをブレンドしながら遷移するときのために、シーク位置はメッシュごとではなくアニメーションごとに持たせておいたほうがよい。
		// また、対応するフレーム情報を持たないボーン（アニメーションしないボーン）もある。
		// スケルトン情報もアニメーション情報も、メッシュ クラス自体と分離したほうがよい。
		// また、複数のオブジェクトがメッシュを使いまわして描画する際、シーク位置はそのオブジェクトごとに持たせるべきで、
		// マスターとなるアニメーション情報クラスやメッシュ クラスに持たせるべきではない。
		// オブジェクトごとにアニメーション コントローラー（ミキサー）クラスのインスタンスを生成して、そのクラス インスタンスにて管理するようにする。

		// HACK: FBX ではモデルに複数のメッシュが含まれるとき、メッシュごとにスキン情報（ボーン スケルトン、アニメーション行列）が存在するが、
		// もしそれぞれがすべてのメッシュでまったく同じなのであれば、
		// モデル（メッシュの集合）全体で共通のスケルトンおよびアニメーションを生成したほうが効率的。
		// シェーダーモデル 3.0 までは、定数レジスタのサイズに強い制約があったため、
		// ボーン数を増やす場合はパーツを分けてそれぞれのパーツごとに必要なボーンのみ
		// 行列パレットに登録して頂点シェーダーでスキニングする、などの対処を行なう必要があったが、
		// 分割することにより発生するプライベート ボーン インデックスの再計算や親のボーン行列を使った相対ボーン変形の計算は非常に厄介。
		// パーツ分けしないほうがオーバーヘッドも減ってレンダリング効率が良くなる。
		// また、メッシュごとに定数バッファを書き換えるよりも、モデル全体で1回だけ書き換えてそれを使いまわすほうが効率がよい。

		MyMath::TBoneSkeletonInfoPtrsArray m_boneSkeletonInfoArray; //!< ボーン スケルトン情報。要素数＝ボーン（クラスター）数。<br>
		MyMath::TBoneInitialPoseInfoArray m_boneInitialPoseInfoArray; //!< ボーン初期姿勢情報配列。要素数＝ボーン（クラスター）数。<br>
		MyMath::TBoneAnimInfoPtrsArrayPtrsArray m_boneAnimInfoArrayArray; //!< ボーン アニメーション情報配列のジャグ配列。要素数＝アニメーション トラック数。各配列の要素数＝ボーン（クラスター）数。<br>

		TStrToIntMap m_boneNameToIndexMap; //!< ボーンの名前でインデックスを逆検索できるマップ。シリアライズの対象外。<br>

		MyMath::TMyMaterialPtrsArray m_materials; //!< ローカル マテリアルの配列。他のメッシュと実体が共有されるもの（名前が同一）も含まれる。<br>
		TIntArray m_materialIndicesForAttribTable;
		std::wstring m_strMeshName;

		int m_rootBoneIndex = MyMath::BoneSkeletonInfo::InvalidBoneIndex;

	public:
		MyModelMeshDetailInfo()
			//: m_rootBoneIndex(MyMath::BoneSkeletonInfo::InvalidBoneIndex)
		{}

		virtual ~MyModelMeshDetailInfo()
		{}

#if 0
		size_t GetMaxTotalFrameCountInAllBones(size_t animIndex) const
		{ return GetMaxTotalFrameCountInAllBonesFromBoneAnimInfoArrayArray(m_boneAnimInfoArrayArray, animIndex); }
#endif

		// シャローコピーの配列なので扱いには注意。
		void SetMaterialsArray(const MyMath::TMyMaterialPtrsArray& srcMaterials) { m_materials = srcMaterials; }
		MyMath::TMyMaterialPtrsArray& GetMaterialsArray() { return m_materials; }
		const MyMath::TMyMaterialPtrsArray& GetMaterialsArray() const { return m_materials; }

		void SetMaterialIndicesArrayForAttribTable(const TIntArray& indices) { m_materialIndicesForAttribTable = indices; }
		const TIntArray& GetMaterialIndicesArrayForAttribTable() const { return m_materialIndicesForAttribTable; }

		void SetMeshName(const char* pName) { m_strMeshName = MyUtils::SafeConvertUtf8toUtf16(pName); }
		void SetMeshName(const wchar_t* pName) { m_strMeshName = pName ? pName : L""; }
		const std::wstring& GetMeshNameW() const { return m_strMeshName; }

		void SetRootBoneIndex(int index) { m_rootBoneIndex = index; }
		int GetRootBoneIndex() const { return m_rootBoneIndex; }

		// 入力はシャローコピーの配列なので扱いには注意。
		void SetBoneSkeletonInfoArray(const MyMath::TBoneSkeletonInfoPtrsArray& boneSkeletonInfoArray);

		const MyMath::TBoneSkeletonInfoPtrsArray& GetBoneSkeletonInfoArray() const { return m_boneSkeletonInfoArray; }

		void SetBoneInitialPoseInfoArray(const MyMath::TBoneInitialPoseInfoArray& boneInitialPoseInfoArray);

		const MyMath::TBoneInitialPoseInfoArray& GetBoneInitialPoseInfoArray() const { return m_boneInitialPoseInfoArray; }

		// 入力はシャローコピーの配列なので扱いには注意。
		void SetBoneAnimInfoArrayArray(const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray);

		const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& GetBoneAnimInfoArrayArray() const { return m_boneAnimInfoArrayArray; }

#ifdef USE_OLD_BONE_TRAVERSAL
		size_t GetCurrentFrameBoneMatricesArray(MyMath::MatrixF pOutArray[], size_t outArraySize, size_t animIndex, size_t frameIndex) const;
		size_t GetCurrentFrameBoneQuatsArray(MyMath::QuatTransform pOutArray[], size_t outArraySize, size_t animIndex, size_t frameIndex) const;
#endif

		//size_t GetCurrentFrameBoneMatricesArray(MyMath::MatrixF pOutArray[], size_t outArraySize, const TMyAnimMixerPtrsArray& animMixers) const;

		size_t GetCurrentFrameBoneQuatsArray(MyMath::QuatTransform pOutArray[], size_t outArraySize, const TMyAnimMixerPtrsArray& animMixers) const;

		template<size_t OutArraySize> size_t GetCurrentFrameBoneQuatsArray(std::array<MyMath::QuatTransform, OutArraySize>& outArray, const TMyAnimMixerPtrsArray& animMixers) const
		{
			return this->GetCurrentFrameBoneQuatsArray(&outArray[0], outArray.size(), animMixers);
		}

		// TODO: パーツ別モーション ブレンド（ボーンごとに適用アニメーションを変える）を実装する場合はどうする？
		// やはり XNA ACL のように、ボーンごとにアニメーション コントローラーを関連付けるほうが良い？
		// それともボーンの数だけ用意したアニメーション コントローラーの配列を受け取るようにする？
		// 複数のボーンで同じコントローラーを共有する場合に備えて、値配列ではなくスマートポインタ配列を使う？
		// XNA ACL は ModelAnimator の各 BonePose といういわば共通のマスターデータに対して、
		// ゲーム オブジェクトのインスタンスごとの固有データである AnimationController を事前にいったん関連付けておき
		// アニメーション情報を生成する仕組みだったが、このライブラリでは参照の関連付けを行なわず、
		// アニメーション情報が必要なときだけメソッド入力（引数）として使うような完全疎結合の設計方針にしたい。
		// 依存関係がないほうが直交性が高くなる。

		size_t GetBoneCount() const
		{ return m_boneSkeletonInfoArray.size(); }

		const TStrToIntMap& GetBoneNameToIndexMap() const { return m_boneNameToIndexMap; }

		int FindBoneIndex(const std::wstring& boneName) const
		{
			auto it = m_boneNameToIndexMap.find(boneName);
			return (it != m_boneNameToIndexMap.end()) ? it->second : MyMath::BoneSkeletonInfo::InvalidBoneIndex;
		}
	};


	// アニメーションが終了したときのアクションはモデルごとではなく、
	// ゲーム オブジェクトのインスタンスごとに関連付けたメソッドをコールバックするようにしたほうがよいので、
	// マスターテーブルとは分離する。
	// std::function を使うと、C# のデリゲートを使う感覚でイベント ハンドラー用メソッドをバインドできる。
	// 関数ポインタで受ける場合と違い、キャプチャ付きのラムダもバインドできる。
	// std::function オブジェクトをクリアする（関数のバインドを解除する）には、nullptr を代入すればよいらしい。

	typedef std::vector<MyModelMeshDetailInfo::TSharedPtr> TMyModelMeshDetailInfoPtrsArray;
	typedef std::function<void()> TMyAnimEndCallbackFunc;
	typedef std::vector<TMyAnimEndCallbackFunc> TMyAnimEndCallbackFuncsArray;

	extern void AdvanceAllAnimMixerFrameCounters(const MyAnimTrackInfoTable& animTrackInfoTable, const TMyModelMeshDetailInfoPtrsArray& modelMeshInfoArray, TMyAnimMixerPtrsArrayPtrsArray& animMixerArrayArray, const TMyAnimEndCallbackFuncsArray& animEndCallbackFuncsArray);


#if 0
	//! @brief  デバイス依存のデータとデバイス非依存のデータをまとめるクラス テンプレート。<br>
	//! XNA の ModelMesh に近いが、ModelMeshPart は属性テーブルで実現される。<br>
	template<class TDeviceMesh, class TMeshInfo> class TMyDeviceModelMesh final : boost::noncopyable
	{
	public:
		typedef std::shared_ptr<TDeviceMesh> TDeviceMeshPtr;
		typedef std::shared_ptr<TMeshInfo> TMeshInfoPtr;

	private:
		TDeviceMeshPtr m_pDeviceMesh;
		TMeshInfoPtr m_pMeshInfo;

	private:
		TMyDeviceModelMesh()
		{}

	public:
		virtual ~TMyDeviceModelMesh()
		{}

	public:
		explicit TMyDeviceModelMesh(TDeviceMeshPtr pDeviceMesh, TMeshInfoPtr pMeshInfo)
			: m_pDeviceMesh(pDeviceMesh)
			, m_pMeshInfo(pMeshInfo)
		{
		}

	public:
		TDeviceMesh* GetDeviceMesh() { return m_pDeviceMesh.get(); }

		TMeshInfo* GetMeshInfo() { return m_pMeshInfo.get(); }
	};
#endif


	// モーフ ターゲットを表すメッシュ名のプレフィックス。モデルデータを作るときに決めておく必要がある。
	// HACK: あるいはライブラリのプロパティとして実行時に指定できるようにする。

	const LPCSTR MorphTargetMeshPrefixNameA = "MTARGET_";
	const size_t MorphTargetMeshPrefixNameLenA = strlen(MorphTargetMeshPrefixNameA);
	const LPCWSTR MorphTargetMeshPrefixNameW = L"MTARGET_";
	const size_t MorphTargetMeshPrefixNameLenW = wcslen(MorphTargetMeshPrefixNameW);

	inline bool CheckHasMeshMorphTargetPrefixName(LPCSTR pMeshName)
	{
		_ASSERTE(pMeshName);
		return strncmp(pMeshName, MorphTargetMeshPrefixNameA, MorphTargetMeshPrefixNameLenA) == 0;
	}

	inline bool CheckHasMeshMorphTargetPrefixName(LPCWSTR pMeshName)
	{
		_ASSERTE(pMeshName);
		return wcsncmp(pMeshName, MorphTargetMeshPrefixNameW, MorphTargetMeshPrefixNameLenW) == 0;
	}

} // end of namespace
