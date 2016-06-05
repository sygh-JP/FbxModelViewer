#include "stdafx.h"
#include "MyCommon.h"

#include "DebugNew.h"


// TODO: パーツ別アニメーション指定、パーツ別モーション ブレンドを実装する。
// 例えば、ジャグリングしながら歩く、走りながら銃を構える、さらにそれらの遷移など。
// つまり、腰より下のボーンには Walking アニメーションを適用し、腰より上のボーンには Juggling を適用する、など。
// ボーンごとに自走するカウンターを同時ブレンド可能数だけ持たせることになる？
// ブレンドの瞬間を取り出すと、「Bone#15 に対して、Anim#01 の Frame#08 を Weight = 0.4 で、Anim#02 の Frame#23 を Weight = 0.6 でブレンドする」など。
// ブレンドはクォータニオンで行なう。
// 実際にはアニメーションやボーンのインデックスではなく名前を指定してモーションを組み立てる。

// int と size_t を混ぜる場合、符号拡張（Sign Extesion）とゼロ拡張（Zero Extension）に注意。
// http://www.atmarkit.co.jp/ait/articles/1307/29/news006_2.html

namespace MyCommon
{
#if 0
	size_t GetTotalFrameCountFromBoneAnimInfoArrayArray(const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray, size_t animIndex, size_t boneIndex)
	{
		// 対応するフレーム情報を持たないボーン（アニメーションしないボーン）もあることに注意。
		// 同一アニメーションでもフレーム周期の違うボーンが混在していることもある。
		// FBX Converter 付属の LocalMotionBlend.fbx がその例。
		// HACK: ループする場合、全体のアニメーション（最大のフレーム数を持つボーンのアニメーション）が終了したら、すべて最初に巻き戻す、という仕様にする？
		if (animIndex < boneAnimInfoArrayArray.size())
		{
			_ASSERTE(boneAnimInfoArrayArray[animIndex].get() != nullptr);
			const auto& boneAnimInfoArray = *boneAnimInfoArrayArray[animIndex].get();
			if (boneIndex < boneAnimInfoArray.size())
			{
				_ASSERTE(boneAnimInfoArray[boneIndex].get() != nullptr);
				return boneAnimInfoArray[boneIndex]->GetGlobalFrameAttitudeMatrices().size();
			}
		}
		return 0;
	}

	size_t GetMaxTotalFrameCountInAllBonesFromBoneAnimInfoArrayArray(const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray, size_t animIndex)
	{
		if (animIndex < boneAnimInfoArrayArray.size())
		{
			_ASSERTE(boneAnimInfoArrayArray[animIndex].get() != nullptr);
			size_t maxCount = 0;
			// ちなみにこの range-based for の expression 内で STL コンテナへのポインタに対する逆参照のスター * を取り去ると、
			// 通常はコンパイル エラーになるべきだが、
			// Visual C++ 2012 コンパイラーの場合、cl.exe がクラッシュするバグがあるらしい。
			// Update 4 でも直っていない。
			// バグ レポートは MS Connect へ提出済み。VC 2013 Update 2 ではちゃんとコンパイル エラーになる模様。
			for (const auto& pBoneAnimInfo : *boneAnimInfoArrayArray[animIndex])
			{
				maxCount = (std::max)(pBoneAnimInfo->GetGlobalFrameAttitudeMatrices().size(), maxCount);
			}
			return maxCount;
		}
		return 0;
	}
#endif

	// すべてのパーツのフレーム時刻を一様に進める。最大フレーム数（周期）はボーンごとに異なることもある。
	void AdvanceAllAnimMixerFrameCounters(const MyAnimTrackInfoTable& animTrackInfoTable, const TMyModelMeshDetailInfoPtrsArray& modelMeshInfoArray, TMyAnimMixerPtrsArrayPtrsArray& animMixerArrayArray, const TMyAnimEndCallbackFuncsArray& animEndCallbackFuncsArray)
	{
		// HACK: 一度に進めるフレーム数を指定できるようにする。逆方向の巻き戻しも実装する。
		// HACK: 周期が最も長いボーン（もしくはルートボーン）のアニメーションが末尾に到達したら、
		// 各ボーンの自走をやめさせて、すべてのボーンのフレーム時刻をリセットする？
		// であれば、周期をあらかじめ記憶しておき、このメソッドの外部で判断するとよい。

		const size_t arrayCount = modelMeshInfoArray.size();
		_ASSERTE(arrayCount == animMixerArrayArray.size());
		for (size_t a = 0; a < arrayCount; ++a)
		{
			const auto& pMeshInfo = modelMeshInfoArray[a];
			_ASSERTE(pMeshInfo);
			const int rootBoneIndex = pMeshInfo->GetRootBoneIndex();
			const auto& skeletonArray = pMeshInfo->GetBoneSkeletonInfoArray();
			const auto& boneAnimInfoArrayArray = pMeshInfo->GetBoneAnimInfoArrayArray();
			const auto& pAnimMixerArray = animMixerArrayArray[a];
			_ASSERTE(pAnimMixerArray);
			const size_t elemCount = skeletonArray.size();
			_ASSERTE(elemCount == pAnimMixerArray->size());
			for (size_t b = 0; b < elemCount; ++b)
			{
				//const auto& pMeshInfo = skeletonArray[b];
				//_ASSERTE(pMeshInfo);
				const auto& pAnimMixer = (*pAnimMixerArray)[b];
				_ASSERTE(pAnimMixer);
				auto& blendInfo = pAnimMixer->AnimTrackBlendInfo;
				for (auto& animTrackInfo : blendInfo.AnimTrackInfos)
				{
					const size_t animIndex = animTrackInfo.AnimTrackIndex;
					if (animIndex < boneAnimInfoArrayArray.size())
					{
						const auto& pBoneAnimInfoArray = boneAnimInfoArrayArray[animIndex];
						_ASSERTE(pBoneAnimInfoArray);
						const size_t maxFrameCount = (*pBoneAnimInfoArray)[b]->GetGlobalFrameAttitudeMatrices().size();
						if (maxFrameCount > 0)
						{
							// ルートボーンがアニメーション末尾に到達したらコールバック関数を呼び出す。
							const size_t oldCounter = animTrackInfo.AnimFrameIndex;
							const size_t tempCounter = oldCounter + 1; // HACK: とりあえず1フレームのみ増加としている。
							if (b == rootBoneIndex && tempCounter >= maxFrameCount)
							{
								_ASSERTE(animIndex < animEndCallbackFuncsArray.size());
								const auto& callbackFunc = animEndCallbackFuncsArray[animIndex];
								if (callbackFunc) // std::function には operator bool() が実装されている。
								{
									callbackFunc();
								}
							}
							// C/C++ において剰余は符号なしで計算すべき。
							const size_t newCounter = (tempCounter) % maxFrameCount; // 各ボーンは自走ループさせておく。
							// 周期が違うこともある複数のボーンを、それぞれループ アニメーションさせている。
							// HACK: ルートが末尾に到達するまでの他のボーンの挙動（ループ／停止）もあらかじめ決めておけるとよい。
							animTrackInfo.AnimFrameIndex = static_cast<int>(newCounter);
						}
					}
				}
			}
		}
	}


	void MyModelMeshDetailInfo::SetBoneSkeletonInfoArray(const MyMath::TBoneSkeletonInfoPtrsArray& boneSkeletonInfoArray)
	{
		const size_t elemCount = boneSkeletonInfoArray.size();
		m_boneSkeletonInfoArray = boneSkeletonInfoArray;
		// 名前でインデックスを逆検索できるマップを作る。
		m_boneNameToIndexMap.clear();
		for (size_t i = 0; i < elemCount; ++i)
		{
			m_boneNameToIndexMap[boneSkeletonInfoArray[i]->GetBoneNameW()] = static_cast<int>(i);
		}
	}

	void MyModelMeshDetailInfo::SetBoneInitialPoseInfoArray(const MyMath::TBoneInitialPoseInfoArray& boneInitialPoseInfoArray)
	{
		// 先にスケルトン情報配列がセットされていることが前提。
		// サイズ不足などの不正要素は許可しない。
		_ASSERTE(boneInitialPoseInfoArray.size() == m_boneSkeletonInfoArray.size());

		m_boneInitialPoseInfoArray = boneInitialPoseInfoArray;
	}

	void MyModelMeshDetailInfo::SetBoneAnimInfoArrayArray(const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray)
	{
		// 先にスケルトン情報配列がセットされていることが前提。
		// null ポインタやサイズ不足などの不正要素は許可しない。
		_ASSERTE(MyAlgHelpers::CheckAllElemSizesInJaggedArrayAreSame(boneAnimInfoArrayArray, m_boneSkeletonInfoArray.size()));

		m_boneAnimInfoArrayArray = boneAnimInfoArrayArray;
	}

#ifdef USE_OLD_BONE_TRAVERSAL
	// 行列スタックの再帰計算。
	// UNDONE: 複数のアニメーションをブレンドする場合はこのタイミングになる。
	// フレーム数の異なるアニメーション トラックをブレンドするときは？　フレーム インデックスに対して剰余を使う？
	// やはりアニメーション スタックごとにフレーム カウンターを持っていないとダメ。
	// ブレンドするときはフレーム インデックスを複数指定することになる。
	// 複数ブレンドする場合、基準姿勢をどれにするか、という問題もある。
	// UNDONE: アニメーションしないボーンには、フレーム情報が含まれない。その対処。
	static void MultiplyLocalBoneMatrix(
		MyMath::MatrixF pOutArray[],
		size_t outArraySize,
		const MyMath::TBoneSkeletonInfoPtrsArray& boneSkeletonInfoArray,
		const MyMath::TBoneInitialPoseInfoArray& boneInitialPoseInfoArray,
		const MyMath::TBoneAnimInfoPtrsArray& boneAnimInfoArray,
		const TIntArray& childrenIndices, size_t frameIndex)
	{
		// パーツ別ブレンドを実装しないならば、FBX SDK によって構築済みのグローバル行列をそのまま使えばよいが、
		// パーツ別ブレンドする場合はこのタイミングで相対ボーン行列から絶対ボーン行列の構築を行なう。
		// HACK: 相対ボーン行列だと、オイラー角のジンバルロック問題が発生する可能性がある。要調査・検討。
		for (auto childIndex : childrenIndices)
		{
			// NOTE: 行列の乗算順序を間違えないように。
			// 子のグローバル行列＝子のローカル行列×親のグローバル行列。
			// D3DX Math, XNA Math, DirectXMath, HLSL では vAB = v * A * B、
			// GLM, GLSL では vAB = B * A * v という記述になる。
			// DirectX ヘルパー API にはベクトルと行列を直接乗算する演算子オーバーロードはないが、
			// XMVector3TransformCoord(v, m), XMMatrixMultiply(m1, m2) など、
			// 該当ヘルパー関数のパラメータ順序はいずれも OpenGL 表記とは逆順になっている。
			// OpenGL API の表記法は線形代数の一般的な表記法に準ずる。
			// ただし HLSL の matrix（float4x4）のデフォルトの内部表現は、OpenGL と同様で4列目に平行移動成分が入る列優先方式らしい。
			// そのため、C++ 側ヘルパーの行列は一度転置して GPU に転送するようにしてやらないといけない。
			_ASSERTE(size_t(childIndex) < outArraySize);
			const auto& childLocalFrameAttitudes = boneAnimInfoArray[childIndex]->GetLocalFrameAttitudeMatrices();
			const auto& childLocalInitAttitude = boneInitialPoseInfoArray[childIndex].GetLocalInitAttitudeMatrix(); // UNDONE: これで OK かどうか要検証。
			auto& parentGlobalAttitude = pOutArray[boneSkeletonInfoArray[childIndex]->GetParentBoneIndex()];
			MyMath::MultiplyMatrix(&pOutArray[childIndex],
				(frameIndex < childLocalFrameAttitudes.size()) ? &childLocalFrameAttitudes[frameIndex] : &childLocalInitAttitude,
				&parentGlobalAttitude
				);
			MultiplyLocalBoneMatrix(pOutArray, outArraySize,
				boneSkeletonInfoArray,
				boneInitialPoseInfoArray,
				boneAnimInfoArray,
				boneSkeletonInfoArray[childIndex]->GetChildrenBoneIndices(), frameIndex);
		}
	}


	// キーフレーム補間だけでなく、モーション ブレンドする際も、
	// 行列の線形補間よりデュアル クォータニオンの球面線形補間のほうが
	// 品質が高くなるはず。


	// デュアル クォータニオン スタックの再帰計算。
	// UNDONE: 複数のアニメーションをブレンドする場合はこのタイミングになる。
	// フレーム数の異なるアニメーション トラックをブレンドするときは？　フレーム インデックスに対して剰余を使う？
	// やはりアニメーション スタックごとにフレーム カウンターを持っていないとダメ。
	// ブレンドするときはフレーム インデックスを複数指定することになる。
	// 複数ブレンドする場合、基準姿勢をどれにするか、という問題もある。
	// UNDONE: アニメーションしないボーンには、フレーム情報が含まれない。その対処。
	static void MultiplyLocalBoneQuat(
		MyMath::QuatTransform pOutArray[],
		size_t outArraySize,
		const MyMath::TBoneSkeletonInfoPtrsArray& boneSkeletonInfoArray,
		const MyMath::TBoneInitialPoseInfoArray& boneInitialPoseInfoArray,
		const MyMath::TBoneAnimInfoPtrsArray& boneAnimInfoArray,
		const TIntArray& childrenIndices, size_t frameIndex)
	{
		// 行列と QuatTransform の相互変換は結構重いらしい。
		// 再帰＋ループで何度も呼び出されると、ボーン数が多い場合フレームレートが激減する。
		// もし計算に QuatTransform を用いる場合、最初にすべての行列を QuatTransform に変換し、
		// 再帰中は常に QuatTransform で計算して、ボーン数 N に対する変換処理部分の計算量オーダーを O(N) に抑えるべき。
		// 途中計算に QuatTransform でなく行列を用いる場合でも、最後にまとめて QuatTransform に変換するだけで済むようにするべき。
		// トータル計算量的には、（オイラー角から sin/cos を使って回転行列を実行時に計算するようなことがなければ）行列で
		// 途中計算を行なって最後にデュアル クォータニオンに変換するようにしておいたほうがシンプルで高速かもしれない。
		// DirectXMath を使えば行列の積も十分高速なはず。
		// 行列だと球面線形補間ができないという問題はあるが……
		// 定数バッファに余裕があるならば、シェーダー側はクォータニオンでなく行列のほうが効率がよい。
		for (auto childIndex : childrenIndices)
		{
			_ASSERTE(size_t(childIndex) < outArraySize);
			const auto& childLocalFrameAttitudes = boneAnimInfoArray[childIndex]->GetLocalFrameAttitudeQuats();
			const auto& childLocalInitAttitude = boneInitialPoseInfoArray[childIndex].GetLocalInitAttitudeQuat(); // UNDONE: これで OK かどうか要検証。
			auto& parentGlobalAttitude = pOutArray[boneSkeletonInfoArray[childIndex]->GetParentBoneIndex()];
			pOutArray[childIndex] = MyMath::QuatTransform::Multiply(
				(frameIndex < childLocalFrameAttitudes.size()) ? childLocalFrameAttitudes[frameIndex] : childLocalInitAttitude,
				parentGlobalAttitude
				);
			MultiplyLocalBoneQuat(pOutArray, outArraySize,
				boneSkeletonInfoArray,
				boneInitialPoseInfoArray,
				boneAnimInfoArray,
				boneSkeletonInfoArray[childIndex]->GetChildrenBoneIndices(), frameIndex);
		}
	}
#endif

	static void MultiplyLocalBoneQuatSingle(
		MyMath::QuatTransform pOutArray[],
		size_t outArraySize,
		const MyMath::TBoneSkeletonInfoPtrsArray& boneSkeletonInfoArray,
		const MyMath::TBoneInitialPoseInfoArray& boneInitialPoseInfoArray,
		const MyMath::TBoneAnimInfoPtrsArrayPtrsArray& boneAnimInfoArrayArray,
		const TMyAnimMixerPtrsArray& animMixers,
		int targetBoneIndex)
	{
		_ASSERTE(size_t(targetBoneIndex) < outArraySize);
		static const MyMath::QuatTransform zeroQuat;
		MyMath::QuatTransform tempQuat0, tempQuat1;
		auto& moveInfo = animMixers[targetBoneIndex]->MovementInfo;
		const auto& blendInfo = animMixers[targetBoneIndex]->AnimTrackBlendInfo;
		const auto animTrackIndex0 = blendInfo.AnimTrackInfos[0].AnimTrackIndex;
		const auto animTrackIndex1 = blendInfo.AnimTrackInfos[1].AnimTrackIndex;
		const auto animFrameIndex0 = blendInfo.AnimTrackInfos[0].AnimFrameIndex;
		const auto animFrameIndex1 = blendInfo.AnimTrackInfos[1].AnimFrameIndex;
		const float blendFactor = blendInfo.BlendFactor;
		const auto& thisLocalInitAttitude = boneInitialPoseInfoArray[targetBoneIndex].GetLocalInitAttitudeQuat(); // UNDONE: これで OK かどうか要検証。
		const int parentBoneIndex = boneSkeletonInfoArray[targetBoneIndex]->GetParentBoneIndex();
		const auto& parentGlobalAttitude = (parentBoneIndex != MyMath::BoneSkeletonInfo::InvalidBoneIndex) ? pOutArray[parentBoneIndex] : zeroQuat;
		// TODO: 揺れ物指定されているボーンの場合、静的アニメーションやブレンドは実行せず、シミュレーションで位置を演算する。
		const auto& calcAttitude = [&](MyMath::QuatTransform& outQuat, size_t animTrackIndex, size_t animFrameIndex)
		{
			if (animTrackIndex < boneAnimInfoArrayArray.size() && !boneAnimInfoArrayArray[animTrackIndex]->empty())
			{
				const auto& thisLocalFrameAttitudes = (*boneAnimInfoArrayArray[animTrackIndex])[targetBoneIndex]->GetLocalFrameAttitudeQuats();
				outQuat = MyMath::QuatTransform::Multiply(
					(animFrameIndex < thisLocalFrameAttitudes.size())
					? thisLocalFrameAttitudes[animFrameIndex]
					: thisLocalInitAttitude,
					parentGlobalAttitude
					);
			}
		};
		calcAttitude(tempQuat0, animTrackIndex0, animFrameIndex0);
		calcAttitude(tempQuat1, animTrackIndex1, animFrameIndex1);
		const auto blendedQuat = MyMath::QuatTransform::Lerp(tempQuat0, tempQuat1, blendFactor);
		pOutArray[targetBoneIndex] = blendedQuat;
		// 前回の結果がない最初のフレームはどうする？　初回計算か否かを表すフラグを使って分岐する？
		if (moveInfo.IsFirst)
		{
			moveInfo.IsFirst = false;
		}
		else
		{
			// 単位時間あたりの変位を速度とする。
			const auto newVelocity = blendedQuat.TranslationV - moveInfo.Position;
			moveInfo.Accel = (newVelocity) - moveInfo.Velocity;
			moveInfo.Velocity = newVelocity;
		}
		moveInfo.Position = blendedQuat.TranslationV;
		// HACK: 一応並列化は可能？　ただし再帰なので単純にはいかない（最初の兄弟のみ並列化するとかでないと無理）。
		for (auto childBoneIndex : boneSkeletonInfoArray[targetBoneIndex]->GetChildrenBoneIndices())
		{
			// パーツ別モーション ブレンドを行なうので、再帰は必須。
			MultiplyLocalBoneQuatSingle(pOutArray, outArraySize,
				boneSkeletonInfoArray,
				boneInitialPoseInfoArray,
				boneAnimInfoArrayArray,
				animMixers,
				childBoneIndex);
		}
	}

#ifdef USE_OLD_BONE_TRAVERSAL
	static const bool IsPartsMotionBlendEnabled = true;

	size_t MyModelMeshDetailInfo::GetCurrentFrameBoneMatricesArray(MyMath::MatrixF pOutArray[], size_t outArraySize, size_t animIndex, size_t frameIndex) const
	{
		//std::fill_n(pOutArray, outArraySize, MyMath::IDENTITY_MATRIXF);
		// --> 呼び出し先ではクリアしない。呼び出し元でクリアしてあることが前提。

		// 描画時に使用するボーンのパレット行列の配列へのコピー速度性能を優先させるのであれば、
		// モデル データのロード時にボーンごとに行列を配列化するのではなく、フレームごとに行列を配列化しておいたほうがよい。
		// が、その場合必ずグローバル変換行列の配列となり、ローカル変換行列が使えないので、パーツ別モーション ブレンドなどはできなくなる。
		if (IsPartsMotionBlendEnabled)
		{
			// パーツ別モーション ブレンドを行なう場合、再帰計算が必要。
			if (
				!m_boneSkeletonInfoArray.empty() &&
				!m_boneInitialPoseInfoArray.empty() &&
				(animIndex < m_boneAnimInfoArrayArray.size()) &&
				m_boneAnimInfoArrayArray[animIndex] &&
				!m_boneAnimInfoArrayArray[animIndex]->empty()
				)
			{
				const auto pBoneAnimInfoArray = m_boneAnimInfoArrayArray[animIndex];
				//_ASSERTE(pBoneAnimInfoArray.get() != nullptr);
				_ASSERTE(pBoneAnimInfoArray->size() == m_boneSkeletonInfoArray.size());
				_ASSERTE(m_boneInitialPoseInfoArray.size() == m_boneSkeletonInfoArray.size());
				size_t activeElemCount = MyUtil::MinVal(m_boneSkeletonInfoArray.size(), outArraySize);
				// 0番がルート ボーンであることが前提のコードは NG。
				// FBX Converter 付属の LocalMotionBlend.fbx などは該当しない。
				// HACK: ルートが複数存在する場合は考慮しなくてよい？　そもそもそれはルートと呼ばない？
				const int rootBoneIndex = m_rootBoneIndex;
				const auto& rootLocalFrameAttitudes = pBoneAnimInfoArray->at(rootBoneIndex)->GetLocalFrameAttitudeMatrices();
				const auto& rootLocalInitAttitude = m_boneInitialPoseInfoArray[rootBoneIndex].GetLocalInitAttitudeMatrix();
				pOutArray[rootBoneIndex] = (frameIndex < rootLocalFrameAttitudes.size()) ? rootLocalFrameAttitudes[frameIndex] : rootLocalInitAttitude;
				MultiplyLocalBoneMatrix(pOutArray, outArraySize,
					m_boneSkeletonInfoArray,
					m_boneInitialPoseInfoArray,
					*pBoneAnimInfoArray,
					m_boneSkeletonInfoArray[rootBoneIndex]->GetChildrenBoneIndices(), frameIndex);
				return activeElemCount;
			}
		}
		else
		{
			// パーツ別モーション ブレンドを行なわない場合、再帰計算は不要。
			// ただし、FBX Converter 付属の LocalMotionBlend.fbx などは、
			// アニメーションなし Null ノード（XXX_End クラスター）を持つため、再帰計算方式が必須？
			if (
				!m_boneSkeletonInfoArray.empty() &&
				!m_boneInitialPoseInfoArray.empty() &&
				(animIndex < m_boneAnimInfoArrayArray.size()) &&
				m_boneAnimInfoArrayArray[animIndex] &&
				!m_boneAnimInfoArrayArray[animIndex]->empty()
				)
			{
				const auto pBoneAnimInfoArray = m_boneAnimInfoArrayArray[animIndex];
				//_ASSERTE(pBoneAnimInfoArray.get() != nullptr);
				_ASSERTE(pBoneAnimInfoArray->size() == m_boneSkeletonInfoArray.size());
				_ASSERTE(m_boneInitialPoseInfoArray.size() == m_boneSkeletonInfoArray.size());
				size_t activeElemCount = MyUtil::MinVal(m_boneSkeletonInfoArray.size(), outArraySize);
				for (size_t i = 0; i < activeElemCount; ++i)
				{
					pOutArray[i] = (pBoneAnimInfoArray->at(i)->GetGlobalFrameAttitudeMatrices().size() > frameIndex)
						? pBoneAnimInfoArray->at(i)->GetGlobalFrameAttitudeMatrices()[frameIndex]
					: m_boneInitialPoseInfoArray[i].GetGlobalInitAttitudeMatrix();
				}
				return activeElemCount;
			}
		}
		return 0;
	}

	size_t MyModelMeshDetailInfo::GetCurrentFrameBoneQuatsArray(MyMath::QuatTransform pOutArray[], size_t outArraySize, size_t animIndex, size_t frameIndex) const
	{
		if (IsPartsMotionBlendEnabled)
		{
			if (
				!m_boneSkeletonInfoArray.empty() &&
				!m_boneInitialPoseInfoArray.empty() &&
				(animIndex < m_boneAnimInfoArrayArray.size()) &&
				m_boneAnimInfoArrayArray[animIndex] &&
				!m_boneAnimInfoArrayArray[animIndex]->empty()
				)
			{
				const auto pBoneAnimInfoArray = m_boneAnimInfoArrayArray[animIndex];
				//_ASSERTE(pBoneAnimInfoArray.get() != nullptr);
				_ASSERTE(pBoneAnimInfoArray->size() == m_boneSkeletonInfoArray.size());
				_ASSERTE(m_boneInitialPoseInfoArray.size() == m_boneSkeletonInfoArray.size());
				size_t activeElemCount = MyUtil::MinVal(m_boneSkeletonInfoArray.size(), outArraySize);
				const int rootBoneIndex = m_rootBoneIndex;
				const auto& rootLocalFrameAttitudes = pBoneAnimInfoArray->at(rootBoneIndex)->GetLocalFrameAttitudeQuats();
				const auto& rootLocalInitAttitude = m_boneInitialPoseInfoArray[rootBoneIndex].GetLocalInitAttitudeQuat();
				pOutArray[rootBoneIndex] = (frameIndex < rootLocalFrameAttitudes.size()) ? rootLocalFrameAttitudes[frameIndex] : rootLocalInitAttitude;
				MultiplyLocalBoneQuat(pOutArray, outArraySize,
					m_boneSkeletonInfoArray,
					m_boneInitialPoseInfoArray,
					*pBoneAnimInfoArray,
					m_boneSkeletonInfoArray[rootBoneIndex]->GetChildrenBoneIndices(), frameIndex);
				return activeElemCount;
			}
		}
		else
		{
			if (
				!m_boneSkeletonInfoArray.empty() &&
				!m_boneInitialPoseInfoArray.empty() &&
				(animIndex < m_boneAnimInfoArrayArray.size()) &&
				m_boneAnimInfoArrayArray[animIndex] &&
				!m_boneAnimInfoArrayArray[animIndex]->empty()
				)
			{
				const auto pBoneAnimInfoArray = m_boneAnimInfoArrayArray[animIndex];
				//_ASSERTE(pBoneAnimInfoArray.get() != nullptr);
				_ASSERTE(pBoneAnimInfoArray->size() == m_boneSkeletonInfoArray.size());
				_ASSERTE(m_boneInitialPoseInfoArray.size() == m_boneSkeletonInfoArray.size());
				size_t activeElemCount = MyUtil::MinVal(m_boneSkeletonInfoArray.size(), outArraySize);
				for (size_t i = 0; i < activeElemCount; ++i)
				{
					pOutArray[i] = (pBoneAnimInfoArray->at(i)->GetGlobalFrameAttitudeQuats().size() > frameIndex)
						? pBoneAnimInfoArray->at(i)->GetGlobalFrameAttitudeQuats()[frameIndex]
					: m_boneInitialPoseInfoArray[i].GetGlobalInitAttitudeQuat();
				}
				return activeElemCount;
			}
		}
		return 0;
	}
#endif

	size_t MyModelMeshDetailInfo::GetCurrentFrameBoneQuatsArray(MyMath::QuatTransform pOutArray[], size_t outArraySize, const TMyAnimMixerPtrsArray& animMixers) const
	{
		if (
			!m_boneSkeletonInfoArray.empty() &&
			!m_boneInitialPoseInfoArray.empty()
			)
		{
			_ASSERTE(m_boneInitialPoseInfoArray.size() == m_boneSkeletonInfoArray.size());
			const size_t activeElemCount = (std::min)(m_boneSkeletonInfoArray.size(), outArraySize);

			MultiplyLocalBoneQuatSingle(pOutArray, outArraySize,
				m_boneSkeletonInfoArray,
				m_boneInitialPoseInfoArray,
				m_boneAnimInfoArrayArray,
				animMixers,
				m_rootBoneIndex);

			return activeElemCount;
		}

		return 0;
	}

} // end of namespace
