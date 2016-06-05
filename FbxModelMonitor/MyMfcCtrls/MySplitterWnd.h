#pragma once

// MFC ウィザードで[分割ウィンドウ]を選択したときに、
// CSplitterWnd を使ったコードが出力されるが、
// メインフレームをリサイズするときに複数ビューの再描画がちらつく問題がある。
// ただし、CMySplitterWnd::CreateStatic() を呼び出しているコードで、WS_CLIPCHILDREN を追加指定するだけだと、
// 境界線をドラッグして子ビューのサイズを変更しようとするとき、
// CSplitterWnd::OnInvertTracker() のアサートに引っかかる。
// OnInvertTracker() をオーバーライドして WS_CLIPCHILDREN の設定を一時的に切り替える方法がある。
// ちなみに CSplitterWnd の代わりに CSplitterWndEx を使うと、境界線の描画色がビジュアル テーマに応じて変わる。


class CMySplitterWnd : public CSplitterWndEx
{
public:
	CMySplitterWnd()
	{}

private:
	virtual void OnInvertTracker(const CRect& rect) override;
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
