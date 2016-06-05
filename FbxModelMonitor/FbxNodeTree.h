#pragma once

#include "FbxNodeAnalyzerBase.h"
#include "MyDesktopHelpers.hpp"
#include "MyMfcHelpers.hpp"


namespace MyMfcFbx
{

	// MFC 専用 FBX ノード情報出力クラス。
	// 指定されたツリーコントロールおよびリッチエディットに FBX の階層構造などを出力する。
	class MyFbxNodeTree : public MyFbx::MyFbxNodeAnalyzerBase
	{
	public:
		explicit MyFbxNodeTree(CTreeCtrl* pTreeCtrl, CRichEditCtrl* pEditCtrl = nullptr);

		virtual ~MyFbxNodeTree();


	private:
		void InsertToLast(LPCWSTR pNodeLabelStr);

	private:
		virtual void OnPreAnalyze(FbxScene* scene) override;
		virtual void OnPostAnalyze(FbxScene* scene) override;

		virtual void OnPreAnalyzeChildren(FbxNode* parent) override;
		virtual void OnPostAnalyzeChildren(FbxNode* parent) override;

		virtual void OnCatchFbxError(FbxNode* errNode) override;

		virtual void OnFindFbxRootNode(FbxNode* root) override;

		virtual void OnFindFbxUnidentifiedNode(FbxNode* node) override;

		virtual void OnFindFbxNullNode(FbxNode* node, FbxNull* nullNode) override;

		virtual void OnFindFbxMarkerNode(FbxNode* node, FbxMarker* marker) override;

		virtual void OnFindFbxSkeletonNode(FbxNode* node, FbxSkeleton* skeleton) override;

		virtual void OnFindFbxMeshNode(FbxNode* node, FbxMesh* mesh, const MyFbx::MyFbxMeshAnalyzer* pMeshAnalyzer) override;

		virtual void OnFindFbxNurbsNode(FbxNode* node, FbxNurbs* nurbsNode) override;

		virtual void OnFindFbxPatchNode(FbxNode* node, FbxPatch* patch) override;

		virtual void OnFindFbxCameraNode(FbxNode* node, FbxCamera* camera) override;

		virtual void OnFindFbxCameraStereoNode(FbxNode* node, FbxCameraStereo* cameraStereo) override;

		virtual void OnFindFbxCameraSwitcherNode(FbxNode* node, FbxCameraSwitcher* cameraSwitcher) override;

		virtual void OnFindFbxLightNode(FbxNode* node, FbxLight* lignt) override;

		virtual void OnFindFbxOpticalReferenceNode(FbxNode* node, FbxOpticalReference* optRef) override;

		virtual void OnFindFbxOpticalMarkerNode(FbxNode* node) override;

		virtual void OnFindFbxNurbsCurveNode(FbxNode* node, FbxNurbsCurve* nurbsCurve) override;

		virtual void OnFindFbxTrimNurbsSurfaceNode(FbxNode* node, FbxTrimNurbsSurface* trimNurbsSurface) override;

		virtual void OnFindFbxBoundaryNode(FbxNode* node, FbxBoundary* boundary) override;

		virtual void OnFindFbxNurbsSurfaceNode(FbxNode* node, FbxNurbsSurface* nurbsSurface) override;

		virtual void OnFindFbxShapeNode(FbxNode* node, FbxShape* shape) override;

		virtual void OnFindFbxLodGroupNode(FbxNode* node, FbxLODGroup* lodGroup) override;

		virtual void OnFindFbxSubDivNode(FbxNode* node, FbxSubDiv* subDiv) override;

		virtual void OnFindFbxCachedEffectNode(FbxNode* node, FbxCachedEffect* cachedEffect) override;

		virtual void OnFindFbxLineNode(FbxNode* node, FbxLine* line) override;

	private:
		void OutputDetailInfoToEditCtrl() const;

	private:
		void AddStringToEditCtrlAndScrollToLastLine(LPCTSTR pString) const
		{
			ATLASSERT(m_pEditCtrl != nullptr);
			MyDesktopHelpers::AddStringToEditCtrlAndScrollToLastLine(*m_pEditCtrl, pString);
		}

		void AddStringLineToEditCtrlAndScrollToLastLine(LPCTSTR pString) const
		{
			ATLASSERT(m_pEditCtrl != nullptr);
			MyDesktopHelpers::AddStringLineToEditCtrlAndScrollToLastLine(*m_pEditCtrl, pString);
		}

	private:
		HTREEITEM GetLastTreeItem() const
		{
			ATLASSERT(m_pTreeCtrl != nullptr);
			const size_t size = m_itemStack.size();
			return (size == 0) ? TVI_ROOT : m_itemStack[size - 1]; // ルートか末尾。
		}

	private:
		CTreeCtrl* m_pTreeCtrl; // ターゲットのツリーコントロール
		//CEdit* m_pEditCtrl;
		CRichEditCtrl* m_pEditCtrl;
		std::vector<HTREEITEM> m_itemStack; // アイテムハンドルのスタック
	};
}

