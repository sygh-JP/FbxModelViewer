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

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "FbxModelMonitor.h"


#include "DebugNew.h"


/////////////////////////////////////////////////////////////////////////////
// CPropertiesWnd

CPropertiesWnd::CPropertiesWnd()
{
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_COMMAND(ID_PROPERTIES1, OnProperties1)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
	ON_COMMAND(ID_PROPERTIES2, OnProperties2)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_WM_DESTROY()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar メッセージ ハンドラ

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient,rectCombo;
	GetClientRect(rectClient);

	m_wndObjectCombo.GetWindowRect(&rectCombo);

	int cyCmb = rectCombo.Size().cy;
	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndObjectCombo.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), 200, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top + cyCmb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + cyCmb + cyTlb, rectClient.Width(), rectClient.Height() -(cyCmb+cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// コンボ ボックスの作成:
	//const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndObjectCombo.Create(dwViewStyle, rectDummy, this, 1))
	{
		//TRACE0("プロパティ コンボ ボックスを作成できませんでした\n");
		TRACE0("Failed to create Properties Combo \n");
		return -1;
	}

	//m_wndObjectCombo.AddString(_T("アプリケーション"));
	//m_wndObjectCombo.AddString(_T("プロパティ ウィンドウ"));
	m_wndObjectCombo.AddString(_T("メッシュ プロパティ"));
	m_wndObjectCombo.AddString(_T("マテリアル プロパティ"));
	//m_wndObjectCombo.SetFont(CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT)));
	// --> コレ、VS 2008 SP1 の頃からあるコードだが、特に意味はあるのか……？
	// DEFAULT_GUI_FONT は Win7/Win8 でも "MS UI Gothic" だが、
	// SetPropListFont() でシステム フォント（たぶん Win7 でメイリオ、Win8 で Meiryo UI）を再設定することになるので意味がない。
	// ちなみに構築直後、SetFont() する前に GetFont() すると NULL が返る。
	m_wndObjectCombo.SetCurSel(0);

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		//TRACE0("プロパティ グリッドを作成できませんでした\n");
		TRACE0("Failed to create Properties Grid \n");
		return -1;
	}

	InitPropList();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* ロックされています*/);
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(g_theApp.m_bHiColorIcons ? IDB_PROPERTIES_HC : IDR_PROPERTIES, 0, 0, TRUE /* ロックされました*/);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	// すべてのコマンドが、親フレーム経由ではなくこのコントロール経由で渡されます:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties()
{
	m_wndPropList.ExpandAll();
}

void CPropertiesWnd::OnUpdateExpandAllProperties(CCmdUI* pCmdUI)
{
}

void CPropertiesWnd::OnSortProperties()
{
	m_wndPropList.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnProperties1()
{
	// TODO: ここにコマンド ハンドラ コードを追加します

	//CMainFrame::GetTheMainFrame()->ShowGradientEditor();
}

void CPropertiesWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: ここにコマンド更新 UI ハンドラ コードを追加します
}

void CPropertiesWnd::OnProperties2()
{
	// TODO: ここにコマンド ハンドラ コードを追加します

	// TDF_ALLOW_DIALOG_CANCELLATION を指定しないと、Close ボタンが消えて Esc キーも効かなくなる。
	CTaskDialog::ShowDialog(_T("Content"), _T("Main Instruction"), AfxGetAppName(), 0, 0, TDCBF_OK_BUTTON,
		TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION);
}

void CPropertiesWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: ここにコマンド更新 UI ハンドラ コードを追加します
}

void CPropertiesWnd::InitPropList()
{
	this->SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();

	auto* pTestVectorRootProp = m_propTestVector.Create(_T("Test vector"), MyMath::Vector4D(), MyMath::Vector4D(-1, -1, -1, -1), MyMath::Vector4D(+1, +1, +1, +1), 1.0e-3);

	m_wndPropList.AddProperty(pTestVectorRootProp);

	// デフォルトのマテリアルはグレー。
	MyMath::MyMaterial defaultMaterial;
	defaultMaterial.SetDiffuseColorRGB(MyMath::Vector3F(0.5f, 0.5f, 0.5f));
	defaultMaterial.SetDiffuseLevel(1);
	auto* pMaterialRootProp = m_propMaterial.Create(defaultMaterial);

	m_wndPropList.AddProperty(pMaterialRootProp);

	auto pGradStopsProp = new CMyGradientStopsProperty(_T("Toon Gradient"), L"", _T("Gradient stops for toon shading.\n"));

	//pGradStopsProp->AddOption(_T("hoge1"));
	//pGradStopsProp->AddOption(_T("hoge2"));

	m_wndPropList.AddProperty(pGradStopsProp);

#if 0
	{
		static const TCHAR szFilter[] = _T("アイコン ファイル (*.ico)|*.ico|すべてのファイル (*.*)|*.*||");
		m_wndPropList.AddProperty(new CMFCPropertyGridFileProperty(_T("アイコン"), TRUE, _T(""), _T("ico"), 0, szFilter, _T("ウィンドウ アイコンを指定します")));

		LOGFONT lf = {};
		lf.lfHeight = -12;
		_tcscpy_s(lf.lfFaceName, _T("Verdana"));
		m_wndPropList.AddProperty(new CMFCPropertyGridFontProperty(_T("フォント"), lf, CF_EFFECTS | CF_SCREENFONTS, _T("ウィンドウの既定フォントを指定します")));
	}
#endif

#if 0
	CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("表示"));

	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("3D 表示"), (_variant_t) false, _T("ウィンドウのフォントが太字以外になり、また、コントロールが 3D ボーダーで描画されます")));

	CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(_T("罫線"), _T("ダイアログ枠"), _T("次のうちのどれかです : なし、細枠、サイズ変更可能枠、ダイアログ枠"));
	pProp->AddOption(_T("なし"));
	pProp->AddOption(_T("細枠"));
	pProp->AddOption(_T("サイズ変更可能枠"));
	pProp->AddOption(_T("ダイアログ枠"));
	pProp->AllowEdit(FALSE);

	pGroup1->AddSubItem(pProp);
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("キャプション"), (_variant_t) _T("バージョン情報"), _T("ウィンドウのタイトル バーに表示されるテキストを指定します")));

	m_wndPropList.AddProperty(pGroup1);

	CMFCPropertyGridProperty* pSize = new CMFCPropertyGridProperty(_T("ウィンドウ サイズ"), 0, TRUE);

	pProp = new CMFCPropertyGridProperty(_T("高さ"), (_variant_t) 250l, _T("ウィンドウの高さを指定します"));
	pProp->EnableSpinControl(TRUE, 50, 300);
	pSize->AddSubItem(pProp);

	pProp = new CMFCPropertyGridProperty( _T("幅"), (_variant_t) 150l, _T("ウィンドウの幅を指定します"));
	pProp->EnableSpinControl(TRUE, 50, 200);
	pSize->AddSubItem(pProp);

	m_wndPropList.AddProperty(pSize);

	CMFCPropertyGridProperty* pGroup2 = new CMFCPropertyGridProperty(_T("フォント"));

	LOGFONT lf;
	CFont* font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	font->GetLogFont(&lf);

	lstrcpy(lf.lfFaceName, _T("ＭＳ Ｐゴシック"));

	pGroup2->AddSubItem(new CMFCPropertyGridFontProperty(_T("フォント"), lf, CF_EFFECTS | CF_SCREENFONTS, _T("ウィンドウの既定フォントを指定します")));
	pGroup2->AddSubItem(new CMFCPropertyGridProperty(_T("システム フォントを使用する"), (_variant_t) true, _T("ウィンドウで MS Shell Dlg フォントを使用するように指定します")));

	m_wndPropList.AddProperty(pGroup2);

	CMFCPropertyGridProperty* pGroup3 = new CMFCPropertyGridProperty(_T("その他"));
	pProp = new CMFCPropertyGridProperty(_T("(名前)"), _T("アプリケーション"));
	pProp->Enable(FALSE);
	pGroup3->AddSubItem(pProp);

	CMFCPropertyGridColorProperty* pColorProp = new CMFCPropertyGridColorProperty(_T("ウィンドウの色"), RGB(210, 192, 254), NULL, _T("ウィンドウの既定の色を指定します"));
	pColorProp->EnableOtherButton(_T("その他..."));
	pColorProp->EnableAutomaticButton(_T("既定値"), ::GetSysColor(COLOR_3DFACE));
	pGroup3->AddSubItem(pColorProp);

	// MFC ウィザードの生成したコードだが、BASED_CODE は NEAR/FAR と同じく Win32 では無意味。const も付いてないし、MFC チームは何をやっているんだ……
	static TCHAR BASED_CODE szFilter[] = _T("アイコン ファイル (*.ico)|*.ico|すべてのファイル (*.*)|*.*||");
	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("アイコン"), TRUE, _T(""), _T("ico"), 0, szFilter, _T("ウィンドウ アイコンを指定します")));

	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("フォルダ"), _T("c:\\")));

	m_wndPropList.AddProperty(pGroup3);

	CMFCPropertyGridProperty* pGroup4 = new CMFCPropertyGridProperty(_T("階層"));

	CMFCPropertyGridProperty* pGroup41 = new CMFCPropertyGridProperty(_T("1 番目のサブレベル"));
	pGroup4->AddSubItem(pGroup41);

	CMFCPropertyGridProperty* pGroup411 = new CMFCPropertyGridProperty(_T("2 番目のサブレベル"));
	pGroup41->AddSubItem(pGroup411);

	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("項目 1"), (_variant_t) _T("値 1"), _T("これは説明です")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("項目 2"), (_variant_t) _T("値 2"), _T("これは説明です")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("項目 3"), (_variant_t) _T("値 3"), _T("これは説明です")));

	pGroup4->Expand(FALSE);
	m_wndPropList.AddProperty(pGroup4);
#endif
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	// WM_SETTINGCHANGE はレジストリ設定や環境変数が変更されたときなどに送信されるらしい。
	// 画面解像度の変更などは WM_DISPLAYCHANGE を使って検出するとよいらしい。
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf = {};
	// Windows 7 日本語版ではデフォルトでメイリオになるらしい。Windows 8 日本語版ではたぶん Meiryo UI となる。
	// MFC 14.0 + Windows 10 日本語版では Segoe UI になった。
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info = {};
	info.cbSize = sizeof(info);

	// VC 2008 SP1 の AFX_GLOBAL_DATA::GetNonClientMetrics() には、/clr と組み合わせるときにバグがあったが、2010 以降では解消されているらしい？
	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
	m_wndObjectCombo.SetFont(&m_fntPropList);
}


void CPropertiesWnd::OnDestroy()
{
	__super::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}

LRESULT CPropertiesWnd::OnPropertyChanged(WPARAM wparam, LPARAM lparam)
{
	// NOTE: CMFCPropertyGridProperty のプロパティ値が GUI を使用して変更されたとき、
	// AFX_WM_PROPERTY_CHANGED メッセージがまずプロパティの管理者である CMFCPropertyGridCtrl に送られて、
	// さらにそのオーナーであるビュー（ペイン）に送られてくる。
	// ・WPARAM: プロパティ リストのコントロール ID。CMFCPropertyGridCtrl の作成時に指定する ID のことらしい。
	// ・LPARAM: 変更されたプロパティ (CMFCPropertyGridProperty) へのポインター。
	auto pProperty = reinterpret_cast<CMFCPropertyGridProperty*>(lparam);
	if (pProperty != nullptr)
	{
		// MFC オブジェクトとして有効であることの確認。
		ASSERT_VALID(pProperty);
		ATLTRACE(__FUNCTION__ "(), %Iu, 0x%p, %s\n", wparam, lparam, typeid(*pProperty).name());
		auto pGradProp = dynamic_cast<CMyGradientStopsProperty*>(pProperty);
		if (pGradProp)
		{
			ATLTRACE("CMyGradientStopsProperty.\n");
		}
	}

	// HACK: プロパティがひとつ更新されるたびに毎回 UI 状態を全取得して
	// ターゲット オブジェクトの全パラメータを書き換えるのではなく、部分的に書き換えるようにして高速化する。
	// その場合、CMFCPropertyGridProperty::GetData() / SetData() が使えるかも。
	// データの種類やデータ量が少ない場合は別に全書き換えでも十分いけるが、
	// D3D/GL テクスチャ データの書き換え（GPU との通信）などを伴う場合は、余計な書き換えが発生するとパフォーマンス的に厳しくなる。

#if 0
	MyMath::Vector4D temp;
	m_propTestVector.GetProperties(temp);
#endif

	auto currMat = CMainFrame::GetTheMainFrame()->GetCurrentTargetMaterial();

	if (currMat)
	{
		// マテリアルは名前をキーにしたテーブルで管理されているので、絶対にテーブルで管理されているオブジェクトの名前を直接書き換えてはいけない。
		// テクスチャのファイル名に関しても同様。
		// UNDONE: マテリアル名やテクスチャ名を実行時に書き換える場合、関連テーブルの書き換えおよびテクスチャのロードが必要。

		MyMath::MyMaterial newMaterialData;
		m_propMaterial.GetProperties(newMaterialData);
		if (!MyMath::CompareGradientColorStopArrays(currMat->GetToonGradientColorStops(), newMaterialData.GetToonGradientColorStops()))
		{
			ATLTRACE("ToonGradient is updated.\n");
			// TODO: テクスチャ DIB を書き換え、GPU に転送する。D3D11_MAP_WRITE_DISCARD を使う。
		}
		_ASSERTE(newMaterialData.GetMaterialName() == currMat->GetMaterialName());
		*currMat = newMaterialData;
		// TODO: マテリアル編集処理のアンドゥ バッファを実装したほうがよいかも。
	}
#if 0
	auto pActiveView = CMainFrame::GetTheMainFrame()->GetActiveView();
	if (pActiveView)
	{
		pActiveView->Invalidate(false);
		// スプリットの場合は片方だけが再描画される。
	}
#else
	// TODO: グラデーション プロパティからの変更通知だった場合、トゥーン グラデーション テクスチャを書き換える。
	// テクスチャの書き換えは結構負荷が高い処理なので、前回の設定値との比較をして、余計な書き換えは防止する必要がある。
	// 将来的にトゥーン グラデーションの他にもグラデーション プロパティを含む場合、
	// 前記の GetData() / SetData() によるマーカーなどを使った区別が必要になる。
	CMainFrame::GetTheMainFrame()->InvalidateAllViews();
#endif

	return 0L;
}
