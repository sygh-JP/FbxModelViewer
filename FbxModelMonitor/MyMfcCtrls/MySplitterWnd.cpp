#include "stdafx.h"
#include "MySplitterWnd.h"

#include "DebugNew.h"


BEGIN_MESSAGE_MAP(CMySplitterWnd, CSplitterWndEx)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


int CMySplitterWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//lpCreateStruct->style |= WS_CLIPCHILDREN;
	if (CSplitterWndEx::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	// TODO:  ここに特定な作成コードを追加してください。

	this->ModifyStyle(0, WS_CLIPCHILDREN);

	return 0;
}


BOOL CMySplitterWnd::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	//return CSplitterWndEx::OnEraseBkgnd(pDC);
	return true;
}

void CMySplitterWnd::OnInvertTracker(const CRect& rect)
{
	// On invert tracker relies on drawing on the child windows,
	// so WS_CLIPCHILDREN style must not be set, otherwise the
	// splitter bar will be clipped and hence invisible.
	const BOOL isChanged = this->ModifyStyle(WS_CLIPCHILDREN, 0);
	__super::OnInvertTracker(rect);
	if (isChanged)
	{
		this->ModifyStyle(0, WS_CLIPCHILDREN);
	}
}
