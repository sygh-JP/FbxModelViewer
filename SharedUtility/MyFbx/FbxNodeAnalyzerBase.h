#pragma once

#include "FbxAnimTimeInfo.h"
#include "FbxMeshAnalyzer.h"
#include "FbxSkeletonAnalyzer.h"


namespace MyFbx
{
	// FBX ノード解析器の基底クラス。
	// 抽象クラスではなく、解析の基本機能はすべて提供しているので、そのままインスタンス化して使用できる。
	// ノード探索時のコールバック処理をカスタマイズしたい場合は派生クラスを実装する必要がある。
	// なお、派生クラスでのコールバック処理中に GUI オブジェクトを操作する場合、コールバックは必ず GUI スレッドから呼ばれているとは限らないことに注意。
	class MyFbxNodeAnalyzerBase : boost::noncopyable
	{
	public:
		using TSharedPtr = std::shared_ptr<MyFbxNodeAnalyzerBase>;
		using TConstSharedPtr = std::shared_ptr<const MyFbxNodeAnalyzerBase>;

	public:
		MyFbxNodeAnalyzerBase();

		virtual ~MyFbxNodeAnalyzerBase();

	public:
		// シーン解析エントリ。
		bool AnalyzeScene(FbxScene* scene);

	private:
		// ノード振り分け
		bool DispatchNodeByType(FbxNode* node);

		void AnalyzeSkeletonNode(FbxNode* node, const FbxSkeleton* skeleton)
		{
			// スケルトン解析
			if (m_skeletonAnalyzer.get() == nullptr)
			{
				m_skeletonAnalyzer = MyFbxSkeletonAnalyzer::TSharedPtr(new MyFbxSkeletonAnalyzer());
			}
			m_skeletonAnalyzer->Analyze(node, skeleton);
		}

		MyFbxMeshAnalyzer::TSharedPtr AnalyzeMeshNode(FbxNode* node, const FbxMesh* mesh)
		{
			// メッシュ解析
			const MyFbxMeshAnalyzer::TSharedPtr pMeshAnalyzer(new MyFbxMeshAnalyzer());
			m_meshAnalyzerArray.push_back(pMeshAnalyzer);
			pMeshAnalyzer->Analyze(node, mesh, m_animTimeInfo);
			return pMeshAnalyzer;
		}

#pragma region // 仮想関数群（コールバック イベント ハンドラー、カスタマイズ ポイント）//
	private:
		// 解析前処理
		virtual void OnPreAnalyze(FbxScene* scene) {}

		// 解析後処理
		virtual void OnPostAnalyze(FbxScene* scene) {}

		// 子ノード解析前処理
		virtual void OnPreAnalyzeChildren(FbxNode* parent) {}

		// 子ノード解析後処理
		virtual void OnPostAnalyzeChildren(FbxNode* parent) {}

		// エラーキャッチ
		virtual void OnCatchFbxError(FbxNode* errNode) {}

	private:
		// ルートノード
		virtual void OnFindFbxRootNode(FbxNode* root) {}

	private:
		// 不定ノード
		virtual void OnFindFbxUnidentifiedNode(FbxNode* node) {}

		// NULL ノード
		virtual void OnFindFbxNullNode(FbxNode* node, FbxNull* nullNode) {}

		// マーカーノード
		virtual void OnFindFbxMarkerNode(FbxNode* node, FbxMarker* marker) {}

	private:
		// スケルトンノード
		virtual void OnFindFbxSkeletonNode(FbxNode* node, FbxSkeleton* skeleton) {}

		// メッシュノード
		virtual void OnFindFbxMeshNode(FbxNode* node, FbxMesh* mesh, const MyFbxMeshAnalyzer* pMeshAnalyzer) {}

	private:
		// NURBs ノード
		virtual void OnFindFbxNurbsNode(FbxNode* node, FbxNurbs* nurbsNode) {}

		// パッチノード
		virtual void OnFindFbxPatchNode(FbxNode* node, FbxPatch* patch) {}

		// カメラノード
		virtual void OnFindFbxCameraNode(FbxNode* node, FbxCamera* camera) {}

		// カメラステレオノード
		virtual void OnFindFbxCameraStereoNode(FbxNode* node, FbxCameraStereo* cameraStereo) {}

		// カメラスイッチャーノード
		virtual void OnFindFbxCameraSwitcherNode(FbxNode* node, FbxCameraSwitcher* cameraSwitcher) {}

		// ライトノード
		virtual void OnFindFbxLightNode(FbxNode* node, FbxLight* lignt) {}

		// 光学参照ノード
		virtual void OnFindFbxOpticalReferenceNode(FbxNode* node, FbxOpticalReference* optRef) {}

		// 光学マーカーノード
		virtual void OnFindFbxOpticalMarkerNode(FbxNode* node) {}
		// NOTE: FbxOpticalMarker クラスは存在しないが、FbxMarker クラスは存在する。
		// FbxNodeAttribute::eMarker があるので、FbxNodeAttribute::eOpticalMarker とは関係ない？
		// FbxMarker::EType に eStandard と eOptical があるが、それで振り分ける？　だとすれば eOpticalMarker の存在意義は？

		// NOTE: 制約ノード KFbxNodeAttribute::eCONSTRAINT は、FBX SDK 2009.3 以降では存在しない模様。
		// eUNIDENTIFIED も存在せず、eUnknown で置き換えられている模様。
		// eNURB は eNurbs で置き換えられている模様。
		// その他はアンダースコア区切りから Camel 方式に変わっただけの模様。
		// 古い FBX SDK を使ったコードを移植・流用する際は注意。

		// NURBs カーブノード
		virtual void OnFindFbxNurbsCurveNode(FbxNode* node, FbxNurbsCurve* nurbsCurve) {}

		// トリム NURBs サーフェイスノード
		virtual void OnFindFbxTrimNurbsSurfaceNode(FbxNode* node, FbxTrimNurbsSurface* trimNurbsSurface) {}

		// 境界ノード
		virtual void OnFindFbxBoundaryNode(FbxNode* node, FbxBoundary* boundary) {}

		// NURBs サーフェイスノード
		virtual void OnFindFbxNurbsSurfaceNode(FbxNode* node, FbxNurbsSurface* nurbsSurface) {}

		// 形状ノード
		virtual void OnFindFbxShapeNode(FbxNode* node, FbxShape* shape) {}

		// LOD グループノード
		virtual void OnFindFbxLodGroupNode(FbxNode* node, FbxLODGroup* lodGroup) {}

		// SubDiv ノード
		virtual void OnFindFbxSubDivNode(FbxNode* node, FbxSubDiv* subDiv) {}

		// CachedEffect ノード
		virtual void OnFindFbxCachedEffectNode(FbxNode* node, FbxCachedEffect* cachedEffect) {}

		virtual void OnFindFbxLineNode(FbxNode* node, FbxLine* line) {}
#pragma endregion

	private:
		int m_maxLevel; // 最大階層数。
		int m_levelCounter; // 階層数カウンター。
		//std::wstring m_fbxFilePath; //!< .FBX ファイルの絶対パス。<br>
	private:
		MyFbxAnimTimeInfo m_animTimeInfo; // アニメーション時間情報
	private:
		std::vector<MyFbxMeshAnalyzer::TSharedPtr> m_meshAnalyzerArray; // メッシュ解析データ。
	private:
		MyFbxSkeletonAnalyzer::TSharedPtr m_skeletonAnalyzer; // スケルトン（ボーン階層）解析データ。

	public:
		const MyFbxAnimTimeInfo& GetAnimTimeInfo() const { return m_animTimeInfo; }
		const std::vector<MyFbxMeshAnalyzer::TSharedPtr>& GetMeshAnalyzerArray() const { return m_meshAnalyzerArray; }
		MyFbxSkeletonAnalyzer::TConstSharedPtr GetSkeletonAnalyzer() const { return m_skeletonAnalyzer; }
	protected:
		int GetHierarchyLevel() const { return m_maxLevel; }

	public:
		// 面をすべて三角形面で構成するように更新する。
		void ConvertAllMeshFacesAsTrianglesOnly()
		{
			for (auto pMeshAnalyzer : m_meshAnalyzerArray)
			{
				pMeshAnalyzer->ConvertAllBufferAsTrianglesOnly();
			}
		}
	};
}
