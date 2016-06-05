#include "stdafx.h"

#include "FbxSkinAnalyzer.h"
#include "CustomTrace.hpp"


namespace
{
	void DumpMatrix(const wchar_t* pName, const MyMath::MatrixF& mat)
	{
		ATLTRACE(L"%s = (%f, %f, %f)(%f, %f, %f)(%f, %f, %f)(%f, %f, %f)\n",
			pName,
			mat._11,
			mat._12,
			mat._13,
			mat._21,
			mat._22,
			mat._23,
			mat._31,
			mat._32,
			mat._33,
			mat._41,
			mat._42,
			mat._43);
	}

	void DumpMatrixTranslationComponents(const wchar_t* pName, const MyMath::MatrixF& mat)
	{
		ATLTRACE(L"%s.T = (%f, %f, %f)\n",
			pName,
			mat._41,
			mat._42,
			mat._43);
	}

	void DumpMatrixTranslationComponents(const wchar_t* pName, const FbxMatrix& mat)
	{
		ATLTRACE(L"%s.T = (%f, %f, %f)\n",
			pName,
			mat[3][0],
			mat[3][1],
			mat[3][2]);
	}
}

namespace MyFbx
{
#if 0
	static FbxAMatrix GetNodeMatrix(const FbxNode* pNode)
	{
		FbxAMatrix matGeometry;
		//const auto pivotSet = FbxNode::eSourcePivot;
		const auto pivotSet = FbxNode::eDestinationPivot;
		const auto vTranslation = pNode->GetGeometricTranslation(pivotSet);
		const auto vRotation = pNode->GetGeometricRotation(pivotSet);
		const auto vScaling = pNode->GetGeometricScaling(pivotSet);
		matGeometry.SetS(vScaling);
		matGeometry.SetR(vRotation);
		matGeometry.SetT(vTranslation);

		return matGeometry;
	}

	static FbxAMatrix ComputeTotalMatrix(const FbxNode* pNode, const FbxAMatrix& matGlobalTransform)
	{
		//return matGlobalTransform * GetNodeMatrix(pNode);
		return GetNodeMatrix(pNode) * matGlobalTransform;
	}
#endif


	MyFbxSkinAnalyzer::MyFbxSkinAnalyzer()
		: m_rootBoneIndex(MyMath::BoneSkeletonInfo::InvalidBoneIndex)
	{
	}

	MyFbxSkinAnalyzer::~MyFbxSkinAnalyzer()
	{
	}

	void MyFbxSkinAnalyzer::Analyze(FbxScene* scene, FbxSkin* skin, const MyFbxAnimTimeInfo& animTimeInfo)
	{
		_ASSERTE(scene != nullptr);
		const auto animPeriod = animTimeInfo.GetPeriod();

		//auto* pAnimEvaluator = scene->GetEvaluator(); // FBX SDK 2014.2 で deprecated になったらしい。

		//auto* pAnimEvaluator = scene->GetFbxManager()->GetAnimationEvaluator(); // FBX SDK 2015.1 で deprecated になったらしい。
		//auto* pDefaultAnimStack = pAnimEvaluator->GetContext(); // FBX SDK 2015.1 で deprecated になったらしい。

		//auto* pAnimEvaluator = scene->GetAnimationEvaluator(); // FbxScene に直接取得・設定メソッドが追加されているのでここでは不要。
		auto* pDefaultAnimStack = scene->GetCurrentAnimationStack();

		// FBX SDK をアップデートするたびに、毎回何らかの deprecated が発生するが、
		// 性能改善や対応プラットフォーム追加はともかく API 設計くらいはいいかげん Fix して欲しい……

		const int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
		ATLTRACE("AnimStackCount = %d\n", animStackCount);

		for (int a = 0; a < animStackCount; ++a)
		{
			m_boneAnimInfoArrayArray.push_back(std::make_shared<MyMath::TBoneAnimInfoPtrsArray>());
		}
		//m_boneAnimInfoArrayArray.resize(animStackCount, std::make_shared<MyMath::TBoneAnimInfoPtrsArray>());
		// --> NG。すべての要素をまったく同じインスタンスで初期化することになるので、スマートポインタが共有されてしまう。

		TStrArray boneNamesArray;
		TStrArray parentNamesArray;

		this->SetSkinName(skin->GetName());
		const int clusterCount = skin->GetClusterCount();
#pragma region // ボーン配列および影響度情報の取得。
#if 0
		// SetLinkMode() は、実データには影響を与えないらしい。
		// http://codelogy.org/archives/2008/09/fbx_1.html
		for (int i = 0; i < clusterCount; ++i)
		{
			auto* pCluster = skin->GetCluster(i);
			pCluster->SetLinkMode(FbxCluster::eTotalOne);
		}
#endif
		for (int i = 0; i < clusterCount; ++i)
		{
			// クラスターはボーンを平面的（というより1次元）に管理するもので、FBX においてボーン階層構造を管理するのはスケルトン。
			// クラスター リンク ノードがスケルトン ノードへの参照になっているらしい。
			// クラスターの数とスケルトン ノードの数が一致するとはかぎらない。
			const auto* pCluster = skin->GetCluster(i);

			const auto originalLinkMode = pCluster->GetLinkMode();

			// クラスターが影響を与える頂点情報を取得。
			const int pointNum = pCluster->GetControlPointIndicesCount();
			const double* weightAry = pCluster->GetControlPointWeights();
			const int* pointAry = pCluster->GetControlPointIndices();

			// ボーン影響情報マップから頂点インデックスをもとに検索して、
			// ボーン影響度情報を追加。
			for (int j = 0; j < pointNum; ++j)
			{
				const float weight = static_cast<float>(weightAry[j]);
				const int vertexIndex = pointAry[j];
				// インデクサを使うと、キーが存在しなければ新規作成されるし、存在すればそのキーに対応する値への参照が返る。
				auto& pBoneInflArray = m_boneInflMap[vertexIndex];
				if (!pBoneInflArray)
				{
					// 配列のインスタンスが無効なので新規作成。
					pBoneInflArray = std::make_shared<TBoneInfluenceArray>();
				}
				pBoneInflArray->push_back(BoneInfluence(i, weight));

				// ちなみに std::set の要素は値とキーを兼ねているが、
				// C++11 規格においては、C++03 以前と違って set の要素を直接変更できない（find() などは必ず const_iterator を返す）ので、
				// 要素の内容を書き換える場合はいったん erase() する必要がある。
				// なお、find(), erase() して insert() するのは冗長（実質2回のツリーorマップ検索が入る）。
				// map/multimap/unordered_map の代わりに set/multiset/unordered_set を使う場合は本当に必要かどうかよく吟味する必要がある。
				// さらに、場合によっては vector と std::find() による線形検索のほうが高速ということも往々にしてよくある（vector は省メモリでキャッシュ効率もよい）。
			}

			const auto* pClusterLinkNode = pCluster->GetLink();
			const auto* pClusterLinkParentNode = pClusterLinkNode->GetParent();
			ATLTRACE("Cluster[%d]: ClusterLinkNode = 0x%p, ClusterLinkParentNode = 0x%p, Cluster = 0x%p, SkinNode = 0x%p, OriginalLinkMode = %d\n",
				i, pClusterLinkNode, pClusterLinkParentNode, pCluster, skin, originalLinkMode);

			const auto attributeType = pClusterLinkNode->GetNodeAttribute()->GetAttributeType();
			// クラスター リンク ノードの種別は eSkeleton のはず。
			if (attributeType != FbxNodeAttribute::eSkeleton)
			{
				ATLTRACE(MY_LOC_FOR_WARNING_TO_STRINGA "Clustrer link node [%d] is not a skeleton.\n", i);
			}

			const auto strBoneName = MyUtil::SafeConvertUtf8toUtf16(pClusterLinkNode->GetName());
			const auto strParentName = MyUtil::SafeConvertUtf8toUtf16(pClusterLinkParentNode->GetName());
			ATLTRACE(L"BoneName[%d] = \"%s\"\n", i, strBoneName.c_str());
			ATLTRACE(L"ParentName[%d] = \"%s\"\n", i, strParentName.c_str());
			auto boneSkeletonInfo = std::make_shared<MyMath::BoneSkeletonInfo>();
			boneSkeletonInfo->SetBoneName(strBoneName.c_str());
			// ボーン階層構造はまだここでは完成しない。FBX スケルトン情報を使った後処理が必要。
			// 一応親の名前は分かるので、クラスター情報のみで階層構造を構築することもできなくはない。
			// なお、スケルトン階層には存在するがクラスター配列には存在しないボーンを含む FBX もある。
			// FBX Converter 付属の LocalMotionBlend.fbx がその例。
			m_boneSkeletonInfoArray.push_back(boneSkeletonInfo);
			boneNamesArray.push_back(strBoneName);
			parentNamesArray.push_back(strParentName);
		}
#pragma endregion

		// NOTE: ルート ボーンを探索するため、ループを分ける必要がある。
		m_rootBoneIndex = MyMath::BoneSkeletonInfo::InvalidBoneIndex;
		for (size_t p = 0; p < parentNamesArray.size(); ++p)
		{
			if (std::find(boneNamesArray.begin(), boneNamesArray.end(), parentNamesArray[p]) == boneNamesArray.end())
			{
				// p 番目の親がクラスター配列に存在しない場合、p 番目のクラスターがルート ボーン。
				// NOTE: ルート ボーンは複数存在しないことが前提。
				m_rootBoneIndex = static_cast<int>(p);
				ATLTRACE(L"RootBoneIndex = %d, RootBoneName = \"%s\", RootBoneParentName = \"%s\"\n",
					m_rootBoneIndex, boneNamesArray[p].c_str(), parentNamesArray[p].c_str());
				break;
			}
		}
		_ASSERTE(m_rootBoneIndex != MyMath::BoneSkeletonInfo::InvalidBoneIndex);

#if 0
		auto* pRootCluster = skin->GetCluster(m_rootBoneIndex);
		auto* pRootBoneLinkNode = pRootCluster->GetLink();
		auto* pRootBoneLinkParentNode = pRootBoneLinkNode->GetParent();
		_ASSERTE(pRootCluster);
		FbxAMatrix fbxRootLinkInitBoneBindPoseMatrix;
		pRootCluster->GetTransformLinkMatrix(fbxRootLinkInitBoneBindPoseMatrix);
		const FbxAMatrix fbxRootLinkInitBoneBindPoseMatrixInv = fbxRootLinkInitBoneBindPoseMatrix.Inverse();
#endif

#pragma region // アニメーション スタックの取得と適用。
		m_boneInitialPoseInfoArray.resize(clusterCount);
		for (int i = 0; i < clusterCount; ++i)
		{
			// FbxNode::GetAnimationInterval() がなぜか const メソッドでないので、const ポインタにできない。
			auto* pCluster = skin->GetCluster(i);
			auto* pClusterLinkNode = pCluster->GetLink();

			ATLTRACE(L"Cluster[%d] = \"%s\"\n", i, boneNamesArray[i].c_str());

			FbxAMatrix fbxClusterTransformMatrix;
			FbxAMatrix fbxClusterTransformLinkMatrix;
			FbxAMatrix fbxClusterTransformAssociateModelMatrix;
			FbxAMatrix fbxClusterTransformParentMatrix;
			// TransformLinkMatrix は、絶対座標系での変換行列？　グローバル初期姿勢行列 Gini に相当するらしい。
			// FBX Converter 付属の LocalMotionBlend.fbx はどうも TransformMatrix のほうを使うみたいだが……
			// 両者の積を使う？
			pCluster->GetTransformMatrix(fbxClusterTransformMatrix);
			pCluster->GetTransformLinkMatrix(fbxClusterTransformLinkMatrix);
			pCluster->GetTransformAssociateModelMatrix(fbxClusterTransformAssociateModelMatrix);
			pCluster->GetTransformParentMatrix(fbxClusterTransformParentMatrix);
			const bool isTransformParentSet = pCluster->IsTransformParentSet();
			const FbxAMatrix fbxClusterTransformLinkMatrixInv = fbxClusterTransformLinkMatrix.Inverse();
			//MyMath::MatrixF tempClusterTransformMatrix;
			MyMath::MatrixF tempClusterTransformLinkMatrix;
			//MyFbx::ToMatrixF(tempClusterTransformMatrix, fbxClusterTransformMatrix);
			MyFbx::ToMatrixF(tempClusterTransformLinkMatrix, fbxClusterTransformLinkMatrix);

			// グローバル初期姿勢行列 Gini の設定。
			// アニメーションごとに用意する必要はない。クラスターごとに1つなので、全体ではスキンに対して1配列になる。

			//m_boneInitialPoseInfoArray[i].SetGlobalInitAttitudeMatrix(tempClusterTransformMatrix); // NG.
			m_boneInitialPoseInfoArray[i].SetGlobalInitAttitudeMatrix(tempClusterTransformLinkMatrix);

#if 1
			// 検証用に平行移動成分をダンプしてみる。
			DumpMatrixTranslationComponents(L"Cluster.TransformMatrix", fbxClusterTransformMatrix);
			DumpMatrixTranslationComponents(L"Cluster.TransformLinkMatrix", fbxClusterTransformLinkMatrix);
			DumpMatrixTranslationComponents(L"Cluster.TransformAssociateModelMatrix", fbxClusterTransformAssociateModelMatrix);
			DumpMatrixTranslationComponents(L"Cluster.TransformParentMatrix", fbxClusterTransformParentMatrix);
			ATLTRACE(L"IsTransformParentSet = %d\n", isTransformParentSet);
#endif
#if 0
			// これらは親ノードからの相対値らしい。
			// EvaluateLocalTransform() で得られる行列の成分に等しくなる？
			// ちなみにローカル座標系を設定した MQO ファイルを FBX Exporter プラグインでエクスポートした場合、
			// ローカル座標情報はメッシュ ノードの LclTranslation などとして記録されている？
			// HACK: ローカル座標を基準にタイヤを回転させるなどの処理をプログラム・スクリプトで組みたい場合、
			// ローカル座標系がサポートされていたほうがよい。
			const auto vTranslation = ToVector3F(pClusterLinkNode->LclTranslation.Get());
			const auto vRotation = ToVector3F(pClusterLinkNode->LclRotation.Get());
#endif

			// TODO: 複数のアニメーション トラック（テイク）の姿勢行列スタックを取り出すとき、
			// FbxAnimEvaluator::SetContext() を使ってアニメーション スタックを切り替えればよいらしい。
			// トラックごとのインターバルは FbxNode::GetAnimationInterval() を使って取得できるらしい。
			for (int a = 0; a < animStackCount; ++a)
			{
				int startFrame = 0, stopFrame = 0;
				auto* pAnimStack = scene->GetSrcObject<FbxAnimStack>(a);
				//pAnimEvaluator->SetContext(pAnimStack); // FBX SDK 2015.1 で deprecated になったらしい。
				scene->SetCurrentAnimationStack(pAnimStack);
				FbxTimeSpan animInterval;
				const bool isNodeAnimated = pClusterLinkNode->GetAnimationInterval(animInterval, pAnimStack); // なぜか const メソッドでない。
				if (isNodeAnimated)
				{
					startFrame = static_cast<int>(animInterval.GetStart().Get() / animPeriod.Get());
					// HACK: FBX Converter 付属の LocalMotionBlend.fbx はココで引っかかる。
					// このとき、animPeriod = 1539538600, startFrame = +1696030290, stopFrame = -1696030290 になっている。
					// 実際には、Start 値が +9223372036854775807 つまり +LONGLONG_MAX（FBXSDK_TIME_INFINITE、旧 KTIME_INFINITE）になっている。
					// また、Stop 値が -9223372036854775807 つまり -LONGLONG_MAX（FBXSDK_TIME_MINUS_INFINITE、旧 KTIME_MINUS_INFINITE）になっている。
					// つまり正しい値が設定されていない？
					// アニメーション付き FBX といえど、アニメーションなしのボーンノードもあるらしい。
					// XXX_End と名付けられた、いわゆる Null ノードに相当するボーンにはアニメーションがない。
					// FbxNode::GetAnimationInterval() の戻り値もチェックする必要があるらしい。

					//_ASSERTE(startFrame == 0); // それ以外は対応しない。
					// UNDONE: --> 開始時間のオフセットがある場合はどうする？
					// FBX SDK を使う場合は開始時刻前／終了時刻後などの範囲外フレームを指定しても姿勢を取得できる模様（たぶん開始時刻／終了時刻の姿勢になる？）。
					// ちなみに (Stop - Start) != TotalFrameCount であることに注意。
					stopFrame = static_cast<int>(animInterval.GetStop().Get() / animPeriod.Get());
				}
				ATLTRACE(L"AmimStack[%d]: StartFrame = %d, StopFrame = %d\n", a, startFrame, stopFrame);

				// 初期姿勢行列およびフレームごとの姿勢行列を取得して、ボーン アニメーション情報としてまとめる。
				// HACK: キーフレームのみ抽出して、補間は実行時にできたほうが柔軟で省メモリ。
				// ただし、補間用の関数（接線）によって結果（アニメーション カーブ）が変動する。
				// SDK で補間する場合、EvaluateGlobalTransform(), EvaluateLocalTransform() での補間方法は
				// FbxNode::SetQuaternionInterpolation() で指定できる？　球面線形補間とかできるのか？
				// カーブ情報の取得は KFCurve, KFCurveNode, FbxAnimCurve, FbxAnimCurveNode あたりが必要？
				// http://sa-zero.blog.eonet.jp/switchcase/2014/01/fbx-9f0a.html
				// http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html?url=files/GUID-386BAD6A-82F9-4148-BD5F-3FE2E90A87D0.htm,topicNumber=d30e9698
				auto boneAnimInfo = std::make_shared<MyMath::BoneAnimInfo>();
				// ボーン ローカル行列はまだここでは完成しない。FBX スケルトン情報を使った後処理が必要。
				m_boneAnimInfoArrayArray[a]->push_back(boneAnimInfo);

				// カーブ情報の取得テスト。
#if 1
				const int animLayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
				ATLTRACE("AmimStack[%d]: AnimLayerCount = %d\n", a, animLayerCount);
				// std::find() を書くのが面倒なので、あえて vector でなく set を使う。
				std::set<int> keyFramedTimesSet;
				if (animLayerCount > 0 && isNodeAnimated)
				{
					auto pAnimLayer0 = pAnimStack->GetMember<FbxAnimLayer>(0);
					// FBX Converter 付属の LocalMotionBlend.fbx はすべてのフレーム（開始～終了間）にカーブのキーが存在する模様。
					// つまりキーフレーム アニメーションじゃなく、全部 Bake してあるってことらしい。
					const auto* pCurveTranslateX = pClusterLinkNode->LclTranslation.GetCurve(pAnimLayer0, FBXSDK_CURVENODE_COMPONENT_X);
					const auto* pCurveTranslateY = pClusterLinkNode->LclTranslation.GetCurve(pAnimLayer0, FBXSDK_CURVENODE_COMPONENT_Y);
					const auto* pCurveTranslateZ = pClusterLinkNode->LclTranslation.GetCurve(pAnimLayer0, FBXSDK_CURVENODE_COMPONENT_Z);
					const auto* pCurveRotateX = pClusterLinkNode->LclRotation.GetCurve(pAnimLayer0, FBXSDK_CURVENODE_COMPONENT_X); // Pitch
					const auto* pCurveRotateY = pClusterLinkNode->LclRotation.GetCurve(pAnimLayer0, FBXSDK_CURVENODE_COMPONENT_Y); // Head
					const auto* pCurveRotateZ = pClusterLinkNode->LclRotation.GetCurve(pAnimLayer0, FBXSDK_CURVENODE_COMPONENT_Z); // Bank
					// スケーリングのアニメーションは無視。
					auto findKeyFramedTimes = [](std::set<int>& outSet, const FbxAnimCurve* pCurve, FbxLongLong period)
					{
						if (pCurve)
						{
							for (int k = 0; k < pCurve->KeyGetCount(); ++k)
							{
								outSet.insert(static_cast<int>(pCurve->KeyGetTime(k).Get() / period));
							}
						}
					};
					// FbxAnimCurve からキーフレームの時刻とその時刻における値（平行移動量や回転角）が取得できる。
					// グローバル値が欲しい場合、キーフレームが存在している時刻情報だけ取得して
					// あとは EvaluateGlobalTransform() で取得する方法もある。
					// FBX ファイルの作り方（キーの打ち方）によっては X, Y, Z, P, H, B のキーフレームの数が異なるということもありえるので注意。
					findKeyFramedTimes(keyFramedTimesSet, pCurveTranslateX, animPeriod.Get());
					findKeyFramedTimes(keyFramedTimesSet, pCurveTranslateY, animPeriod.Get());
					findKeyFramedTimes(keyFramedTimesSet, pCurveTranslateZ, animPeriod.Get());
					findKeyFramedTimes(keyFramedTimesSet, pCurveRotateX, animPeriod.Get());
					findKeyFramedTimes(keyFramedTimesSet, pCurveRotateY, animPeriod.Get());
					findKeyFramedTimes(keyFramedTimesSet, pCurveRotateZ, animPeriod.Get());
					ATLTRACE("KeyFrameCount = %Iu\n", keyFramedTimesSet.size());
#if 0
					if (pCurveTranslateX && pCurveTranslateY && pCurveTranslateZ)
					{
						ATLTRACE("CurveT.Count = (%d, %d, %d)\n",
							pCurveTranslateX->KeyGetCount(),
							pCurveTranslateY->KeyGetCount(),
							pCurveTranslateZ->KeyGetCount());
#if 0
						for (int k = 0; k < pCurveTranslateX->KeyGetCount(); ++k)
						{
							pCurveTranslateX->KeyGetTime(k);
							pCurveTranslateX->KeyGetValue(k);
						}
#endif
					}
					if (pCurveRotateX && pCurveRotateY && pCurveRotateZ)
					{
						ATLTRACE("CurveR.Count = (%d, %d, %d)\n",
							pCurveRotateX->KeyGetCount(),
							pCurveRotateY->KeyGetCount(),
							pCurveRotateZ->KeyGetCount());
					}
#endif
				}
#endif

				// 最大フレーム数。
				// ノードがアニメーションなしの場合、フレーム数がゼロになる。
				const int frameNum = stopFrame;
				//const int frameNum = 0;

				for (int f = 0; f < frameNum; ++f)
				{
					// 指定フレームの時刻を取得。
					const auto time = animPeriod * f; // 開始時刻はゼロであることが前提。

					// ノードからグローバル姿勢行列を取得する。

					//const auto fbxGlobalMat = pClusterLinkNode->GetGlobalFromCurrentTake(time); // FBX SDK 2011.3 以降は宣言だけ残っていて実装が削除されているのでリンクエラーになる。
					const auto fbxGlobalMat = pClusterLinkNode->EvaluateGlobalTransform(time);
					//const auto fbxLocalMat = pClusterLinkNode->EvaluateLocalTransform(time); // フレームで変動せず、一定らしい。書き出しに使ったプログラムコードにも寄る？
#if 0
					// 検証用に平行移動成分をダンプしてみる。
					const auto globalT = fbxGlobalMat.GetT();
					const auto localT = fbxLocalMat.GetT();
					ATLTRACE("Bone[%3d], Frame[%3d], GlobalT(%f, %f, %f), LocalT(%f, %f, %f)\n",
						i, f,
						globalT[0], globalT[1], globalT[2],
						localT[0], localT[1], localT[2]);
					// FbxAMatrix::GetR() でオイラー角（X-Y-Z 順）を取得できるらしい。
					const auto globalR = fbxGlobalMat.GetR();
					const auto localR = fbxLocalMat.GetR();
#endif
					//const auto fbxGlobalMat2 = pAnimEvaluator->GetNodeGlobalTransform(pClusterLinkNode, time);
					//const auto fbxLocalMat2 = pAnimEvaluator->GetNodeLocalTransform(pClusterLinkNode, time);
					// FbxNode::EvaluateGlobalTransform() と FbxAnimEvaluator::GetNodeGlobalTransform() は同じらしい。
					// FbxNode::EvaluateLocalTransform() と FbxAnimEvaluator::GetNodeLocalTransform() は同じらしい。
					// Doxygen コメントにも書いてある。

					// FBX SDK 付属の Transformations サンプルと、
					// http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx
					// を参考にしたが、ルート以外は初期姿勢行列の逆行列を乗じる必要があるらしい。つまり、初期姿勢を基準にすることになる。
					// なお、グラフィックス表示用に保持する行列が行優先か列優先かによって、行列の乗算順序が変わってくるので注意。
					// また、0 番クラスターのリンクがルートボーンであるという前提は通用しないので注意。
					// 例えば、FBX Converter 付属の LocalMotionBlend.fbx などはルートが 0 番でない。
					MyMath::MatrixF mat;
					//const int rootBoneIndex = 0;
#if 1
					if (i != m_rootBoneIndex)
					{
						//MyFbx::ToMatrixF(mat, fbxGlobalMat * fbxClusterTransformLinkMatrixInv); // FBX Exporter for MQO はコレで OK だが、一般的には不十分？
						// 折衷案として Cluster.TransformMatrix と Cluster.TransformLinkMatrix^-1 の積を用いる。
						MyFbx::ToMatrixF(mat, fbxGlobalMat * fbxClusterTransformMatrix * fbxClusterTransformLinkMatrixInv);
					}
					else
					{
						//MyFbx::ToMatrixF(mat, fbxGlobalMat); // FBX Exporter for MQO はコレで OK だが、一般的には不十分？
						MyFbx::ToMatrixF(mat, fbxGlobalMat * fbxClusterTransformMatrix);
					}
#else
					MyFbx::ToMatrixF(mat, fbxGlobalMat * fbxClusterTransformMatrix); // NG. ……のはずだが、LocalMotionBlend.fbx はコレで OK？
#endif
					boneAnimInfo->AddGlobalFrameAttitudeMatrix(mat);

					// パーツ別モーション ブレンドをするんだったら、グローバル変換行列じゃなくって、
					// 旧 DirectX RM の X フォーマットのようにローカル変換行列を使った上で親子関係を考慮していかなければダメなはず。
					// FBX のグローバル変換行列はボーン親子関係を考慮しない、完全にボーンごとに独立したものなので、
					// シングル アニメーションの再生には便利だし分かりやすいが、
					// パーツ別モーション ブレンドには向かない。
					// もしローカル変換行列が取得or計算できない場合、パーツ別モーション ブレンドが不可能になる？
					// FBX SDK の API を使ってブレンドしない場合、階層構造の構築と再帰探索の相当機能を自前で実装する必要がある。
					// 階層構造はボーン配列に対するインデックス ツリーを作ればよい。
				} // end of frame
			} // end of animation
		} // end of cluster
#pragma endregion

		//pAnimEvaluator->SetContext(pDefaultAnimStack);
		scene->SetCurrentAnimationStack(pDefaultAnimStack);
	}

	TBoneInfluenceArrayConstPtr MyFbxSkinAnalyzer::GetBoneInflInfo(int vertexIndex) const
	{
		const auto it = m_boneInflMap.find(vertexIndex);
		if (it != m_boneInflMap.end())
		{
			return it->second;
		}
		else
		{
			return TBoneInfluenceArrayConstPtr();
		}
	}

} // end of namespace
