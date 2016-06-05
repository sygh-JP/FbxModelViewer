
#pragma once

#if 0
/////////////////////////////////////////////////////////////////////
//! @brief  ログ用エディット コントロール。<br>
class CMyLogEditCtrl : public CEdit
{
	DECLARE_DYNAMIC(CMyLogEditCtrl)

public:
	CMyLogEditCtrl();
	virtual ~CMyLogEditCtrl();

private:
	// HACK: フォントや前景色・背景色はユーザーが設定できるようにする。Visual Studio のように、設定メニューはメイン フレームに備えたほうが良いかも。
	COLORREF m_foreColor;
	COLORREF m_backColor;
	CBrush m_backBrush;

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
};
#endif

/////////////////////////////////////////////////////////////////////
//! @brief  ログ ペイン用ツールバー。<br>
class CLogPaneToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler) override
	{
		__super::OnUpdateCmdUI(static_cast<CFrameWnd*>(this->GetOwner()), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const override { return FALSE; }
	virtual void PreSubclassWindow() override { m_bLargeIconsAreEnbaled = FALSE; }
};

#if 0
/////////////////////////////////////////////////////////////////////////////
// COutputList ウィンドウ

class COutputList : public CListBox
{
	// コンストラクション
public:
	COutputList();

	// 実装
public:
	virtual ~COutputList();

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
};
#endif

class COutputWnd : public CDockablePane
{
	// コンストラクション
public:
	COutputWnd();

	// 属性
private:
#if 0
	CFont m_Font;

	CMFCTabCtrl m_wndTabs;

	COutputList m_wndOutputBuild;
	COutputList m_wndOutputDebug;
	COutputList m_wndOutputFind;
#else
	//! @brief  エディット コントロール用フォント。<br>
	//! 通例、ログ用エディット コントロールには、プロポーショナルでなくモノスペース（等幅）フォントを使う。<br>
	CFont m_mainFont;

	// CEdit は文字列が長大になるとやたら重くなるので使わない。おそらく仮想化されていないせい。
	// もし CRichEditCtrl でもパフォーマンスが出ない場合、
	// WPF の TextBox や RichTextBox を使ったほうがよい。
	// ただし WPF の RichTextBox はメモリーを大量に消費するらしいが……
	// DirectWrite や Direct2D を直接叩いてコントロール描画を自前の直接モードでやるのは骨が折れるのでやめておいたほうがよい。

	//CMyLogEditCtrl m_mainCtrl;
	CRichEditCtrl m_mainCtrl;

	CLogPaneToolBar m_wndToolBar;
#endif

#if 0
protected:
	void FillBuildWindow();
	void FillDebugWindow();
	void FillFindWindow();

	void AdjustHorzScroll(CListBox& wndListBox);
#else
	virtual void AdjustLayout() override;
	void OnChangeVisualStyle();
#endif

	// 実装
public:
	virtual ~COutputWnd();

public:
#if 0
	const CEdit& GetEditCtrl() const { return m_mainCtrl; }
	CEdit& GetEditCtrl() { return m_mainCtrl; }
#else
	const CRichEditCtrl& GetEditCtrl() const { return m_mainCtrl; }
	CRichEditCtrl& GetEditCtrl() { return m_mainCtrl; }
#endif

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	afx_msg void OnLogProperties();
	afx_msg void OnEraseLog();
	afx_msg void OnSpecifyFont();

	DECLARE_MESSAGE_MAP()
};

