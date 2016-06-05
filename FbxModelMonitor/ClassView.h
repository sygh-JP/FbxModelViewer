// この MFC サンプル ソース コードでは、MFC Microsoft Office Fluent ユーザー インターフェイス 
// ("Fluent UI") の使用方法を示します。このコードは、MFC C++ ライブラリ ソフトウェアに 
// 同梱されている Microsoft Foundation Class リファレンスおよび関連電子ドキュメントを
// 補完するための参考資料として提供されます。
// Fluent UI を複製、使用、または配布するためのライセンス条項は個別に用意されています。
// Fluent UI ライセンス プログラムの詳細については、Web サイト
// http://msdn.microsoft.com/officeui を参照してください。
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

#pragma once

//#include "ViewTree.h"

class CClassToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler) override
	{
		__super::OnUpdateCmdUI(static_cast<CFrameWnd*>(this->GetOwner()), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const override { return FALSE; }
	virtual void PreSubclassWindow() override { m_bLargeIconsAreEnbaled = FALSE; }
};


class CMyMaterialListCtrl : public CMFCListCtrl
{
public:
	CMyMaterialListCtrl()
		: m_isPrevNewSelected()
		, m_isPrevOldSelected()
	{}
	virtual ~CMyMaterialListCtrl()
	{}
public:
	//static const size_t TempItemCount = 100;

public:
	void UpdateListItemCount();

private:
	bool m_isPrevNewSelected;
	bool m_isPrevOldSelected;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
};


class CClassView : public CDockablePane
{
public:
	CClassView();
	virtual ~CClassView();

	virtual void AdjustLayout() override;
	void OnChangeVisualStyle();

private:
	CClassToolBar m_wndToolBar;
	//CViewTree m_wndClassView;
	//CMFCListCtrl m_wndListCtrl;
	CMyMaterialListCtrl m_wndListCtrl;
	CImageList m_ClassViewImages;
	UINT m_nCurrSort;

public:
	void UpdateListItemCount()
	{ m_wndListCtrl.UpdateListItemCount(); }

public:
	const CListCtrl& GetListCtrl() const { return m_wndListCtrl; }
	CListCtrl& GetListCtrl() { return m_wndListCtrl; }

private:
	void FillClassView();

	static void InitListHeader(CListCtrl& listCtrl);

	// オーバーライド
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnClassAddMemberFunction();
	afx_msg void OnClassAddMemberVariable();
	afx_msg void OnClassDefinition();
	afx_msg void OnClassProperties();
	afx_msg void OnNewFolder();
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg LRESULT OnChangeActiveTab(WPARAM, LPARAM);
	afx_msg void OnSort(UINT id);
	afx_msg void OnUpdateSort(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

