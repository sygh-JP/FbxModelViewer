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

// FbxModelMonitorView.h : CFbxModelMonitorView クラスのインターフェイス
//


#pragma once


class CFbxModelMonitorView : public CView
{
protected: // シリアル化からのみ作成します。
	CFbxModelMonitorView();
	DECLARE_DYNCREATE(CFbxModelMonitorView)

	// 属性
public:
	CFbxModelMonitorDoc* GetDocument() const;

	// 操作
public:

	// オーバーライド
public:
	virtual void OnDraw(CDC* pDC) override;  // このビューを描画するためにオーバーライドされます。
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo) override;
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) override;
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) override;

	// 実装
public:
	virtual ~CFbxModelMonitorView();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

private:
	bool m_isMouseButtonPressedL;
	bool m_isMouseButtonPressedM;
	bool m_isMouseButtonPressedR;
	CPoint m_mousePointOld;

private:
	bool GetIsNoButtonPressed() const
	{ return !m_isMouseButtonPressedL && !m_isMouseButtonPressedM && !m_isMouseButtonPressedR; }

	// 生成された、メッセージ割り当て関数
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#ifndef _DEBUG
inline CFbxModelMonitorDoc* CFbxModelMonitorView::GetDocument() const
//{ return reinterpret_cast<CFbxModelMonitorDoc*>(m_pDocument); } // MFC ウィザードが吐くコードだが、継承関係は静的にさかのぼれるので static_cast で十分。
{ return static_cast<CFbxModelMonitorDoc*>(m_pDocument); }
#endif

