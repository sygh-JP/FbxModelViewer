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

#include "stdafx.h"
#include "ViewTree.h"

#if 0
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif
#include "DebugNew.h"


/////////////////////////////////////////////////////////////////////////////
// CViewTree

CViewTree::CViewTree()
{
}

CViewTree::~CViewTree()
{
}

BEGIN_MESSAGE_MAP(CViewTree, CTreeCtrl)
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewTree メッセージ ハンドラ

BOOL CViewTree::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	BOOL bRes = __super::OnNotify(wParam, lParam, pResult);

	NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
	ASSERT(pNMHDR != nullptr);

	if (pNMHDR && pNMHDR->code == TTN_SHOW && GetToolTips() != nullptr)
	{
		GetToolTips()->SetWindowPos(&wndTop, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
	}

	return bRes;
}


int CViewTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	// TODO:  ここに特定な作成コードを追加してください。

	// Win32/MFC のリスト ビュー、ツリー ビューはデフォルトで Aero ビジュアル テーマが適用されておらず、
	// 選択アイテムの背景色が単色の青だったり、ノード展開アイコンが三角形矢印でなく[+]/[-]だったりする。
	// Windows XP 以降で使用可能なテーマ API を明示的に使用することで、
	// Windows Vista/7/8 では、Aero エクスプローラー風のよりリッチな外観になる。
	// ただし、必ず LVS_EX_DOUBLEBUFFER を同時に指定すること（これも XP 以降で使用可能）。
	// でないと選択が変更されたときなどに背景消去されずに再描画結果がおかしくなる。
	// なお、オーナードローと併用・共存させる場合、
	// OpenThemeData(), DrawThemeBackground(), DrawThemeEdge(), ..., CloseThemeData() などを使って一部明示的に描画する必要がある。
	// ちなみに WPF 3.5 以降の ListView ではデフォルトで Aero スタイルになる。
	// WPF 4.0 以前には Aero2 テーマが実装されていないので、Windows 8 でも Aero2 ではなく Aero になる。
	::SetWindowTheme(this->GetSafeHwnd(), L"Explorer", nullptr);

	return 0;
}
