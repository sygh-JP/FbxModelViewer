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

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"

#include "DebugNew.h"


namespace
{
	const UINT PANE_TOOLBAR_RES_ID       = IDR_MY_TOOLBAR_LOG_PANE; //!< ペイン ツールバーのリソース ID。<br>
	const UINT PANE_TOOLBAR_IMAGE_RES_ID = IDR_MY_TOOLBAR_LOG_PANE; //!< ペイン ツールバー画像のリソース ID。<br>

	const COLORREF EditCtrlForeColor = RGB(0xF1, 0xF1, 0xF1);
	const COLORREF EditCtrlBackColor = RGB(0x25, 0x25, 0x25);
}

#if 0
/////////////////////////////////////////////////////////////////////
#pragma region // CMyLogEditCtrl //

IMPLEMENT_DYNAMIC(CMyLogEditCtrl, CEdit)


CMyLogEditCtrl::CMyLogEditCtrl()
: m_foreColor(EditCtrlForeColor)
, m_backColor(EditCtrlBackColor)
{
	m_backBrush.CreateSolidBrush(m_backColor);
}

CMyLogEditCtrl::~CMyLogEditCtrl()
{
}


BEGIN_MESSAGE_MAP(CMyLogEditCtrl, CEdit)
	//ON_WM_CTLCOLOR()
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()


// CMyLogEditCtrl メッセージ ハンドラ


BOOL CMyLogEditCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。

	if (pMsg->message == WM_KEYDOWN)
	{
		const BOOL isCtrlPressed = ::GetKeyState(VK_CONTROL) & 0x80;
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
			// Esc キーで CEdit が閉じてしまうのを防止する。
			return true;
		case 'A':
			if (isCtrlPressed)
			{
				this->SetSel(0, -1);
			}
			break;
		case 'C':
			if (isCtrlPressed)
			{
				this->Copy();
			}
			break;
		default:
			break;
		}
	}
	// メインフレームがキー入力の取得を邪魔するようなので、Ctrl+C は OnKeyDown() では受け取れない。
	// なので、PreTranslateMessage() でなんとかする。

	return __super::PreTranslateMessage(pMsg);
}

HBRUSH CMyLogEditCtrl::CtlColor(CDC* pDC, UINT nCtlColor)
{
	// 文字色
	pDC->SetTextColor(m_foreColor);
	// 背景色
	pDC->SetBkColor(m_backColor);
	return static_cast<HBRUSH>(m_backBrush.GetSafeHandle());
}

#pragma endregion
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputWnd

COutputWnd::COutputWnd()
{
}

COutputWnd::~COutputWnd()
{
}

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_MY_CMD_LOG_PROPERTIES, OnLogProperties)
	ON_COMMAND(ID_MY_CMD_ERASE_LOG, OnEraseLog)
	ON_COMMAND(ID_MY_CMD_SPECIFY_FONT, OnSpecifyFont)
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

#if 0
	m_Font.CreateStockObject(DEFAULT_GUI_FONT);
#else
	CClientDC dc(this);
	// 非対応文字を GDI フォント リンクで解決するようなフォントの場合、
	// 非対応文字が混ざる行では、デフォルトで MS UI Gothic になってしまう模様。
	// また、フォント リンクがうまく働かず、おかしな文字が表示される状況もある。
	// 例えば '♡' を Segoe UI で表示するとなぜか '≡' のような文字で表示されてしまう。
	// やはり時代遅れの GDI や Win32 コモン コントロールを今更使うよりも、WPF を使ったほうがよさげ。

	//m_mainFont.CreatePointFont(100, _T("ＭＳ ゴシック"), &dc);
	//m_mainFont.CreatePointFont(100, _T("Meiryo UI"), &dc);
	//m_mainFont.CreatePointFont(100, _T("Meiryo"), &dc);
	m_mainFont.CreatePointFont(100, _T("Consolas"), &dc);
#endif

	CRect rectDummy;
	rectDummy.SetRectEmpty();

#if 0
	// タブ付きウィンドウの作成:
	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 1))
	{
		//TRACE0("タブ付き出力ウィンドウを作成できませんでした\n");
		TRACE0("Failed to create output tab window\n");
		return -1;
	}

	// 出力ペインの作成:
	const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutputBuild.Create(dwStyle, rectDummy, &m_wndTabs, 2) ||
		!m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, 3) ||
		!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4))
	{
		//TRACE0("出力ウィンドウを作成できませんでした\n");
		TRACE0("Failed to create output windows\n");
		return -1;
	}

	m_wndOutputBuild.SetFont(&m_Font);
	m_wndOutputDebug.SetFont(&m_Font);
	m_wndOutputFind.SetFont(&m_Font);

	CString strTabName;
	BOOL bNameValid;

	// 一覧ウィンドウをタブに割り当てます:
	bNameValid = strTabName.LoadString(IDS_BUILD_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputBuild, strTabName, (UINT)0);
	bNameValid = strTabName.LoadString(IDS_DEBUG_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, (UINT)1);
	bNameValid = strTabName.LoadString(IDS_FIND_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)2);

	// 出力タブにダミー テキストを入力します
	FillBuildWindow();
	FillDebugWindow();
	FillFindWindow();
#else
	// Create view:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		ES_MULTILINE | ES_READONLY;

	const UINT ctrlID = 1;
	if (!m_mainCtrl.Create(dwViewStyle, rectDummy, this, ctrlID))
	{
		TRACE("Failed to create Log View.\n");
		return -1;
	}

	m_mainCtrl.SetFont(&m_mainFont);
	// CRichEditCtrl に SetFont() でフォントを設定すると、GetDefaultCharFormat() の結果が変わる。
	// 前景色を変える前に、先にフォントを設定してしまう。

#if 0
	// エディット ボックスに入力できる文字列は、既定では Single-line で 32K、Multi-line で 64K という制限がある。
	// Win16 時代の MFC では、CEdit::LimitText() のパラメータに 0 を指定すると、
	// テキストの最大長は UINT_MAX バイトに設定されるらしい。
	// Win32 では、CEdit::SetLimitText() に置き換わっているらしい。
	// Win16 では、UINT_MAX == USHRT_MAX == 0xffff だが、Win32 では UINT_MAX == ULONG_MAX == 0xffffffff となっている。
	m_mainCtrl.SetLimitText(0);
#else
	m_mainCtrl.SetBackgroundColor(false, EditCtrlBackColor);
	CHARFORMAT richEditCharFormat = {};
	richEditCharFormat.cbSize = sizeof(richEditCharFormat);
	m_mainCtrl.GetDefaultCharFormat(richEditCharFormat);
	richEditCharFormat.crTextColor = EditCtrlForeColor;
	richEditCharFormat.dwMask = CFM_COLOR;
	richEditCharFormat.dwEffects &= ~CFE_AUTOCOLOR;
	m_mainCtrl.SetDefaultCharFormat(richEditCharFormat);
#endif

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, PANE_TOOLBAR_RES_ID);
	m_wndToolBar.LoadToolBar(PANE_TOOLBAR_RES_ID, 0, 0, TRUE /* Is locked */);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control, not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	this->AdjustLayout();
#endif

	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

#if 0
	// タブ コントロールは、クライアント領域全体をカバーする必要があります:
	m_wndTabs.SetWindowPos(NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
#else
	this->AdjustLayout();
#endif
}

void COutputWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

#pragma region // コントロールの枠線。//
	CRect rectCtrl;
	m_mainCtrl.GetWindowRect(rectCtrl);
	ScreenToClient(rectCtrl);

	rectCtrl.InflateRect(1, 1);
	dc.Draw3dRect(rectCtrl, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
#pragma endregion
}

void COutputWnd::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	m_mainCtrl.SetFocus();
}

void COutputWnd::AdjustLayout()
{
	if (!this->GetSafeHwnd())
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	const int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_mainCtrl.SetWindowPos(nullptr, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::OnChangeVisualStyle()
{
	m_wndToolBar.CleanUpLockedImages();

	BOOL loadresult = m_wndToolBar.LoadBitmap(PANE_TOOLBAR_IMAGE_RES_ID, 0, 0, TRUE /* Locked */);
}

#if 0
void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_Font);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i ++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void COutputWnd::FillBuildWindow()
{
	m_wndOutputBuild.AddString(_T("ビルド出力データがここに表示されます。"));
	m_wndOutputBuild.AddString(_T("出力データはリスト ビューの各行に表示されます"));
	m_wndOutputBuild.AddString(_T("表示方法を変更することもできます..."));
	m_wndOutputBuild.AddString(_T("Build output is being displayed here."));
	m_wndOutputBuild.AddString(_T("The output is being displayed in rows of a list view"));
	m_wndOutputBuild.AddString(_T("but you can change the way it is displayed as you wish..."));
}

void COutputWnd::FillDebugWindow()
{
	m_wndOutputDebug.AddString(_T("デバッグ出力データがここに表示されます。"));
	m_wndOutputDebug.AddString(_T("出力データはリスト ビューの各行に表示されます"));
	m_wndOutputDebug.AddString(_T("表示方法を変更することもできます..."));
	m_wndOutputDebug.AddString(_T("Debug output is being displayed here."));
	m_wndOutputDebug.AddString(_T("The output is being displayed in rows of a list view"));
	m_wndOutputDebug.AddString(_T("but you can change the way it is displayed as you wish..."));
}

void COutputWnd::FillFindWindow()
{
	m_wndOutputFind.AddString(_T("検索出力データがここに表示されます。"));
	m_wndOutputFind.AddString(_T("出力データはリスト ビューの各行に表示されます"));
	m_wndOutputFind.AddString(_T("表示方法を変更することもできます..."));
	m_wndOutputFind.AddString(_T("Find output is being displayed here."));
	m_wndOutputFind.AddString(_T("The output is being displayed in rows of a list view"));
	m_wndOutputFind.AddString(_T("but you can change the way it is displayed as you wish..."));
}
#endif

void COutputWnd::OnLogProperties()
{
	AfxMessageBox(_T("Showing log properties has not been implemented yet!!\n[ログ プロパティの表示]は未実装です。"));
}

void COutputWnd::OnEraseLog()
{
	// CEdit::Clear() は現在選択されているテキストのみを削除する。
	// テキスト文字列を無条件にすべてクリアするには、SetWindowText() で空文字列を設定するしかなさそう。
	m_mainCtrl.SetWindowText(_T(""));
	//m_mainCtrl.Clear();
}

void COutputWnd::OnSpecifyFont()
{
	AfxMessageBox(_T("Specifying of font has not been implemented yet!!\n[フォントの指定]は未実装です。"));
}

#if 0
/////////////////////////////////////////////////////////////////////////////
// COutputList

COutputList::COutputList()
{
}

COutputList::~COutputList()
{
}

BEGIN_MESSAGE_MAP(COutputList, CListBox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// COutputList メッセージ ハンドラ

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnEditCopy()
{
	MessageBox(_T("出力データをコピーします"));
}

void COutputList::OnEditClear()
{
	MessageBox(_T("出力データをクリアします"));
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != NULL && pParentBar != NULL)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}
#endif


BOOL COutputWnd::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。

	if (pMsg->message == WM_KEYDOWN && m_mainCtrl.GetSafeHwnd())
	{
		const BOOL isCtrlPressed = ::GetKeyState(VK_CONTROL) & 0x80;
		switch (pMsg->wParam)
		{
		case 'A':
			if (isCtrlPressed)
			{
				m_mainCtrl.SetSel(0, -1);
			}
			break;
		case 'C':
			if (isCtrlPressed)
			{
				m_mainCtrl.Copy(); // リッチ エディットの場合、フォント情報もコピーされることに注意。
			}
			break;
		default:
			break;
		}
	}
	// メインフレームがキー入力の取得を邪魔するようなので、Ctrl+C は OnKeyDown() では受け取れない。
	// なので、PreTranslateMessage() でなんとかする。

	return __super::PreTranslateMessage(pMsg);
}
