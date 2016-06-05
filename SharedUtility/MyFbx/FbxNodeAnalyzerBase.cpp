#include "stdafx.h"

#include "FbxNodeAnalyzerBase.h"
#include "MyStopwatch.hpp"


namespace MyFbx
{

	MyFbxNodeAnalyzerBase::MyFbxNodeAnalyzerBase()
		: m_maxLevel()
		, m_levelCounter()
	{
	}

	MyFbxNodeAnalyzerBase::~MyFbxNodeAnalyzerBase()
	{
	}

	bool MyFbxNodeAnalyzerBase::AnalyzeScene(FbxScene* scene)
	{
		if (!scene)
		{
			return false;
		}

		_ASSERTE(m_meshAnalyzerArray.empty() && !m_skeletonAnalyzer);
		_ASSERTE(m_maxLevel == 0 && m_levelCounter == 0);

		// アニメーション時間情報の解析。
		// メッシュやスキン情報を解析する前に、まず最初に実行する必要がある。
		m_animTimeInfo.Analyze(scene);
		MyFbxAnimTimeInfo::AnalyzeLegacyTakeInfo(scene);

		//m_fbxFilePath = pFbxFilePath;

		this->OnPreAnalyze(scene);

		auto* pRootNode = scene->GetRootNode();
		if (!pRootNode)
		{
			return false;
		}

		const bool res = this->DispatchNodeByType(pRootNode);

		_ASSERTE(m_levelCounter == 0);

		this->OnPostAnalyze(scene);

		return res;
	}

	// FbxMesh::IsTriangleMesh() で三角形化する必要があるか否か判断するのは呼び出し元の責任。
	// このメソッドは問答無用で三角形化した複製メッシュを返す。
	// なお、複製したメッシュはシーンに含まれないので注意。
	static TFbxSdkMeshPtr CreateTriangulatedMesh(FbxMesh* pMesh)
	{
		// FBX SDK ライブラリのデバッグ ビルドではポリゴン数に比例して、三角形化にはかなり時間がかかる模様。
		// リリース ビルドではそれなりに高速。できれば FBX ファイルを出力する際に、3D ツール（モデラー）側で三角形化しておいたほうがいい。
		FbxGeometryConverter geoConv(pMesh->GetFbxManager());

		MyUtil::HRStopwatch stopwatch;
		stopwatch.Start();

		// Web で見かける古いコードで使われている TriangulateMesh() は deprecated らしい。
		// ちなみに古いバージョンでは穴あきポリゴンの三角形化はサポートされていなかったらしい。
		// ちなみに、Triangulate() メソッドの pReplace に true を指定すると、
		// pNodeAttribute パラメータに渡したオブジェクトがマジで delete (Destroy) されて無効ポインタになるらしい。
		auto newAttribute = geoConv.Triangulate(pMesh, false); // false を指定すると、変換元を削除しない。

		stopwatch.Stop();
		ATLTRACE(__FUNCTION__ "() Elapsed time to triangulate = %I64d[ms]\n", stopwatch.GetElapsedTimeInMilliseconds());

		_ASSERTE(newAttribute->GetAttributeType() == FbxNodeAttribute::eMesh);
		auto newMesh = static_cast<FbxMesh*>(newAttribute);

		return TFbxSdkMeshPtr(newMesh, Deleter<FbxMesh>());
	}


	bool MyFbxNodeAnalyzerBase::DispatchNodeByType(FbxNode* node)
	{
		auto* attrib = node->GetNodeAttribute();
		if (!attrib)
		{
			// MQO オブジェクト側の階層構造は、ボーン（スケルトン）階層構造とは別物。
			// FBX Exporter for MQO を使うと、
			// ポリゴンを持たない空の MQO オブジェクトは Attribute のないルートノードに変換されるらしい。
			// ポリゴンはなくてもローカル座標系を持つことはあるので、そのあたりの検証が必要。

			// TODO: 空のルートノードのローカル座標系を取り出すならばこのタイミング。
			// メッシュとともにローカル座標系に関する階層構造を作ったほうがよい。
			this->OnFindFbxRootNode(node);
		}
		else
		{
			const auto type = attrib->GetAttributeType();

			switch (type)
			{
			case FbxNodeAttribute::eUnknown:
				this->OnFindFbxUnidentifiedNode(node);
				break;
			case FbxNodeAttribute::eNull:
				this->OnFindFbxNullNode(node, StaticDownCast<FbxNull>(attrib));
				break;
			case FbxNodeAttribute::eMarker:
				this->OnFindFbxMarkerNode(node, StaticDownCast<FbxMarker>(attrib));
				break;
			case FbxNodeAttribute::eSkeleton:
				{
					//auto skeleton = StaticDownCast<FbxSkeleton>(attrib);
					auto skeleton = node->GetSkeleton();
					_ASSERTE(skeleton);
					this->AnalyzeSkeletonNode(node, skeleton);
					this->OnFindFbxSkeletonNode(node, skeleton);
				}
				break;
			case FbxNodeAttribute::eMesh:
				{
					//auto mesh = StaticDownCast<FbxMesh>(attrib);
					auto mesh = node->GetMesh();
					_ASSERTE(mesh);
					// FBX SDK による三角形化は完全のはずだが重いので、不完全だがより軽量な独自アルゴリズムを後で使う。
					// HACK: メッシュ コンバート機能を実装するときは、FBX SDK に任せる。
#if 0
					if (!mesh->IsTriangleMesh())
					{
						auto triMesh = CreateTriangulatedMesh(mesh);
						auto meshAnalyzer = this->AnalyzeMeshNode(node, triMesh.get());
						this->OnFindFbxMeshNode(node, mesh, meshAnalyzer.get());
					}
					else
#endif
					{
						auto meshAnalyzer = this->AnalyzeMeshNode(node, mesh);
						this->OnFindFbxMeshNode(node, mesh, meshAnalyzer.get());
					}
				}
				break;
			case FbxNodeAttribute::eNurbs:
				this->OnFindFbxNurbsNode(node, StaticDownCast<FbxNurbs>(attrib));
				break;
			case FbxNodeAttribute::ePatch:
				this->OnFindFbxPatchNode(node, StaticDownCast<FbxPatch>(attrib));
				break;
			case FbxNodeAttribute::eCamera:
				this->OnFindFbxCameraNode(node, StaticDownCast<FbxCamera>(attrib));
				break;
			case FbxNodeAttribute::eCameraStereo:
				this->OnFindFbxCameraStereoNode(node, StaticDownCast<FbxCameraStereo>(attrib));
				break;
			case FbxNodeAttribute::eCameraSwitcher:
				this->OnFindFbxCameraSwitcherNode(node, StaticDownCast<FbxCameraSwitcher>(attrib));
				break;
			case FbxNodeAttribute::eLight:
				this->OnFindFbxLightNode(node, StaticDownCast<FbxLight>(attrib));
				break;
			case FbxNodeAttribute::eOpticalReference:
				this->OnFindFbxOpticalReferenceNode(node, StaticDownCast<FbxOpticalReference>(attrib));
				break;
			case FbxNodeAttribute::eOpticalMarker:
				this->OnFindFbxOpticalMarkerNode(node);
				break;
			case FbxNodeAttribute::eNurbsCurve:
				this->OnFindFbxNurbsCurveNode(node, StaticDownCast<FbxNurbsCurve>(attrib));
				break;
			case FbxNodeAttribute::eTrimNurbsSurface:
				this->OnFindFbxTrimNurbsSurfaceNode(node, StaticDownCast<FbxTrimNurbsSurface>(attrib));
				break;
			case FbxNodeAttribute::eBoundary:
				this->OnFindFbxBoundaryNode(node, StaticDownCast<FbxBoundary>(attrib));
				break;
			case FbxNodeAttribute::eNurbsSurface:
				this->OnFindFbxNurbsSurfaceNode(node, StaticDownCast<FbxNurbsSurface>(attrib));
				break;
			case FbxNodeAttribute::eShape:
				this->OnFindFbxShapeNode(node, StaticDownCast<FbxShape>(attrib));
				break;
			case FbxNodeAttribute::eLODGroup:
				this->OnFindFbxLodGroupNode(node, StaticDownCast<FbxLODGroup>(attrib));
				break;
			case FbxNodeAttribute::eSubDiv:
				this->OnFindFbxSubDivNode(node, StaticDownCast<FbxSubDiv>(attrib));
				break;
			case FbxNodeAttribute::eCachedEffect:
				this->OnFindFbxCachedEffectNode(node, StaticDownCast<FbxCachedEffect>(attrib));
				break;
			case FbxNodeAttribute::eLine:
				this->OnFindFbxLineNode(node, StaticDownCast<FbxLine>(attrib));
				break;
			default:
				// 想定外のノード。
				_ASSERTE(false);
				return false;
			}
		}

		// 子ノード解析前処理
		this->OnPreAnalyzeChildren(node);

		// 子ノード解析
		const int childNum = node->GetChildCount();
		// FBX ノード ツリーの最大深さを調べる。
		if (childNum > 0)
		{
			++m_levelCounter;
		}

		// 再帰。
		for (int i = 0; i < childNum; ++i)
		{
			auto* child = node->GetChild(i);
			if (!this->DispatchNodeByType(child))
			{
				this->OnCatchFbxError(child);
			}
			m_maxLevel = (std::max)(m_maxLevel, m_levelCounter);
		}

		if (childNum > 0)
		{
			--m_levelCounter;
		}

		// 子ノード解析後処理
		this->OnPostAnalyzeChildren(node);

		return true;
	}


} // end of namespace

