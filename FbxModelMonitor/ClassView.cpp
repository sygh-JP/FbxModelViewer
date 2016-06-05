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
#include "MainFrm.h"
#include "ClassView.h"
#include "Resource.h"
#include "FbxModelMonitor.h"

#include "DebugNew.h"


class CClassViewMenuButton : public CMFCToolBarMenuButton
{
	friend class CClassView;

	DECLARE_SERIAL(CClassViewMenuButton)

public:
	explicit CClassViewMenuButton(HMENU hMenu = nullptr)
		: CMFCToolBarMenuButton((UINT)-1, hMenu, -1)
	{
	}

	virtual void OnDraw(CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE,
		BOOL bCustomizeMode = FALSE, BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE) override
	{
		pImages = CMFCToolBar::GetImages();

		CAfxDrawState ds;
		pImages->PrepareDrawImage(ds);

		CMFCToolBarMenuButton::OnDraw(pDC, rect, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons);

		pImages->EndDrawImage(ds);
	}
};

IMPLEMENT_SERIAL(CClassViewMenuButton, CMFCToolBarMenuButton, 1);

//////////////////////////////////////////////////////////////////////
// コンストラクション/デストラクション
//////////////////////////////////////////////////////////////////////

CClassView::CClassView()
{
	m_nCurrSort = ID_SORTING_GROUPBYTYPE;
}

CClassView::~CClassView()
{
}

BEGIN_MESSAGE_MAP(CClassView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_CLASS_ADD_MEMBER_FUNCTION, OnClassAddMemberFunction)
	ON_COMMAND(ID_CLASS_ADD_MEMBER_VARIABLE, OnClassAddMemberVariable)
	ON_COMMAND(ID_CLASS_DEFINITION, OnClassDefinition)
	ON_COMMAND(ID_CLASS_PROPERTIES, OnClassProperties)
	ON_COMMAND(ID_NEW_FOLDER, OnNewFolder)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_COMMAND_RANGE(ID_SORTING_GROUPBYTYPE, ID_SORTING_SORTBYACCESS, OnSort)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SORTING_GROUPBYTYPE, ID_SORTING_SORTBYACCESS, OnUpdateSort)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassView メッセージ ハンドラ

int CClassView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

#if 0
	// ビューの作成:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndClassView.Create(dwViewStyle, rectDummy, this, 2))
	{
		//TRACE0("クラス ビューを作成できませんでした\n");
		TRACE0("Failed to create Class View\n");
		return -1;
	}
#endif
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_NOSORTHEADER | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
	// LVS_OWNERDATA を指定すると自動ソートできなくなる。

	if (!m_wndListCtrl.Create(dwViewStyle, rectDummy, this, 2))
	{
		TRACE0("Failed to create Material List View.\n");
		return -1;
	}

	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
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
	::SetWindowTheme(m_wndListCtrl.GetSafeHwnd(), L"Explorer", nullptr);

	InitListHeader(m_wndListCtrl);

	// イメージの読み込み:
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_SORT);
	m_wndToolBar.LoadToolBar(IDR_SORT, 0, 0, TRUE /* ロックされています*/);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// すべてのコマンドが、親フレーム経由ではなくこのコントロール経由で渡されます:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	CMenu menuSort;
	menuSort.LoadMenu(IDR_POPUP_SORT);

	m_wndToolBar.ReplaceButton(ID_SORT_MENU, CClassViewMenuButton(menuSort.GetSubMenu(0)->GetSafeHmenu()));

	CClassViewMenuButton* pButton =  DYNAMIC_DOWNCAST(CClassViewMenuButton, m_wndToolBar.GetButton(0));

	if (pButton != nullptr)
	{
		pButton->m_bText = FALSE;
		pButton->m_bImage = TRUE;
		pButton->SetImage(GetCmdMgr()->GetCmdImage(m_nCurrSort));
		pButton->SetMessageWnd(this);
	}

	// 静的ツリー ビュー データ (ダミー コード) を入力します
	FillClassView();

	return 0;
}

void CClassView::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CClassView::FillClassView()
{
#if 0
	HTREEITEM hRoot = m_wndClassView.InsertItem(_T("FakeApp クラス"), 0, 0);
	m_wndClassView.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);

	HTREEITEM hClass = m_wndClassView.InsertItem(_T("CFakeAboutDlg"), 1, 1, hRoot);
	m_wndClassView.InsertItem(_T("CFakeAboutDlg()"), 3, 3, hClass);

	m_wndClassView.Expand(hRoot, TVE_EXPAND);

	hClass = m_wndClassView.InsertItem(_T("CFakeApp"), 1, 1, hRoot);
	m_wndClassView.InsertItem(_T("CFakeApp()"), 3, 3, hClass);
	m_wndClassView.InsertItem(_T("InitInstance()"), 3, 3, hClass);
	m_wndClassView.InsertItem(_T("OnAppAbout()"), 3, 3, hClass);

	hClass = m_wndClassView.InsertItem(_T("CFakeAppDoc"), 1, 1, hRoot);
	m_wndClassView.InsertItem(_T("CFakeAppDoc()"), 4, 4, hClass);
	m_wndClassView.InsertItem(_T("~CFakeAppDoc()"), 3, 3, hClass);
	m_wndClassView.InsertItem(_T("OnNewDocument()"), 3, 3, hClass);

	hClass = m_wndClassView.InsertItem(_T("CFakeAppView"), 1, 1, hRoot);
	m_wndClassView.InsertItem(_T("CFakeAppView()"), 4, 4, hClass);
	m_wndClassView.InsertItem(_T("~CFakeAppView()"), 3, 3, hClass);
	m_wndClassView.InsertItem(_T("GetDocument()"), 3, 3, hClass);
	m_wndClassView.Expand(hClass, TVE_EXPAND);

	hClass = m_wndClassView.InsertItem(_T("CFakeAppFrame"), 1, 1, hRoot);
	m_wndClassView.InsertItem(_T("CFakeAppFrame()"), 3, 3, hClass);
	m_wndClassView.InsertItem(_T("~CFakeAppFrame()"), 3, 3, hClass);
	m_wndClassView.InsertItem(_T("m_wndMenuBar"), 6, 6, hClass);
	m_wndClassView.InsertItem(_T("m_wndToolBar"), 6, 6, hClass);
	m_wndClassView.InsertItem(_T("m_wndStatusBar"), 6, 6, hClass);

	hClass = m_wndClassView.InsertItem(_T("Globals"), 2, 2, hRoot);
	m_wndClassView.InsertItem(_T("theFakeApp"), 5, 5, hClass);
	m_wndClassView.Expand(hClass, TVE_EXPAND);
#endif
}

void CClassView::InitListHeader(CListCtrl& listCtrl)
{
	// 見出しの設定。
	LVCOLUMN lvc = {};
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	struct ColumnData
	{
		UINT CaptionStringResId;
		int Width;
		int Format;
	};

	// 数値以外は左寄せが基本。
	const ColumnData pColumnDataArray[] =
	{
		// 0番カラムは寄せ指定が効かない。デフォルトの左寄せになる。
		{ IDS_CLASS_VIEW, 100, LVCFMT_LEFT },
		{ IDS_CLASS_VIEW, 100, LVCFMT_RIGHT },
		{ IDS_CLASS_VIEW, 100, LVCFMT_RIGHT },
		{ IDS_CLASS_VIEW, 100, LVCFMT_RIGHT },
	};
	const int columnNum = ARRAYSIZE(pColumnDataArray);

	for (int i = 0; i < columnNum; ++i)
	{
		// サブアイテム番号。
		lvc.iSubItem = i;
		// 見出しテキスト。
		CString strTempLabel;
		strTempLabel.LoadString(pColumnDataArray[i].CaptionStringResId);
		lvc.pszText = strTempLabel.GetBuffer();
		// 横幅。
		lvc.cx = pColumnDataArray[i].Width;
		lvc.fmt = pColumnDataArray[i].Format;
		const int newColIndex = listCtrl.InsertColumn(i, &lvc);
		ASSERT(newColIndex != -1);
		strTempLabel.ReleaseBuffer();
	}
}

void CClassView::OnContextMenu(CWnd* pWnd, CPoint point)
{
#if 0
	CTreeCtrl* pWndTree = dynamic_cast<CTreeCtrl*>(&m_wndClassView);
	ASSERT_VALID(pWndTree);

	if (pWnd != pWndTree)
	{
		CDockablePane::OnContextMenu(pWnd, point);
		return;
	}

	if (point != CPoint(-1, -1))
	{
		// クリックされた項目の選択:
		CPoint ptTree = point;
		pWndTree->ScreenToClient(&ptTree);

		UINT flags = 0;
		HTREEITEM hTreeItem = pWndTree->HitTest(ptTree, &flags);
		if (hTreeItem != nullptr)
		{
			pWndTree->SelectItem(hTreeItem);
		}
	}

	pWndTree->SetFocus();
	CMenu menu;
	menu.LoadMenu(IDR_POPUP_SORT);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		(dynamic_cast<CMDIFrameWndEx*>(AfxGetMainWnd()))->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}
#else
	__super::OnContextMenu(pWnd, point);
#endif
}

void CClassView::AdjustLayout()
{
	if (GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	const int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_wndClassView.SetWindowPos(
	m_wndListCtrl.SetWindowPos(
		nullptr,
		rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

BOOL CClassView::PreTranslateMessage(MSG* pMsg)
{
	return __super::PreTranslateMessage(pMsg);
}

void CClassView::OnSort(UINT id)
{
	if (m_nCurrSort == id)
	{
		return;
	}

	m_nCurrSort = id;

	CClassViewMenuButton* pButton =  DYNAMIC_DOWNCAST(CClassViewMenuButton, m_wndToolBar.GetButton(0));

	if (pButton != nullptr)
	{
		pButton->SetImage(GetCmdMgr()->GetCmdImage(id));
		m_wndToolBar.Invalidate();
		m_wndToolBar.UpdateWindow();
	}
}

void CClassView::OnUpdateSort(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pCmdUI->m_nID == m_nCurrSort);
}

void CClassView::OnClassAddMemberFunction()
{
	AfxMessageBox(_T("メンバ関数の追加..."));
}

void CClassView::OnClassAddMemberVariable()
{
	// TODO: ここにコマンド ハンドラ コードを追加します
}

void CClassView::OnClassDefinition()
{
	// TODO: ここにコマンド ハンドラ コードを追加します
}

void CClassView::OnClassProperties()
{
	// TODO: ここにコマンド ハンドラ コードを追加します
}

void CClassView::OnNewFolder()
{
	AfxMessageBox(_T("新しいフォルダ..."));
}

void CClassView::OnPaint()
{
	CPaintDC dc(this); // 描画のデバイス コンテキスト

	CRect rectTree;
	//m_wndClassView.GetWindowRect(rectTree);
	m_wndListCtrl.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CClassView::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	//m_wndClassView.SetFocus();
	m_wndListCtrl.SetFocus();
}

void CClassView::OnChangeVisualStyle()
{
	m_ClassViewImages.DeleteImageList();

	UINT uiBmpId = g_theApp.m_bHiColorIcons ? IDB_CLASS_VIEW_24 : IDB_CLASS_VIEW;

	CBitmap bmp;
	if (!bmp.LoadBitmap(uiBmpId))
	{
		//TRACE(_T("ビットマップを読み込めませんでした: %x\n"), uiBmpId);
		TRACE(_T("Can't load bitmap: 0x%x\n"), uiBmpId);
		ASSERT(FALSE);
		return;
	}

	BITMAP bmpObj;
	bmp.GetBitmap(&bmpObj);

	UINT nFlags = ILC_MASK;

	nFlags |= (g_theApp.m_bHiColorIcons) ? ILC_COLOR24 : ILC_COLOR4;

	m_ClassViewImages.Create(16, bmpObj.bmHeight, nFlags, 0, 0);
	m_ClassViewImages.Add(&bmp, RGB(255, 0, 0));

	//m_wndClassView.SetImageList(&m_ClassViewImages, TVSIL_NORMAL);
	m_wndListCtrl.SetImageList(&m_ClassViewImages, LVSIL_NORMAL);

	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(g_theApp.m_bHiColorIcons ? IDB_SORT_24 : IDR_SORT, 0, 0, TRUE /* ロックされました*/);
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CMyMaterialListCtrl, CMFCListCtrl)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CMyMaterialListCtrl::OnLvnGetdispinfo)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CMyMaterialListCtrl::OnLvnItemchanged)
END_MESSAGE_MAP()

void CMyMaterialListCtrl::UpdateListItemCount()
{
	// コレがゼロだと LVN_GETDISPINFO が発行されない（コールバック関数は呼ばれない）。
	const size_t itemCount = CMainFrame::GetTheMainFrame()->GetGlobalMaterialsArray().size();
	VERIFY(this->SetItemCountEx(static_cast<int>(itemCount), LVSICF_NOINVALIDATEALL));
}

void CMyMaterialListCtrl::OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	if (pDispInfo->item.mask & LVIF_TEXT)
	{
		// 画面に表示されない部分はコールバック描画されないことの確認。
		//ATLTRACE("Item=Row=%d, SubItem=Column=%d\n", pDispInfo->item.iItem, pDispInfo->item.iSubItem);

		const int rowIndex = pDispInfo->item.iItem;
		const int columnIndex = pDispInfo->item.iSubItem;
		auto currMat = CMainFrame::GetTheMainFrame()->GetGlobalMaterialByIndex(rowIndex);
		if (currMat)
		{
			CString strTemp;

			switch (columnIndex)
			{
			case 0:
				strTemp = currMat->GetMaterialName().c_str();
				break;
			case 1:
				{
					const auto& diffuse = currMat->GetDiffuse();
					strTemp.Format(_T("%f, %f, %f, %f"), diffuse.x, diffuse.y, diffuse.z, diffuse.w);
				}
				break;
			case 2:
				strTemp.Format(_T("(%d, %d)"), rowIndex, columnIndex);
				break;
			case 3:
				strTemp.Format(_T("(%d, %d)"), rowIndex, columnIndex);
				break;
			default:
				break;
			}

			// LVITEM::cchTextMax は終端 null 文字分を含むバッファ長らしい。
			// http://msdn.microsoft.com/en-us/library/windows/desktop/bb774760.aspx
			_tcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strTemp.GetString(), _TRUNCATE); // 文字列をコピーする。
		}
		else
		{
			ATLASSERT(false);
		}
	}

	*pResult = 0;
}

// 選択状態に変更があったときに通知される。
void CMyMaterialListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// 単一行選択の場合を想定。複数行選択には非対応。
	if (pNMLV->uChanged & LVIF_STATE)
	{
		const int rowIndex = pNMLV->iItem;
		const bool isNewSelected = (pNMLV->uNewState & LVIS_SELECTED) != 0;
		const bool isOldSelected = (pNMLV->uOldState & LVIS_SELECTED) != 0;
		//ATLTRACE(__FUNCTIONW__ L"(), Row[%d] isNewSel=%d, isOldSel=%d\n", rowIndex, isNewSelected, isOldSelected);

		// リスト ビューで「すべて非選択になったとき」の検出が意外に難しい。
		// ・選択状態が [すべて非選択] から [A] へ変更された場合、
		// Row[ A] isNewSel=1, isOldSel=0
		// のみが飛んでくる。
		// ・選択状態が [A] から [B] へ変更された場合、
		// Row[ A] isNewSel=0, isOldSel=0
		// Row[-1] isNewSel=0, isOldSel=1
		// Row[ B] isNewSel=1, isOldSel=0
		// が連続して飛んでくる。
		// ・選択状態が [すべて非選択] になった場合、
		// Row[-1] isNewSel=0, isOldSel=1
		// のみが飛んでくる。
		if (isNewSelected)
		{
			ATLTRACE("Row[%d] is Selected.\n", rowIndex);
			CMainFrame::GetTheMainFrame()->SetTargetMaterialIndex(rowIndex);
		}
		else if (m_isPrevNewSelected && rowIndex == -1)
		{
			ATLTRACE("All Deselected.\n");
			CMainFrame::GetTheMainFrame()->SetTargetMaterialIndex(-1);
		}
		m_isPrevNewSelected = isNewSelected;
		m_isPrevOldSelected = isOldSelected;
	}

	*pResult = 0;
}

// NOTE: マテリアル ビューで選択されたマテリアルの情報を、プロパティ ビューで表示し、モードレスに編集できる。Visual Studio ライク。
// ダブルクリックなどでモーダル ダイアログを出して、編集できるようにする方法は楽だが使わない。
// Metasequoia も LightWave も、マテリアル エディター自体はダイアログ ウィンドウになっているが、
// リストで編集対象マテリアルをすぐに切り替えられるのは一緒。

// UNDONE: マテリアル編集のアンドゥ・リドゥの実装。
