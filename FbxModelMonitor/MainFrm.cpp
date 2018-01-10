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

// MainFrm.cpp : CMainFrame クラスの実装
//

#include "stdafx.h"
#include "FbxModelMonitor.h"
#include "FbxModelMonitorDoc.h"
#include "FbxModelMonitorView.h"

#include "MainFrm.h"
#include "MyUtil.h"
#include "MyStopWatch.hpp"
#include "FbxNodeTree.h"
#include "MyFbxVMeshBuilder.hpp"
#include "CustomTrace.hpp"
#include "MfcLocalizeHelper.hpp"
#include "AutoFileExtUpdateDlg.h"
#include "UserDefWinMsg.hpp"


#include "DebugNew.h"

#define ENABLES_TREE_UI

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx);

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_VIEW_CAPTION_BAR, &CMainFrame::OnViewCaptionBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTION_BAR, &CMainFrame::OnUpdateViewCaptionBar)
	ON_COMMAND(ID_TOOLS_OPTIONS, &CMainFrame::OnOptions)

	ON_COMMAND(ID_CHECK_CLASS_PANE, &CMainFrame::OnCheckClassPane)
	ON_UPDATE_COMMAND_UI(ID_CHECK_CLASS_PANE, &CMainFrame::OnUpdateCheckClassPane)
	ON_COMMAND(ID_CHECK_FILE_PANE, &CMainFrame::OnCheckFilePane)
	ON_UPDATE_COMMAND_UI(ID_CHECK_FILE_PANE, &CMainFrame::OnUpdateCheckFilePane)
	ON_COMMAND(ID_CHECK_OUTPUT_PANE, &CMainFrame::OnCheckOutputPane)
	ON_UPDATE_COMMAND_UI(ID_CHECK_OUTPUT_PANE, &CMainFrame::OnUpdateCheckOutputPane)
	ON_COMMAND(ID_CHECK_PROPERTIES_PANE, &CMainFrame::OnCheckPropertiesPane)
	ON_UPDATE_COMMAND_UI(ID_CHECK_PROPERTIES_PANE, &CMainFrame::OnUpdateCheckPropertiesPane)

	ON_COMMAND(ID_FILE_PRINT, &CMainFrame::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CMainFrame::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CMainFrame::OnFilePrintPreview)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, &CMainFrame::OnUpdateFilePrintPreview)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_MESSAGE(UDWM_INVOKE_SIMPLE_DELEGATE_BY_UI_THREAD, &CMainFrame::OnUdwmInvokeSimpleDelegateByUIThread)
	ON_COMMAND(ID_MY_BUTTON_ANIM_PLAY, &CMainFrame::OnMyButtonAnimPlay)
	ON_COMMAND(ID_MY_BUTTON_ANIM_STOP, &CMainFrame::OnMyButtonAnimStop)
	ON_COMMAND(ID_CHECK_SHOW_WAVE_FRONT, &CMainFrame::OnCheckShowWaveFront)
	ON_UPDATE_COMMAND_UI(ID_CHECK_SHOW_WAVE_FRONT, &CMainFrame::OnUpdateCheckShowWaveFront)
	ON_COMMAND(ID_CHECK_IMAGE_BASED_FUR, &CMainFrame::OnCheckImageBasedFur)
	ON_UPDATE_COMMAND_UI(ID_CHECK_IMAGE_BASED_FUR, &CMainFrame::OnUpdateCheckImageBasedFur)
	ON_COMMAND(ID_CHECK_BLOOM_EFFECT, &CMainFrame::OnCheckBloomEffect)
	ON_UPDATE_COMMAND_UI(ID_CHECK_BLOOM_EFFECT, &CMainFrame::OnUpdateCheckBloomEffect)
	ON_COMMAND(ID_CHECK_TOON_SHADING, &CMainFrame::OnCheckToonShading)
	ON_UPDATE_COMMAND_UI(ID_CHECK_TOON_SHADING, &CMainFrame::OnUpdateCheckToonShading)
	ON_COMMAND(ID_CHECK_TOON_INK, &CMainFrame::OnCheckToonInk)
	ON_UPDATE_COMMAND_UI(ID_CHECK_TOON_INK, &CMainFrame::OnUpdateCheckToonInk)
	ON_COMMAND(ID_CHECK_SHOW_COORD_AXES, &CMainFrame::OnCheckShowCoordAxes)
	ON_UPDATE_COMMAND_UI(ID_CHECK_SHOW_COORD_AXES, &CMainFrame::OnUpdateCheckShowCoordAxes)
	ON_COMMAND(ID_MY_BUTTON_BACK_COLOR, &CMainFrame::OnMyButtonBackColor)
	ON_COMMAND(ID_MY_BUTTON_FIRE_PROJECTILE, &CMainFrame::OnMyButtonFireProjectile)
	ON_COMMAND(ID_MY_BUTTON_LOAD_ENV_MAP_IMG, &CMainFrame::OnMyButtonLoadEnvMapImg)
	ON_COMMAND(ID_MY_COMBO_ANIM_TRACK, &CMainFrame::OnMyComboAnimTrack)
END_MESSAGE_MAP()

// CMainFrame コンストラクション/デストラクション

CMainFrame::CMainFrame()
{
	// TODO: メンバ初期化コードをここに追加してください。
	g_theApp.m_nAppLook = g_theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLACK);
}

CMainFrame::~CMainFrame()
{
}

bool CMainFrame::CreateFontTextureDibs()
{
	// HACK: GDI 相互運用をせず、最初から Direct2D & DirectWrite 向けに組むのであれば、LOGFONT を使う必要はない。
	const LOGFONTW logFont =
	{
		-32,
		0,
		0,
		0,
		//FW_NORMAL,
		FW_BOLD,
		false,
		false,
		false,
		ANSI_CHARSET,
		//OEM_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		//ANTIALIASED_QUALITY,
		CLEARTYPE_QUALITY,
#if 0
		DEFAULT_PITCH | FF_SWISS,
		L"Microsoft Suns Serif",
#elif 0
		FIXED_PITCH | FF_MODERN,
		L"Courier New",
#elif 0
		VARIABLE_PITCH | FF_ROMAN,
		L"Times New Roman",
#elif 0
		DEFAULT_PITCH | FF_DONTCARE,
		L"Verdana",
#elif 0
		DEFAULT_PITCH | FF_DONTCARE,
		L"メイリオ",
#elif 0
		DEFAULT_PITCH | FF_DONTCARE,
		L"Segoe UI",
#elif 0
		FIXED_PITCH | FF_DONTCARE,
		L"Segoe UI Mono",
#else
		FIXED_PITCH | FF_DONTCARE,
		L"Consolas",
		// Consolas は Windows 環境の標準モノスペース フォントの中では至高のフォント。
		// DirectWrite の IDWriteFactory::CreateTextFormat() に L"Consolas" を指定すると、
		// ギリシャ文字や特殊文字は「Segoe UI」に、日本語部分は「Meiryo UI」になる。
		// ちなみに Visual Studio 2010/2012/2013 でコード エディターなどのフォントに Consolas を使うと、Windows 7 でも 8 でも
		// 日本語文字は「メイリオ」に、ギリシャ文字や特殊文字は「Segoe UI」に、#region ラベルは「Meiryo UI」になる模様。
		// 「αγπτ」は、メイリオ（Verdana ベースらしい）だと判別しづらい。
		// Segoe UI の「……」「――」「→」や、Meiryo UI のカナ・かなは、全角幅ではなく日本語の組版を逸脱している（ノベル形式のテキスト・メッセージには向かない）。
		// 環境に依存しないでテキスト描画の品質を一定に保ちたかったら、やはり事前作成された画像ファイルを使うほかない（Unicode フル対応は厳しいが）。
		// XNA 用の WPF カスタム フォント プロセッサーが公開されているので、事前作成処理を高速化したい場合はこれを参考にするといいかも。
		// http://blogs.msdn.com/b/ito/archive/2012/02/19/wpf-font-processor.aspx
		// WPF のフォント描画品質にも納得がいかない場合、Adobe Illustrator や Photoshop を使って画像を作成していくしかない。
		// ちなみに DirectX TK 付属のフォント画像作成ツールは GDI+ ベースなので、OpenType 対応などに問題がある。
		// 画像生成の処理速度にも（アルゴリズム的に）問題がある。
		// http://zerogram.info/?p=1012
		// Direct2D & DirectWrite 描画であれば簡単に Unicode フル対応できるが、XAML と合わせてシステム系のメッセージ表示のみに限定したほうがいいかも。
		// DirectX 11.2 では 2D 描画・合成のパフォーマンス面での改善が図られているようだが……
#endif
	};

#if 0
	// フォントの生成。
	CFont font;
	if (!font.CreateFontIndirect(&logFont))
	{
		return false;
	}
#endif

	// デスクトップと互換性のあるデバイス コンテキスト取得。
	// デバイスにフォントを持たせないと GetGlyphOutline() 関数はエラーとなる。
	CDC dc;
	dc.CreateCompatibleDC(nullptr);
	//CFont* pOldFont = dc.SelectObject(&font);

	// システム DPI に合わせてテクスチャ フォントのデバイスピクセルサイズを決定する。
	m_hudFontTexData.FontHeight = ::MulDiv(abs(logFont.lfHeight), dc.GetDeviceCaps(LOGPIXELSY), 96);
	m_hudFontTexData.UsesMonospaceFont = true;
	m_hudFontTexData.TextureData.TextureWidth = 1024;
	m_hudFontTexData.TextureData.TextureHeight = 1024;
	m_hudFontTexData.TextureData.TextureColorDepthInBits = 8;
	m_hudFontTexData.TextureData.IsForAlpha = true;

	const bool result = MyDWriteWrapper::CreateFontAlphaTextureBufferFromDC(
		dc.GetSafeHdc(),
		//static_cast<HFONT>(font.GetSafeHandle()),
		logFont,
		//logFont.lfHeight,
		0,
		m_hudFontTexData.TextureData.TextureWidth,
		m_hudFontTexData.TextureData.TextureHeight,
		m_hudFontTexData.TextureData.TextureDib,
		m_hudFontTexData.CodeUVMap);

	//dc.SelectObject(pOldFont);

	return result;
}

//#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isfinite()

void CMainFrame::CreateToonShadingDiffuseCoefRefTextureDibs(const std::vector<MyMath::TMyGradientColorStopArray>& toonGradientColorStopsArray)
{
	const uint32_t texW = MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;
	const uint32_t texH = MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;

	m_toonShadingDiffuseCoefRefTexData.TextureWidth = texW;
	m_toonShadingDiffuseCoefRefTexData.TextureHeight = texH;
	m_toonShadingDiffuseCoefRefTexData.TextureColorDepthInBits = 32;
	m_toonShadingDiffuseCoefRefTexData.IsForAlpha = false;

	// デフォルトで白色。
	m_toonShadingDiffuseCoefRefTexData.TextureDib.resize(texW * texH * 4, 0xFF);
	MyMath::ColorRgba* textureBuf = MyUtil::StaticPointerCast<MyMath::ColorRgba>(&m_toonShadingDiffuseCoefRefTexData.TextureDib[0]);

	// HACK: UI でマテリアルの種類ごとに動的に制御・設定できるようにする。
	// マテリアルの種類の上限は高さピクセル数に左右される。256 あれば十分か？
	// メッシュ単体だけではなく、トゥーン シェーディング対象全体のマテリアル数上限なので注意。
	// グラデーションも自由にかけられるとよい。少なくとも1影、2影の指定はできたほうがよい。
	// Adobe Photoshop 風のグラデーション エディタを実装する？
	// グラデーション ストップを決めるオフセットの範囲は0～255ではなく、0～100％のほうがよさげ。
	// 適当なプリセット（肌向け、白い布向けなど）があるといいかも。
	// また、ソース DIB バッファは Direct3D/OpenGL で共用とする。

	for (uint32_t h = 0; h < texH && h < toonGradientColorStopsArray.size(); ++h)
	{
		// グラデーション オフセットをもとに昇順ソート済みであることが前提。
		const auto& toonGradientColorStops = toonGradientColorStopsArray[h];
		if (!toonGradientColorStops.empty())
		{
			const auto& firstColorStop = *toonGradientColorStops.begin();
			//const auto& lastColorStop = *(--toonGradientColorStops.end());
			auto prevColor = firstColorStop.Color;
			float prevOffset = 0;
			uint32_t w = 0;
			for (const auto& colorStop : toonGradientColorStops)
			{
				for (__noop; w < texW; ++w)
				{
					const float offset = float(w) / (texW - 1);
					if (colorStop.Offset < offset)
					{
						break;
					}
					if (colorStop.Offset == prevOffset)
					{
						break;
					}
					// 次の色と線形補間していく。
					const float s = float(offset - prevOffset) / (colorStop.Offset - prevOffset);
					_ASSERTE(_finite(s) && 0 <= s && s <= 1);
					textureBuf[texW * h + w] =
						MyMath::LerpColor(prevColor, colorStop.Color, s);
				}
				prevColor = colorStop.Color;
				prevOffset = colorStop.Offset;
			}
			// 残りを最後の色で埋めていく。
			for (__noop; w < texW; ++w)
			{
				textureBuf[texW * h + w] = prevColor;
			}
		}
	}
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	if (!this->CreateFontTextureDibs())
	{
		AfxMessageBox(_T("Failed to create font texture DIB!!"), MB_ICONERROR);
		return -1;
	}

	{
		std::vector<MyMath::TMyGradientColorStopArray> toonGradientColorStopsArray;
		// 階調ステップ化のテスト用。
		const MyMath::ColorRgba NormalShadowBase1(0x28, 0x28, 0x28);
		const MyMath::ColorRgba NormalShadowBase2(0x80, 0x80, 0x80);
		const MyMath::ColorRgba NormalShadowBase3(0xFF, 0xFF, 0xFF);
		MyMath::TMyGradientColorStopArray grad0, grad1;
		grad0.push_back(MyMath::MyGradientColorStop(0.20f, NormalShadowBase1));
		grad0.push_back(MyMath::MyGradientColorStop(0.20f, NormalShadowBase2));
		grad0.push_back(MyMath::MyGradientColorStop(0.30f, NormalShadowBase2));
		grad0.push_back(MyMath::MyGradientColorStop(0.30f, NormalShadowBase3));
		// 肌の影用。
#if 0
		// これだと髪からのドロップシャドウがほとんど見えなくなる。
		const MyMath::ColorRgba FleshShadowBase1(0xEC, 0xB9, 0xB9);
		const MyMath::ColorRgba FleshShadowBase2(0xFF, 0xFF, 0xFF);
		grad1.push_back(MyMath::MyGradientColorStop(0.05f, FleshShadowBase1));
		grad1.push_back(MyMath::MyGradientColorStop(0.20f, FleshShadowBase2));
#elif 0
		// テスト。
		const MyMath::ColorRgba FleshShadowBase1(0x91, 0x3e, 0x3e);
		const MyMath::ColorRgba FleshShadowBase2(0xFF, 0xFF, 0xFF);
#else
		const MyMath::ColorRgba FleshShadowBase1(0xEC, 0xB9, 0xB9);
		const MyMath::ColorRgba FleshShadowBase2(0xFF, 0xFF, 0xFF);
		grad1.push_back(MyMath::MyGradientColorStop(0.0f, FleshShadowBase1));
		grad1.push_back(MyMath::MyGradientColorStop(0.5f, FleshShadowBase2));
#endif
		toonGradientColorStopsArray.push_back(grad0);
		toonGradientColorStopsArray.resize(50, grad1);

		this->CreateToonShadingDiffuseCoefRefTextureDibs(toonGradientColorStopsArray);
	}

	CPathW pathModuleDir;
	ATLVERIFY(MyDesktopHelpers::GetModuleDirPath(pathModuleDir));

	// OnCreate() で作成して、OnDestroy() で破棄する。
	_ASSERTE(!m_pD3DManager);
	m_pD3DManager = new MyD3D::MyD3DManager();
	m_pD3DManager->SetLogDirPath(pathModuleDir.m_strPath.GetString());
	m_pD3DManager->SetMediaDirPath(pathModuleDir + L"\\D3DShaders");
	m_pD3DManager->SetHudFontTextureData(&m_hudFontTexData);
	m_pD3DManager->SetToonShadingDiffuseCoefRefTextureData(&m_toonShadingDiffuseCoefRefTexData);
	m_pD3DManager->SetModelMeshInfoArray(&m_modelMeshInfoArray);
	m_pD3DManager->SetAnimMixerArrayArray(&m_animMixerArrayArray);
	m_pD3DManager->SetGlobalMaterialTable(&m_globalMaterialTable);
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	_ASSERTE(!m_pOGLManager);
	m_pOGLManager = new MyOGL::MyOGLManager();
	m_pOGLManager->SetLogDirPath(pathModuleDir.m_strPath.GetString());
	m_pOGLManager->SetMediaDirPath(pathModuleDir + L"\\OGLShaders");
	m_pOGLManager->SetHudFontTextureData(&m_hudFontTexData);
	m_pOGLManager->SetToonShadingDiffuseCoefRefTextureData(&m_toonShadingDiffuseCoefRefTexData);
	m_pOGLManager->SetModelMeshInfoArray(&m_modelMeshInfoArray);
	m_pOGLManager->SetAnimMixerArrayArray(&m_animMixerArrayArray);
	m_pOGLManager->SetGlobalMaterialTable(&m_globalMaterialTable);
#endif

	BOOL bNameValid = false;
	// 固定値に基づいてビジュアル マネージャと visual スタイルを設定します
	OnApplicationLook(g_theApp.m_nAppLook);

	m_wndRibbonBar.Create(this);
#if 0
	// VS 2008 SP1 のコード ベース リボン。
	InitializeRibbon();
	m_wndRibbonBar.SaveToXMLFile(_T("RibbonOutput.xml"));
#else
	// VS 2010 の XML リソース ベース リボン。
	m_wndRibbonBar.LoadFromResource(IDR_RIBBON1);
#endif

	// 日本語の Meiryo UI フォントがインストールされていると思われる環境。
	// フォントの有無を普通にチェックしたほうがいいかも。
	// Windows 7 以降であれば、英語版でも下記の日本語対応フォントは必ずインストールされているらしい。
	// Meiryo; Meiryo UI; MS Gothic; MS Mincho; MS PGothic; MS PMincho; MS UI Gothic.
	if (Misc::LocalizeHelper::GetCurrentUserDefaultPrimaryLangID() == LANG_JAPANESE)
	{
		auto pOldRibbonFont = m_wndRibbonBar.GetFont();
		LOGFONT oldRibbonLogFont = {};
		pOldRibbonFont->GetLogFont(&oldRibbonLogFont);

		CDC dc;
		dc.CreateCompatibleDC(nullptr);
		const int logPixelsX = dc.GetDeviceCaps(LOGPIXELSX);
		const int logPixelsY = dc.GetDeviceCaps(LOGPIXELSY);
		// MFC 14.0 において、リボン バーの既定フォントは、Windows 7/10 では日英問わず Segoe UI がデフォルトらしい。
		// 96 DPI 環境だと height = -11。120 DPI 環境だと height = -14。
		// CreateFont() において、引数 nHeight は論理単位 (logical units) であり、デバイス単位 (device units) に変換されると書いてある。
		// https://msdn.microsoft.com/en-us/library/dd183499.aspx
		// CreateFontIndirect() は LOGFONT を受け取り、また LOGFONT::lfHeight の説明には logical units と書いてある。
		// また、LOGFONT::lfHeight は正数と負数の両方を許可するが、それぞれ意味が異なるらしい。
		// https://msdn.microsoft.com/en-us/library/dd145037.aspx
		// https://support.microsoft.com/en-us/help/32667/info-font-metrics-and-the-use-of-negative-lfheight
		// しかし、システムの DPI 値によって CFont::GetLogFont() の結果が変動し、
		// また高 DPI 環境において従来の 96 DPI を前提としたサイズを指定した場合はフォントの見た目が小さくなることから、
		// CFont::CreateFontIndirect() において、実際には LOGFONT::lfHeight はデバイス単位として解釈されるのではないか？
		// アプリケーションが DPI Unaware の場合、GetDeviceCaps(LOGPIXELSX), GetDeviceCaps(LOGPIXELSY) は常に 96 を返す。
		// MFC は VC2010 以降で System DPI Aware が既定値となっている。[構成プロパティ]→[マニフェスト ツール]→[入出力]→[DPI 認識]にて変更可能。
		// なお、CFont::CreatePointFontIndirect() を使う場合は、ポイント数の10倍の数と解釈される。
		ATLTRACE(_T("Old font = \"%s\", height = %ld, LogPixelsX = %d, LogPixelsY = %d\n"),
			oldRibbonLogFont.lfFaceName, oldRibbonLogFont.lfHeight, logPixelsX, logPixelsY);

		_tcscpy_s(oldRibbonLogFont.lfFaceName, _T("Meiryo UI"));
		//_tcscpy_s(oldRibbonLogFont.lfFaceName, _T("Segoe UI"));
		// Windows 7/10 デフォルト（Segoe UI + フォント リンクされたメイリオ）の場合は高さが -11 らしいが、Meiryo UI では小さすぎる。
		// MS Paint などと同じサイズにするには、-12 を指定する。
		oldRibbonLogFont.lfHeight = -::MulDiv(12, logPixelsY, 96);
		if (m_ribbonFont.CreateFontIndirect(&oldRibbonLogFont))
		{
			m_wndRibbonBar.SetFont(&m_ribbonFont);
		}
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("ステータス バーの作成に失敗しました。\n");
		return -1;      // 作成できない場合
	}

	CString strTitlePane1;
	CString strTitlePane2;
	bNameValid = strTitlePane1.LoadString(IDS_STATUS_PANE1);
	ASSERT(bNameValid);
	bNameValid = strTitlePane2.LoadString(IDS_STATUS_PANE2);
	ASSERT(bNameValid);
	m_wndStatusBar.AddElement(new CMFCRibbonStatusBarPane(ID_STATUSBAR_PANE1, strTitlePane1, TRUE), strTitlePane1);
	m_wndStatusBar.AddExtendedElement(new CMFCRibbonStatusBarPane(ID_STATUSBAR_PANE2, strTitlePane2, TRUE), strTitlePane2);

	// Visual Studio 2005 スタイルのドッキング ウィンドウ動作を有効にします
	CDockingManager::SetDockingMode(DT_SMART);
	// Visual Studio 2005 スタイルのドッキング ウィンドウの自動非表示動作を有効にします
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// キャプション バーを作成します:
	if (!CreateCaptionBar())
	{
		//TRACE0("キャプション バーを作成できませんでした\n");
		TRACE0("Failed to create caption bar\n");
		return -1;
	}

#if 0
	// ウィンドウ タイトル バーでドキュメント名とアプリケーション名の順序を切り替えます。これにより、
	// ドキュメント名をサムネイルで表示できるため、タスク バーの使用性が向上します。
	ModifyStyle(0, FWS_PREFIXTITLE);
#endif

	// メニュー項目イメージ (どの標準ツール バーにもないイメージ) を読み込みます:
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES, g_theApp.m_bHiColorIcons ? IDB_MENU_IMAGES_24 : 0);

	// ドッキング ウィンドウを作成します
	if (!CreateDockingWindows())
	{
		//TRACE0("ドッキング ウィンドウを作成できませんでした\n");
		TRACE0("Failed to create docking windows\n");
		return -1;
	}

	m_wndFileView.EnableDocking(CBRS_ALIGN_ANY);
	m_wndClassView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndFileView);
	CDockablePane* pTabbedBar = NULL;
	m_wndClassView.AttachToTabWnd(&m_wndFileView, DM_SHOW, TRUE, &pTabbedBar);
#if 1
	m_wndOutput.AttachToTabWnd(&m_wndFileView, DM_SHOW, TRUE, &pTabbedBar);
#else
	m_wndOutput.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndOutput);
#endif
	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndProperties);

	// FBX 管理オブジェクトの作成
	m_pFbxManager = MyFbx::TFbxSdkManagerPtr(FbxManager::Create(), MyFbx::Deleter<FbxManager>());

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
#if 0
	// 動的分割ウィンドウ
	return m_wndSplitter.Create(this,
		2, 2,               // TODO: 行と列の数を調整してください。
		CSize(10, 10),      // TODO: 最小ペインのサイズを変更します。
		pContext);
#else
#if 0
	m_isSplitterCreated = m_wndSplitter.Create(this, 2, 2, CSize(100, 100), pContext);
	if (m_isSplitterCreated)
	{
		//m_wndSplitter.SetColumnInfo(0, 100, 100);
		//m_wndSplitter.SetRowInfo(0, 100, 100);
		//m_wndSplitter.SetScrollStyle(0);
		//m_wndSplitter.RecalcLayout();
	}
#elif 0
	// 静的分割ウィンドウ
	m_isSplitterCreated = m_wndSplitter.CreateStatic(this, 2, 2);
	if (m_isSplitterCreated)
	{
		//CRect clrect;
		//m_wndSplitter.GetClientRect(&clrect); // --> この時点ではすべてゼロになってしまう。
		//this->GetClientRect(&clrect); // --> フレームのクライアント領域なので、分割ウィンドウのビューのみのサイズではない。
		//const int paneWidth = clrect.Width() / 2;
		//const int paneHeight = clrect.Height() / 2;
		const int paneWidth = 400;
		const int paneHeight = 300;

		// 左上のペインを作成。
		if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: left-top pane\n");
			return FALSE;
		}
		// 右上のペインを作成。
		if (!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: right-top pane\n");
			return FALSE;
		}
		// 左下のペインを作成。
		if (!m_wndSplitter.CreateView(1, 0, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: left-bottom pane\n");
			return FALSE;
		}
		// 右下のペインを作成。
		if (!m_wndSplitter.CreateView(1, 1, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: right-bottom pane\n");
			return FALSE;
		}
		// UNDONE: 3DCG ソフトのような三面図＋透視図の状態と透視図のみの状態を切り替えるため、実行時に分割／非分割を切り替えるようにする。
	}
#elif 1
	// OpenGL/Direct3D の描画を別々に行なうと、やはり負荷が高くなる。
	// 単一のビューに対してどちらかの API 単独で描画するようにして、実行時にオプションで切り替えることができるようにしておいたほうがよい。
	// その場合でも、抽象化されたレンダラーインターフェイスの作成と実装は行なっておいたほうがよい。
	// Valve は D3D と GL 両方でレンダリングしてシステマティックに比較するフレームワークを作成しているらしい。
	// http://www.4gamer.net/games/107/G010729/20130322107/
	m_isSplitterCreated = m_wndSplitter.CreateStatic(this, 2, 1);
	if (m_isSplitterCreated)
	{
		const int paneWidth = 0;
		const int paneHeight = 0;
		// UNDONE: 初期化時に幅・高さがそれぞれ非分割の場合のほぼ1/2になるように適切なサイズを決定する。
		// とりあえず InitInstance() にて、フレームウィンドウの初期化後、Direct3D/OpenGL の初期化前に調整している。
		// UNDONE: 拡大・縮小時には全体に対する各ペインの比率を保持するようにする（デフォルトでは拡大・縮小時に左上ペインのサイズが保持される）。

		// 上のペインを作成。
		if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: top pane\n");
			return FALSE;
		}
		// 下のペインを作成。
		if (!m_wndSplitter.CreateView(1, 0, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: bottom pane\n");
			return FALSE;
		}
	}
#else
	// ペイン1つだけの静的スプリットは作成できないらしい。
	m_isSplitterCreated = m_wndSplitter.CreateStatic(this, 1, 1);
	if (m_isSplitterCreated)
	{
		const int paneWidth = 0;
		const int paneHeight = 0;
		if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CFbxModelMonitorView), CSize(paneWidth, paneHeight), pContext))
		{
			TRACE("creation failure: top pane\n");
			return FALSE;
		}
	}
#endif

	return m_isSplitterCreated;
#endif
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWndEx::PreCreateWindow(cs))
	{
		return FALSE;
	}
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	return TRUE;
}


// 下記を参考にして、VS 2008 SP1 のコード ベース リボンから VS 2010 の XML リソース ベース リボンへ移行するとよい。
// http://msdn.microsoft.com/ja-jp/library/ee354411.aspx
// なお、この自動 XML 生成メソッドでは、完全に対処しきれないものもある。
// その場合、VS 2010 のリボン UI アプリ ウィザードが生成したコードと比較しながら、不足しているコードを随時追加する必要がある。

#if 0
void CMainFrame::InitializeRibbon()
{
	BOOL bNameValid;

	CString strTemp;
	bNameValid = strTemp.LoadString(IDS_RIBBON_FILE);
	ASSERT(bNameValid);

	// パネル イメージを読み込みます:
	m_PanelImages.SetImageSize(CSize(16, 16));
	m_PanelImages.Load(IDB_BUTTONS);

	// メイン ボタンを初期化します:
	m_MainButton.SetImage(IDB_MAIN);
	m_MainButton.SetText(_T("\nf"));
	m_MainButton.SetToolTipText(strTemp);

	m_wndRibbonBar.SetApplicationButton(&m_MainButton, CSize (45, 45));
	CMFCRibbonMainPanel* pMainPanel = m_wndRibbonBar.AddMainCategory(strTemp, IDB_FILESMALL, IDB_FILELARGE);

	bNameValid = strTemp.LoadString(IDS_RIBBON_NEW);
	ASSERT(bNameValid);
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_NEW, strTemp, 0, 0));
	bNameValid = strTemp.LoadString(IDS_RIBBON_OPEN);
	ASSERT(bNameValid);
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_OPEN, strTemp, 1, 1));
	bNameValid = strTemp.LoadString(IDS_RIBBON_SAVE);
	ASSERT(bNameValid);
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_SAVE, strTemp, 2, 2));
	bNameValid = strTemp.LoadString(IDS_RIBBON_SAVEAS);
	ASSERT(bNameValid);
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_SAVE_AS, strTemp, 3, 3));

	bNameValid = strTemp.LoadString(IDS_RIBBON_PRINT);
	ASSERT(bNameValid);
	CMFCRibbonButton* pBtnPrint = new CMFCRibbonButton(ID_FILE_PRINT, strTemp, 6, 6);
	pBtnPrint->SetKeys(_T("p"), _T("w"));
	bNameValid = strTemp.LoadString(IDS_RIBBON_PRINT_LABEL);
	ASSERT(bNameValid);
	pBtnPrint->AddSubItem(new CMFCRibbonLabel(strTemp));
	bNameValid = strTemp.LoadString(IDS_RIBBON_PRINT_QUICK);
	ASSERT(bNameValid);
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_DIRECT, strTemp, 7, 7, TRUE));
	bNameValid = strTemp.LoadString(IDS_RIBBON_PRINT_PREVIEW);
	ASSERT(bNameValid);
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_PREVIEW, strTemp, 8, 8, TRUE));
	bNameValid = strTemp.LoadString(IDS_RIBBON_PRINT_SETUP);
	ASSERT(bNameValid);
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_SETUP, strTemp, 11, 11, TRUE));
	pMainPanel->Add(pBtnPrint);
	pMainPanel->Add(new CMFCRibbonSeparator(TRUE));

	bNameValid = strTemp.LoadString(IDS_RIBBON_CLOSE);
	ASSERT(bNameValid);
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_CLOSE, strTemp, 9, 9));

	bNameValid = strTemp.LoadString(IDS_RIBBON_RECENT_DOCS);
	ASSERT(bNameValid);
	pMainPanel->AddRecentFilesList(strTemp);

	bNameValid = strTemp.LoadString(IDS_RIBBON_EXIT);
	ASSERT(bNameValid);
	pMainPanel->AddToBottom(new CMFCRibbonMainPanelButton(ID_APP_EXIT, strTemp, 15));

	// [クリップボード] パネルに [ホーム] カテゴリを追加します:
	// ==> これって「リボン バーに [ホーム] カテゴリを追加します」の間違いだろ？
	// ちなみに Windows Ribbon Framework や Ribbon for WPF では、カテゴリのことを「タブ（Tab）」、
	// パネルのことを「グループ（Group）」や「チャンク（Chunk）」と呼ぶ。
	bNameValid = strTemp.LoadString(IDS_RIBBON_HOME);
	ASSERT(bNameValid);
	CMFCRibbonCategory* pCategoryHome = m_wndRibbonBar.AddCategory(strTemp, IDB_WRITESMALL, IDB_WRITELARGE);

	// [クリップボード] パネルを作成します:
	bNameValid = strTemp.LoadString(IDS_RIBBON_CLIPBOARD);
	ASSERT(bNameValid);
	CMFCRibbonPanel* pPanelClipboard = pCategoryHome->AddPanel(strTemp, m_PanelImages.ExtractIcon(27));

	bNameValid = strTemp.LoadString(IDS_RIBBON_PASTE);
	ASSERT(bNameValid);
	CMFCRibbonButton* pBtnPaste = new CMFCRibbonButton(ID_EDIT_PASTE, strTemp, 0, 0);
	pPanelClipboard->Add(pBtnPaste);

	bNameValid = strTemp.LoadString(IDS_RIBBON_CUT);
	ASSERT(bNameValid);
	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_CUT, strTemp, 1));
	bNameValid = strTemp.LoadString(IDS_RIBBON_COPY);
	ASSERT(bNameValid);
	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_COPY, strTemp, 2));
	bNameValid = strTemp.LoadString(IDS_RIBBON_SELECTALL);
	ASSERT(bNameValid);
	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_SELECT_ALL, strTemp, -1));

	// [表示] パネルを作成して追加します:
	bNameValid = strTemp.LoadString(IDS_RIBBON_VIEW);
	ASSERT(bNameValid);
	CMFCRibbonPanel* pPanelView = pCategoryHome->AddPanel(strTemp, m_PanelImages.ExtractIcon(7));

	bNameValid = strTemp.LoadString(IDS_RIBBON_STATUSBAR);
	ASSERT(bNameValid);
	CMFCRibbonButton* pBtnStatusBar = new CMFCRibbonCheckBox(ID_VIEW_STATUS_BAR, strTemp);
	pPanelView->Add(pBtnStatusBar);
	bNameValid = strTemp.LoadString(IDS_RIBBON_CAPTIONBAR);
	ASSERT(bNameValid);
	CMFCRibbonButton* pBtnCaptionBar = new CMFCRibbonCheckBox(ID_VIEW_CAPTION_BAR, strTemp);
	pPanelView->Add(pBtnCaptionBar);

	// タブの右部に要素を追加します:
	bNameValid = strTemp.LoadString(IDS_RIBBON_STYLE);
	ASSERT(bNameValid);
	CMFCRibbonButton* pVisualStyleButton = new CMFCRibbonButton(-1, strTemp, -1, -1);

	pVisualStyleButton->SetMenu(IDR_THEME_MENU, FALSE /* 既定のコマンドがありません*/, TRUE /* 右揃え*/);

	bNameValid = strTemp.LoadString(IDS_RIBBON_STYLE_TIP);
	ASSERT(bNameValid);
	pVisualStyleButton->SetToolTipText(strTemp);
	bNameValid = strTemp.LoadString(IDS_RIBBON_STYLE_DESC);
	ASSERT(bNameValid);
	pVisualStyleButton->SetDescription(strTemp);
	m_wndRibbonBar.AddToTabs(pVisualStyleButton);

	// クイック アクセス ツール バー コマンドを追加します:
	CList<UINT, UINT> lstQATCmds;

	lstQATCmds.AddTail(ID_FILE_NEW);
	lstQATCmds.AddTail(ID_FILE_OPEN);
	lstQATCmds.AddTail(ID_FILE_SAVE);
	lstQATCmds.AddTail(ID_FILE_PRINT_DIRECT);

	m_wndRibbonBar.SetQuickAccessCommands(lstQATCmds);

	m_wndRibbonBar.AddToTabs(new CMFCRibbonButton(ID_APP_ABOUT, _T("\na"), m_PanelImages.ExtractIcon(0)));
}
#endif

BOOL CMainFrame::CreateDockingWindows()
{
	BOOL bNameValid;

	// クラス ビューを作成します
	CString strClassView;
	bNameValid = strClassView.LoadString(IDS_CLASS_VIEW);
	ASSERT(bNameValid);
	if (!m_wndClassView.Create(strClassView, this, CRect(0, 0, 200, 200),
		TRUE, ID_VIEW_CLASSVIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		//TRACE0("クラス ビュー ウィンドウを作成できませんでした\n");
		TRACE0("Failed to create Class View window\n");
		return FALSE;
	}

	// ファイル ビューを作成します
	CString strFileView;
	bNameValid = strFileView.LoadString(IDS_FILE_VIEW);
	ASSERT(bNameValid);
	if (!m_wndFileView.Create(strFileView, this, CRect(0, 0, 200, 200),
		TRUE, ID_VIEW_FILEVIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI))
	{
		//TRACE0("ファイル ビュー ウィンドウを作成できませんでした\n");
		TRACE0("Failed to create File View window\n");
		return FALSE;
	}

	// 出力ウィンドウを作成します
	CString strOutputWnd;
	bNameValid = strOutputWnd.LoadString(IDS_OUTPUT_WND);
	ASSERT(bNameValid);
	if (!m_wndOutput.Create(strOutputWnd, this, CRect(0, 0, 200, 200),
		TRUE, ID_VIEW_OUTPUTWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		TRACE0("出力ウィンドウを作成できませんでした\n");
		return FALSE;
	}

	// プロパティ ウィンドウを作成します
	CString strPropertiesWnd;
	bNameValid = strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
	ASSERT(bNameValid);
	if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 200, 200),
		TRUE, ID_VIEW_PROPERTIESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		//TRACE0("プロパティ ウィンドウを作成できませんでした\n");
		TRACE0("Failed to create Properties window\n");
		return FALSE;
	}

	SetDockingWindowIcons(g_theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
	HICON hFileViewIcon = static_cast<HICON>(::LoadImage(::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_FILE_VIEW_HC : IDI_FILE_VIEW),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0));
	m_wndFileView.SetIcon(hFileViewIcon, FALSE);

	HICON hClassViewIcon = static_cast<HICON>(::LoadImage(::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_CLASS_VIEW_HC : IDI_CLASS_VIEW),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0));
	m_wndClassView.SetIcon(hClassViewIcon, FALSE);

	HICON hOutputBarIcon = static_cast<HICON>(::LoadImage(::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0));
	m_wndOutput.SetIcon(hOutputBarIcon, FALSE);

	HICON hPropertiesBarIcon = static_cast<HICON>(::LoadImage(::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0));
	m_wndProperties.SetIcon(hPropertiesBarIcon, FALSE);

}

BOOL CMainFrame::CreateCaptionBar()
{
	if (!m_wndCaptionBar.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, this, ID_VIEW_CAPTION_BAR, -1, TRUE))
	{
		//TRACE0("キャプション バーを作成できませんでした\n");
		TRACE0("Failed to create caption bar\n");
		return FALSE;
	}

	BOOL bNameValid;

	CString strTemp, strTemp2;
	bNameValid = strTemp.LoadString(IDS_CAPTION_BUTTON);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetButton(strTemp, ID_TOOLS_OPTIONS, CMFCCaptionBar::ALIGN_LEFT, FALSE);
	bNameValid = strTemp.LoadString(IDS_CAPTION_BUTTON_TIP);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetButtonToolTip(strTemp);

	bNameValid = strTemp.LoadString(IDS_CAPTION_TEXT);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetText(strTemp, CMFCCaptionBar::ALIGN_LEFT);

	m_wndCaptionBar.SetBitmap(IDB_INFO, RGB(255, 255, 255), FALSE, CMFCCaptionBar::ALIGN_LEFT);
	bNameValid = strTemp.LoadString(IDS_CAPTION_IMAGE_TIP);
	ASSERT(bNameValid);
	bNameValid = strTemp2.LoadString(IDS_CAPTION_IMAGE_TEXT);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetImageToolTip(strTemp, strTemp2);

	return TRUE;
}

// CMainFrame 診断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif


// CMainFrame メッセージ ハンドラ

afx_msg LRESULT CMainFrame::OnUdwmInvokeSimpleDelegateByUIThread(WPARAM wParam, LPARAM lParam)
{
	return MyDesktopHelpers::MyWin32DelegateWrapper::OnInvoke(wParam, lParam);
}

void CMainFrame::OnApplicationLook(UINT id)
{
	// CMFCVisualManagerVS2008 と CMFCVisualManagerWindows7 は VS 2010 で追加された。

	CWaitCursor wait;

	g_theApp.m_nAppLook = id;

	switch (g_theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(TRUE);
		break;

	default:
		switch (g_theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
		m_wndRibbonBar.SetWindows7Look(FALSE);
		break;
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	g_theApp.WriteInt(_T("ApplicationLook"), g_theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(g_theApp.m_nAppLook == pCmdUI->m_nID);
}

void CMainFrame::OnViewCaptionBar()
{
	m_wndCaptionBar.ShowWindow(m_wndCaptionBar.IsVisible() ? SW_HIDE : SW_SHOW);
	RecalcLayout(FALSE);
}

void CMainFrame::OnUpdateViewCaptionBar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndCaptionBar.IsVisible());
}

void CMainFrame::OnOptions()
{
	CMFCRibbonCustomizeDialog *pOptionsDlg = new CMFCRibbonCustomizeDialog(this, &m_wndRibbonBar);
	ASSERT(pOptionsDlg != NULL);

	pOptionsDlg->DoModal();
	delete pOptionsDlg;
}

#pragma region // ペインの表示・非表示。//

void CMainFrame::OnCheckClassPane()
{
	m_wndClassView.ShowPane(!m_wndClassView.IsVisible(), false, false);
	//RecalcLayout(FALSE);
}

void CMainFrame::OnUpdateCheckClassPane(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndClassView.IsVisible());
}

void CMainFrame::OnCheckFilePane()
{
	m_wndFileView.ShowPane(!m_wndFileView.IsVisible(), false, false);
	//RecalcLayout(FALSE);
}

void CMainFrame::OnUpdateCheckFilePane(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndFileView.IsVisible());
}

void CMainFrame::OnCheckOutputPane()
{
	m_wndOutput.ShowPane(!m_wndOutput.IsVisible(), false, false);
	//RecalcLayout(FALSE);
}

void CMainFrame::OnUpdateCheckOutputPane(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndOutput.IsVisible());
}

void CMainFrame::OnCheckPropertiesPane()
{
	m_wndProperties.ShowPane(!m_wndProperties.IsVisible(), false, false);
	//RecalcLayout(FALSE);
}

void CMainFrame::OnUpdateCheckPropertiesPane(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndProperties.IsVisible());
}

#pragma endregion

void CMainFrame::OnFilePrint()
{
	if (IsPrintPreview())
	{
		PostMessage(WM_COMMAND, AFX_ID_PREVIEW_PRINT);
	}
}

void CMainFrame::OnFilePrintPreview()
{
	if (IsPrintPreview())
	{
		PostMessage(WM_COMMAND, AFX_ID_PREVIEW_CLOSE);  // 印刷プレビュー モードを強制的に終了する。
	}
}

void CMainFrame::OnUpdateFilePrintPreview(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(IsPrintPreview());
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWndEx::OnSize(nType, cx, cy);

	// TODO: ここにメッセージ ハンドラ コードを追加します。
	ATLTRACE(__FUNCTION__"(), cx = %d, cy = %d\n", cx, cy);
	if (m_isSplitterCreated)
	{
		CRect clrect;
		m_wndSplitter.GetClientRect(&clrect);
		ATLTRACE(__FUNCTION__"(), SplitterWnd.ClientRect = (%d, %d, %d, %d)\n", clrect.left, clrect.top, clrect.right, clrect.bottom);

		// UNDONE: D3D デバイスのリセット（バック バッファを含むすべてのウィンドウ同サイズ レンダーターゲットのリサイズ？）を行なう。

		// HACK: Visual Studio 2012 でデバッグ実行中、ブレークポイントで停止しているときに、Windows 7 の [デスクトップの表示] ボタンを連打していると、
		// なぜか MFC アプリケーションのウィンドウ サイズがゼロになる現象が発生した。
		// この現象が発生するとメイン ウィンドウがタスク バーから復帰しなくなる。
		// レジストリ エディターで
		// "HKEY_CURRENT_USER\Software\<アプリケーション ウィザードで生成されたローカル アプリケーション>\<AppName>\Workspace"
		// 配下を削除すればリセットできるが……
	}
}

bool CMainFrame::GetSplitterWndClientSize(CSize& size) const
{
	if (m_isSplitterCreated)
	{
		CRect clrect;
		m_wndSplitter.GetClientRect(&clrect);
		size.cx = clrect.right - clrect.left;
		size.cy = clrect.bottom - clrect.top;
		return true;
	}
	else
	{
		size.cx = 0;
		size.cy = 0;
		return false;
	}
}

bool CMainFrame::UpdateMySplitterWndRowColumnSize(int row, int col, int cxIdeal, int cyIdeal, int cxMin, int cyMin)
{
	if (m_isSplitterCreated)
	{
		m_wndSplitter.SetRowInfo(row, cyIdeal, cyMin);
		m_wndSplitter.SetColumnInfo(col, cxIdeal, cxMin);
		return true;
	}
	else
	{
		return false;
	}
}

bool CMainFrame::RecalcSplitterWndLayout()
{
	if (m_isSplitterCreated)
	{
		m_wndSplitter.RecalcLayout();
		return true;
	}
	else
	{
		return false;
	}
}

//! @brief  Direct3D と OpenGL の初期化。<br>
bool CMainFrame::InitDirect3DAndOpenGL()
{
	if (!m_isSplitterCreated)
	{
		return false;
	}

	// CSplitterWnd::GetPane() により、CreateView() で割り当てたビューを取得できる。
	{
		m_pWndHostD3D = m_wndSplitter.GetPane(0, 0); // 0行0列。
		const CSize clsize = this->GetSafeDirect3DHostWndClientRectSize();
#if 0
		if (clrect.Width() <= 0 || clrect.Height() <= 0)
		{
			ATLTRACE("Client size of target window is invalid!!\n");
			// HACK: Direct3D にはサイズ1で渡して、作成は続行してしまったほうが良いかも。
			return false;
		}
#endif
		_ASSERTE(m_pD3DManager != nullptr);
		if (!m_pD3DManager->Create(clsize.cx, clsize.cy, m_pWndHostD3D->GetSafeHwnd()))
		{
			return false;
		}
	}

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	{
		m_pWndHostOGL = m_wndSplitter.GetPane(1, 0); // 1行0列。
		const CSize clsize = this->GetSafeOpenGLHostWndClientRectSize();
#if 0
		if (clrect.Width() <= 0 || clrect.Height() <= 0)
		{
			ATLTRACE("Client size of target window is invalid!!\n");
			// HACK: OpenGL にはサイズ1で渡して、作成は続行してしまったほうが良いかも。
			return false;
		}
#endif
		_ASSERTE(m_pOGLManager != nullptr);
		if (!m_pOGLManager->Create(clsize.cx, clsize.cy, m_pWndHostOGL->GetSafeHwnd()))
		{
			return false;
		}
	}
#endif

	this->SetDisplaysCoordAxes(true);
	//this->SetBackColor(MyMath::Vector4F(0.0f, 0.5f, 0.5f, 1.0f));
	this->SetBackColor(MyMath::Vector4F(32 / 255.0f, 65 / 255.0f, 90 / 255.0f, 1.0f));

	if (!m_resizeCheckTimerID)
	{
		m_resizeCheckTimerID = this->SetTimer(ResizeCheckTimerID, ResizeCheckPeriodMs, nullptr);
		ATLASSERT(m_resizeCheckTimerID != 0);
	}

	// 一度レンダリングする。
	//return this->RenderMyScene();
	return true;
}

bool CMainFrame::RenderMyScene(bool advancesFrame, CWnd* pHostWnd)
{
	// nullptr が指定されたら両方とも描画する。
	// TODO: 他のメソッドも、ビューを操作したときにそのビューが担当する Manager のみを変更するか、それとも同期させるか否かを決めるオプションを作る。
	// Manager クラスは共通のインターフェイスを継承して、ビューごとにそれらを関連付け、仮想メソッド経由でビューごとに操作させるようにしたほうが良いかも。
	// なお、Direct3D と OpenGL は API レベルでの思想の違いがかなり大きいので、抽象化の粒度はあまり小さくしないほうがいい。

	// GPU リソースに時間（時刻）依存データが書き込まれ、GPU によって更新されるものがあるので、描画時にフレーム時刻を進めるか否かを表すフラグを渡す必要がある。

	if (pHostWnd == nullptr || pHostWnd == m_pWndHostD3D)
	{
		if (!m_pD3DManager || !m_pD3DManager->Render(advancesFrame, !m_isLoadingResourcesNow))
		{
			return false;
		}
	}

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	if (pHostWnd == nullptr || pHostWnd == m_pWndHostOGL)
	{
		if (!m_pOGLManager || !m_pOGLManager->Render(advancesFrame))
		{
			return false;
		}
	}
#endif

	if (advancesFrame && !m_isLoadingResourcesNow)
	{
		// ミキサーのボーン アニメーション フレームを進める。
		// ゲームであれば、ゲーム オブジェクトの数だけアニメーション コントローラーが存在し、
		// そのフレーム時間をそれぞれ進める（もしくは停止・巻き戻しする）ことになる。
		// こちらはデバイス非依存なので、オフスクリーンのみで制御可能。
		MyCommon::AdvanceAllAnimMixerFrameCounters(m_animTrackInfoTable, m_modelMeshInfoArray, m_animMixerArrayArray, m_animEndCallbackFuncsArray);
		if (m_isCurrentAnimEnded)
		{
			// すべてをリセットし、ループ再生する。
			// 各ボーンを自走させたままだと、ボーンごとに周期が違う場合、ルートボーンがループするたびにずれが生じていく。
			const int selectedAnimIndex = this->GetRibbonComboBoxSelectedIndex(ID_MY_COMBO_ANIM_TRACK);
			MyCommon::SetSingleAnim(m_animMixerArrayArray, selectedAnimIndex);
			m_isCurrentAnimEnded = false;
		}
	}

	return true;
}

bool CMainFrame::ResizeMyView(CWnd* pHostWnd)
{
	if (pHostWnd == nullptr || pHostWnd == m_pWndHostD3D)
	{
		const CSize clsize = this->GetSafeDirect3DHostWndClientRectSize();
		if (m_pD3DManager && m_pD3DManager->SafeResizeScreen(clsize.cx, clsize.cy))
		{
			m_pD3DManager->Render(false, !m_isLoadingResourcesNow);
		}
		else
		{
			return false;
		}
	}

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	if (pHostWnd == nullptr || pHostWnd == m_pWndHostOGL)
	{
		const CSize clsize = this->GetSafeOpenGLHostWndClientRectSize();
		if (m_pOGLManager && m_pOGLManager->SafeResizeScreen(clsize.cx, clsize.cy))
		{
			m_pOGLManager->Render(false);
		}
		else
		{
			return false;
		}
	}
#endif

	return true;
}

void CMainFrame::OnDestroy()
{
	// ウィンドウ ハンドルが無効になる前に、先に D3D / OGL をリリースしておく。
	// メインフレームではなく、直接のホストとなる各ビューにインターフェイス経由でコンポジションして関連付けておいたほうが良いかも。

	if (m_renderingTimerID)
	{
		ATLVERIFY(this->KillTimer(m_renderingTimerID));
		m_renderingTimerID = 0;
	}
	if (m_resizeCheckTimerID)
	{
		ATLVERIFY(this->KillTimer(m_resizeCheckTimerID));
		m_resizeCheckTimerID = 0;
	}

	MyUtil::SafeDelete(m_pD3DManager);
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	MyUtil::SafeDelete(m_pOGLManager);
#endif

	__super::OnDestroy();

	// TODO: ここにメッセージ ハンドラ コードを追加します。
}

void CMainFrame::OnClose()
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	CMyGradientStopsProperty::DestroyWpfGradientEditor();

	__super::OnClose();
}

MyFbx::MyFbxNodeAnalyzerBase::TSharedPtr CMainFrame::LoadFbxFile(CStringW strFilePath)
{
	MyUtil::HRStopwatch stopwatch;
	stopwatch.Start();

	// NOTE: ATL::CComPtr は内包するポインタ型への暗黙のキャストがオーバーロードされているが、std::shared_ptr はそうでないことに注意。
	// ただし operator bool() は実装されているので、内包するポインタの有効・無効を判定する論理式として使うことはできる。
	// Microsoft::WRL::ComPtr も std::shared_ptr と同様。

	if (m_pFbxManager.get() == nullptr) // 「if (!m_pFbxManager)」でも OK。
	{
		return MyFbx::MyFbxNodeAnalyzerBase::TSharedPtr();
	}

	// ファイル I/O は当然時間がかかる処理。サブスレッドに逃がすべき。
	// HACK: もし複数スレッドで複数のファイルを並列読み込みするとき、FbxManager オブジェクトはスレッドごとに複数必要になる？
	MyFbx::TFbxSdkScenePtr pScene;
	if (!MyFbx::CreateSceneFromFbx(strFilePath, m_pFbxManager.get(), pScene))
	{
		return MyFbx::MyFbxNodeAnalyzerBase::TSharedPtr();
	}

	stopwatch.Stop();
	ATLTRACE(__FUNCTION__"(): Elapsed time for '%s' = %I64d[ms]\n", "CreateSceneFromFbx", stopwatch.GetElapsedTimeInMilliseconds());

	stopwatch.Restart();

	// 前準備。
#ifdef ENABLES_TREE_UI
	auto& treeCtrl = m_wndFileView.GetTreeCtrl();
	auto& editCtrl = m_wndOutput.GetEditCtrl();
	// GUI へのアクセスがパフォーマンスを低下させているのかと思ったが、そうではないらしい。
	// 解析・データ変換するだけでかなり時間を消費している。
	// HACK: サブスレッドで解析しながら GUI を操作することになる。
	// MFC のクラスメソッドのほとんどはサブスレッドから直接呼び出しても実害はないが、
	// 内部的には結局 SendMessage が呼ばれるので、メインスレッドとサブスレッドを行ったり来たりすることになる。
	// 解析処理と GUI 操作は完全にステップを分離するべき。
	auto nodeAnalyzer = std::make_shared<MyMfcFbx::MyFbxNodeTree>(&treeCtrl, &editCtrl);
#else
	auto nodeAnalyzer = std::make_shared<MyFbx::MyFbxNodeAnalyzerBase>();
#endif
	const bool isSucceeded = nodeAnalyzer->AnalyzeScene(pScene.get());

	stopwatch.Stop();
	ATLTRACE(__FUNCTION__"(): Elapsed time for '%s' = %I64d[ms]\n", "AnalyzeScene", stopwatch.GetElapsedTimeInMilliseconds());

	stopwatch.Restart();

	nodeAnalyzer->ConvertAllMeshFacesAsTrianglesOnly();

	stopwatch.Stop();
	ATLTRACE(__FUNCTION__"(): Elapsed time for '%s' = %I64d[ms]\n", "ConvertAllMeshFacesAsTrianglesOnly", stopwatch.GetElapsedTimeInMilliseconds());

	return nodeAnalyzer;
}

bool CMainFrame::CreateDeviceMeshFromFbx(CStringW strFilePath, const MyFbx::MyFbxNodeAnalyzerBase& nodeAnalyzer)
{
	// TODO: OpenGL および D3D10/D3D11 のアニメーション モデル（メッシュ階層）を作成する。

	CPathW texDirPath(strFilePath);
	texDirPath.RemoveFileSpec(); // FBX ファイルと同階層にすべての相対テクスチャ ファイルがあるものとする。

	// ポリゴン数、マテリアルやテクスチャが多い場合はデバイス メッシュの構築にも時間がかかる。
	MyUtil::HRStopwatch stopwatch;
	stopwatch.Start();

	MyMath::TMyNameToMaterialTable nameToMaterialTable;
	const HRESULT hr = MyFbxViewer::CreateMySkinMeshFromFbx(nodeAnalyzer,
		texDirPath.m_strPath,
		m_pD3DManager->GetD3DDevice(),
		m_pD3DManager->GetMainMeshArray(),
		m_pD3DManager->GetMainTexTable(),
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT 
		m_pOGLManager->GetMainMeshArray(),
		m_pOGLManager->GetMainTexTable(),
#endif
		nameToMaterialTable,
		m_modelMeshInfoArray,
		m_animTrackInfoTable
		);

	stopwatch.Stop();
	ATLTRACE(__FUNCTION__"(): Elapsed time for '%s' = %I64d[ms]\n", "CreateMySkinMeshFromFbx", stopwatch.GetElapsedTimeInMilliseconds());

	ATLTRACE(__FUNCTION__"(), hr = 0x%08lx\n", hr);

	if (FAILED(hr))
	{
		// リソース不足などで失敗することはありえる。
		return false;
	}

	// [メッシュ数]×[スケルトン数] だけミキサーを確保する。
	_ASSERTE(m_animMixerArrayArray.empty());
	for (const auto& modelMeshInfo : m_modelMeshInfoArray)
	{
		m_animMixerArrayArray.push_back(MyCommon::CreateAnimMixersArray(modelMeshInfo->GetBoneSkeletonInfoArray().size()));

#if 0
		// 以下、確認。
		// 通例、アニメーション全体の Duration はルートボーンの周期になる模様。
		// HACK: 必ずしもそうなるとはいえないのかもしれない。要確認。
		const int rootBoneIndex = modelMeshInfo->GetRootBoneIndex();
		if (rootBoneIndex != MyMath::BoneSkeletonInfo::InvalidBoneIndex)
		{
			for (const auto& boneAnimInfoArray : modelMeshInfo->GetBoneAnimInfoArrayArray())
			{
				const size_t rootBoneAnimFrameCount = (*boneAnimInfoArray)[rootBoneIndex]->GetGlobalFrameAttitudeMatrices().size();
				ATLTRACE("RootBoneAnimFrameCount = %Iu\n", rootBoneAnimFrameCount);
			}
		}
#endif
	}

	// 各アニメーション終了時のコールバック処理を登録する。
	// std::function によるコールバックは、仮想関数によるコールバックに近い。
	// C 関数の関数ポインタ登録のようなパラメータまわりのわずらわしさがないのがメリットだが、そのぶん実行時にコストを払う必要がある（Type Erasure）。
	// なお、this をラムダでキャプチャする場合は寿命管理に注意。
	m_animEndCallbackFuncsArray.resize(m_animTrackInfoTable.GetAnimCount());
	//for (size_t a = 0; a < m_animTrackInfoTable.GetAnimCount(); ++a)
	for (auto& func : m_animEndCallbackFuncsArray)
	{
		//m_animEndCallbackFuncsArray[a] = [this]() { m_isCurrentAnimEnded = true; };
		func = [this]() { m_isCurrentAnimEnded = true; };
	}

	this->CreateGlobalMaterialsArray(nameToMaterialTable);
	MyAppHelpers::InvokeWith(this->GetSafeHwnd(), [this]()
	{
		this->UpdateMaterialViews();
		this->CreateAnimTrackComboBoxItems();
		return 0;
	});

#ifdef ENABLES_TREE_UI
	if (!::IsWindow(m_wndFileView.GetSafeHwnd()) || !::IsWindow(m_wndOutput.GetSafeHwnd()))
	{
		return false;
	}

	auto& treeCtrl = m_wndFileView.GetTreeCtrl();
	auto& editCtrl = m_wndOutput.GetEditCtrl();

	MyDesktopHelpers::AddStringLineToEditCtrlAndScrollToLastLine(editCtrl, strFilePath);

	// 再帰的に探索して全て展開する。
	// HACK: 「全て展開」および「全て折りたたむ」のツールボタンを作るとよい。
	MyDesktopHelpers::ExpandAll(treeCtrl);

	CString strMsg;
	strMsg.Format(_T("TotalMaterialCount = %Iu"), nameToMaterialTable.size());
	MyDesktopHelpers::AddStringLineToEditCtrlAndScrollToLastLine(editCtrl, strMsg);
#endif
#if 1
	// テスト用にテクスチャを DDS ファイルとしてダンプする。
	// D3DX もしくは DirectXTex を使うと簡単。
	const auto& texTable = m_pD3DManager->GetMainTexTable();
	for (const auto& src : texTable)
	{
		CPathW ddsFilePath(src.first.c_str());
		ddsFilePath.RenameExtension(L".dds");
		m_pD3DManager->DumpTextureToFile(src.second.Texture2D.Get(), ddsFilePath);
	}
#endif
	return true;
}

void CMainFrame::CreateAnimTrackComboBoxItems()
{
	const auto& animTrackNamesArray = m_animTrackInfoTable.GetAnimTrackNamesArray();

	auto pAnimTrackCombo = this->GetRibbonComboBox(ID_MY_COMBO_ANIM_TRACK);
	_ASSERTE(pAnimTrackCombo);
	_ASSERTE(pAnimTrackCombo->GetCount() == 0);
	pAnimTrackCombo->RemoveAllItems();
	for (const auto& strAnimName : animTrackNamesArray)
	{
		pAnimTrackCombo->AddItem(strAnimName.c_str());
	}
	// アニメーションがあれば最初のトラックを選択する。
	if (!animTrackNamesArray.empty())
	{
		pAnimTrackCombo->SelectItem(0);
	}
}

void CMainFrame::ClearAnimTrackComboBoxItems()
{
	auto pAnimTrackCombo = this->GetRibbonComboBox(ID_MY_COMBO_ANIM_TRACK);
	if (pAnimTrackCombo)
	{
		pAnimTrackCombo->RemoveAllItems();
	}
}

void CMainFrame::ClearMeshTree()
{
#ifdef ENABLES_TREE_UI
	if (::IsWindow(m_wndFileView.GetSafeHwnd()))
	{
		auto& treeCtrl = m_wndFileView.GetTreeCtrl();
		treeCtrl.DeleteAllItems();
	}
	if (::IsWindow(m_wndOutput.GetSafeHwnd()))
	{
		auto& editCtrl = m_wndOutput.GetEditCtrl();
		//editCtrl.Clear();
		editCtrl.SetWindowText(_T(""));
	}
#endif

	// D3D / OGL 専用データ、および共通データをすべて破棄。
	// 本来は Doc-View が担う役目だが、SDI なのでフレーム側でできないこともない。
	if (m_pD3DManager)
	{
		m_pD3DManager->ClearMainTexTable();
		m_pD3DManager->ClearMainMeshArray();
	}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	if (m_pOGLManager)
	{
		m_pOGLManager->ClearMainTexTable();
		m_pOGLManager->ClearMainMeshArray();
	}
#endif
	this->ClearGlobalMaterialsArray();
	MyAppHelpers::InvokeWith(this->GetSafeHwnd(), [this]()
	{
		this->UpdateMaterialViews();
		this->ClearAnimTrackComboBoxItems();
		return 0;
	});

	m_modelMeshInfoArray.clear();
	m_animTrackInfoTable.Clear();
	m_animEndCallbackFuncsArray.clear();
	m_animMixerArrayArray.clear();
}

void CMainFrame::OnMyButtonAnimPlay()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	// TODO: ボタンのアイコンを変更する。
	if (!m_renderingTimerID)
	{
		m_renderingTimerID = this->SetTimer(RenderingTimerID, RenderingPeriodPerFrameMs, nullptr);
		ATLASSERT(m_renderingTimerID != 0);
	}
	else
	{
		// TODO: すでに再生している場合は一時停止する。
	}
}


void CMainFrame::OnMyButtonAnimStop()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	if (m_renderingTimerID)
	{
		ATLVERIFY(this->KillTimer(m_renderingTimerID));
		m_renderingTimerID = 0;
		// TODO: これはただの一時停止（Pause）。本来 Stop は再生位置をリセットする。
	}
}


void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	if (nIDEvent == m_renderingTimerID)
	{
		this->RenderMyScene(true);
	}
	else if (nIDEvent == m_resizeCheckTimerID)
	{
		// WM_SIZE はリサイズ中に何度も呼ばれることがあるし、最大化・最小化の扱いも面倒なので、あえてタイマーを使ってサイズ変更を検出する。
		this->ResizeMyView(nullptr);
	}

	__super::OnTimer(nIDEvent);
}


void CMainFrame::OnCheckShowWaveFront()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	const auto* pCheckBox = this->GetRibbonCheckBox(ID_CHECK_SHOW_WAVE_FRONT);

	ATLASSERT(pCheckBox);
	ATLTRACE(__FUNCTION__"(), IsChecked = %d\n", pCheckBox->IsChecked());

	if (m_pD3DManager)
	{
		m_pD3DManager->GetEffectSettings().DisplaysWaveFront = !pCheckBox->IsChecked();
		this->RenderMyScene(false);
	}
}


void CMainFrame::OnUpdateCheckShowWaveFront(CCmdUI *pCmdUI)
{
	// TODO: ここにコマンド更新 UI ハンドラー コードを追加します。

	if (m_pD3DManager)
	{
		pCmdUI->SetCheck(m_pD3DManager->GetEffectSettings().DisplaysWaveFront);
	}
}


void CMainFrame::OnCheckImageBasedFur()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	const auto* pCheckBox = this->GetRibbonCheckBox(ID_CHECK_IMAGE_BASED_FUR);

	ATLASSERT(pCheckBox);
	ATLTRACE(__FUNCTION__"(), IsChecked = %d\n", pCheckBox->IsChecked());

	if (m_pD3DManager)
	{
		m_pD3DManager->GetEffectSettings().EnablesImageBasedFur = !pCheckBox->IsChecked();
		this->RenderMyScene(false);
	}
}


void CMainFrame::OnUpdateCheckImageBasedFur(CCmdUI *pCmdUI)
{
	// TODO: ここにコマンド更新 UI ハンドラー コードを追加します。

	if (m_pD3DManager)
	{
		pCmdUI->SetCheck(m_pD3DManager->GetEffectSettings().EnablesImageBasedFur);
	}
}

void CMainFrame::OnCheckBloomEffect()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	const auto* pCheckBox = this->GetRibbonCheckBox(ID_CHECK_BLOOM_EFFECT);

	ATLASSERT(pCheckBox);
	ATLTRACE(__FUNCTION__"(), IsChecked = %d\n", pCheckBox->IsChecked());

	if (m_pD3DManager)
	{
		m_pD3DManager->GetEffectSettings().EnablesBloomEffect = !pCheckBox->IsChecked();
		this->RenderMyScene(false);
	}
}


void CMainFrame::OnUpdateCheckBloomEffect(CCmdUI *pCmdUI)
{
	// TODO: ここにコマンド更新 UI ハンドラー コードを追加します。

	if (m_pD3DManager)
	{
		pCmdUI->SetCheck(m_pD3DManager->GetEffectSettings().EnablesBloomEffect);
	}
}

void CMainFrame::OnCheckToonShading()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	const auto* pCheckBox = this->GetRibbonCheckBox(ID_CHECK_TOON_SHADING);

	ATLASSERT(pCheckBox);
	ATLTRACE(__FUNCTION__"(), IsChecked = %d\n", pCheckBox->IsChecked());

	if (m_pD3DManager)
	{
		m_pD3DManager->GetEffectSettings().EnablesToonShading = !pCheckBox->IsChecked();
		this->RenderMyScene(false);
	}
}


void CMainFrame::OnUpdateCheckToonShading(CCmdUI *pCmdUI)
{
	// TODO: ここにコマンド更新 UI ハンドラー コードを追加します。

	if (m_pD3DManager)
	{
		pCmdUI->SetCheck(m_pD3DManager->GetEffectSettings().EnablesToonShading);
	}
}


void CMainFrame::OnCheckToonInk()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	const auto* pCheckBox = this->GetRibbonCheckBox(ID_CHECK_TOON_INK);
	ATLASSERT(pCheckBox);
	ATLTRACE(__FUNCTION__"(), IsChecked = %d\n", pCheckBox->IsChecked());

	if (m_pD3DManager)
	{
		m_pD3DManager->GetEffectSettings().EnablesToonInk = !pCheckBox->IsChecked();
		this->RenderMyScene(false);
	}
}


void CMainFrame::OnUpdateCheckToonInk(CCmdUI *pCmdUI)
{
	// TODO: ここにコマンド更新 UI ハンドラー コードを追加します。

	if (m_pD3DManager)
	{
		pCmdUI->SetCheck(m_pD3DManager->GetEffectSettings().EnablesToonInk);
	}
}


void CMainFrame::OnCheckShowCoordAxes()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	const auto* pCheckBox = this->GetRibbonCheckBox(ID_CHECK_SHOW_COORD_AXES);

	ATLASSERT(pCheckBox);
	ATLTRACE(__FUNCTION__"(), IsChecked = %d\n", pCheckBox->IsChecked());

	{
		this->SetDisplaysCoordAxes(!pCheckBox->IsChecked());
		this->RenderMyScene(false);
	}
}


void CMainFrame::OnUpdateCheckShowCoordAxes(CCmdUI *pCmdUI)
{
	// TODO: ここにコマンド更新 UI ハンドラー コードを追加します。

	pCmdUI->SetCheck(this->GetDisplaysCoordAxes());
}


void CMainFrame::OnMyButtonBackColor()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	// オブジェクトに関連付けられている色が変更されたタイミングで、このメッセージ ハンドラーが呼ばれる。

	const auto* pButton = this->GetRibbonColorButton(ID_MY_BUTTON_BACK_COLOR);
	ATLASSERT(pButton);
	const COLORREF colorVal = pButton->GetColor();
	ATLTRACE(__FUNCTION__"(), Color = 0x%08X\n", colorVal);
	// 32bit RGBX の順で並んでいるので、Vector4F に変換する。リトルエンディアンだと文字列化したとき XBGR に見える。
	this->SetBackColor(MyMath::ConvertRGBXToColor4F(colorVal));
	this->RenderMyScene(false);
}


void CMainFrame::OnMyButtonFireProjectile()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	if (m_pD3DManager)
	{
		m_pD3DManager->FireProjectile();
	}

#if 0
	auto hMainWnd = this->GetSafeHwnd();
	ATLTRACE(_T("ThreadID = %lu\n"), ::GetCurrentThreadId());
	CString strOldWinText;
	this->GetWindowText(strOldWinText);
	this->SetWindowText(strOldWinText + _T(" (Now firing...)"));
	auto myTask = concurrency::create_task([=]()
	{
		ATLTRACE(_T("ThreadID = %lu\n"), ::GetCurrentThreadId());
		::Sleep(2 * 1000);
		return strOldWinText;
	});
	myTask.then([=](decltype(myTask) result)
	{
		ATLTRACE(_T("ThreadID = %lu\n"), ::GetCurrentThreadId());
		MyAppHelpers::InvokeWith(hMainWnd,
			[=]()
			{
				ATLTRACE(_T("ThreadID = %lu\n"), ::GetCurrentThreadId());
				this->SetWindowText(result.get());
				return 0;
			});
	});
	// create_task で作成したタスクが IAsyncInfo をラップしている場合、後続タスクは既定では継続チェーンを作成したコンテキストで実行される。
	// また、concurrency::task_continuation_context::use_current() が使えるのは C++/CX (WinRT) のみらしい。
	// http://msdn.microsoft.com/ja-jp/library/windows/apps/jj160321.aspx
	// WinRT アプリでは定型文を使うことでメインスレッドによる継続を簡単に実装できるが、MFC では小細工が必要となる。
	// CWnd などの MFC コントロール クラスは、ほとんどが Win32 API のラッパーで、メソッドも HWND 経由でメッセージを同期送信しているだけのものが多いため、
	// サブスレッドからクラス インスタンス経由で直接メソッドを呼び出しても特に問題が発生しないこともあるが、動作保証外になる。
	// WPF の UI 要素を C++/CLI 経由で操作するときなども不便（サブスレッドから直接操作できない）。
	// コントロールを作成したメインスレッドからのみ呼び出すようにするか、サブスレッドではウィンドウ ハンドルと Win32 API を直接使うようにするか、
	// サブスレッドで CWnd::FromHandle() を使って一時的なクラス インスタンス（シャドウ）を生成するなどの配慮が必要。
	// また、複数のスレッドから同時にアクセスする場合には当然競合が発生するので、必要に応じて排他処理をする必要もある。
	// サブスレッドからウィンドウを操作する場合は、そのウィンドウの寿命にも注意。
	// C++ でラムダ式を使う場合は、さらにキャプチャ変数の参照先の寿命にも注意する必要がある。
	// なお、MFC スレッドでないただの Win32/CRT スレッドやマネージ スレッドからは、MFC オブジェクトにアクセスできない。
	// http://msdn.microsoft.com/ja-jp/library/h14y172e.aspx
	// ちなみに CWnd 系を ASSERT_VALID するには、メインスレッドからでないとダメらしい？
	// いっそ UI は WPF で作成して、サブスレッドから System.Windows.Threading.Dispatcher.Invoke() 経由で UI を操作するようにしたほうがよい。
#endif
}


void CMainFrame::OnMyButtonLoadEnvMapImg()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

#if 1
	CMyAutoFileExtUpdateDialog fileDlg(
#else
	CFileDialog fileDlg(
#endif
		true, _T("dds"), _T("*.dds"), OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		_T("DDS Image (*.dds)|*.dds|PNG Image (*.png)|*.png|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		//AfxMessageBox(fileDlg.GetPathName());

		// TODO: キューブマップもしくはスフィアマップを作成する。
		// キューブマップは DDS ファイルが使えると非常に楽。
		// ハードウェアで GL_EXT_texture_compression_s3tc 拡張（OpenGL 1.2 および OpenGL ES 2.0/3.0 の拡張）がサポートされていれば、
		// OpenGL 1.3 の glCompressedTexImage2D() で S3TC/DXTC 圧縮形式（DXT1=BC1, DXT2/DXT3=BC2, DXT4/DXT5=BC3）をそのまま扱える？
		// OpenGL 4.3 / ES 3.0 では ETC2/EAC のサポートが標準化されているらしい。
		// DirectX 11 では従来の DXT のほかに BC6H/BC7 が標準化されている。
		// OpenGL にも BPTC と呼ばれる BC6H/BC7 相当の圧縮フォーマットが追加されているらしいが、
		// 標準ではなく OpenGL 3.2 以降の拡張らしい（GL_ARB_texture_compression_bptc）。
		// http://wlog.flatlib.jp/item/1496
		// Windows PC 上であれば S3TC と BPTC だけでほぼ十分だが、モバイルの OpenGL ES で S3TC 対応しているのは NVIDIA ぐらいなので、
		// クロスプラットフォーム対応しようと思えば OpenGL モードでの ETC2/EAC の追加対応は必須。
		// なお、Web で見かける glCompressedTexImage2D() 関数を使用しているサンプルは「展開する」「解凍する」などという言葉を使っているが、まぎらわしい。
		// 圧縮したまま VRAM に転送できないと圧縮形式テクスチャを使う意味がない（展開というか圧縮したままサンプリングするのは GPU の仕事）。
		// http://www.webtech.co.jp/blog/optpixlabs/imageformat/4013/
		// UNDONE: DirectXTex で glCompressedTexImage2D() に渡すための S3TC/BPTC 圧縮データを取得できるか？
		// ScratchImage::GetPixels(), GetPixelsSize() は DDS ヘッダー（128バイト）を除いた生データを指している模様。
		// DirectXTex のスタティック ライブラリ プロジェクト（DirectXTex.h/.lib）には DDS 読み込み自体と D3D11 テクスチャのファイル保存機能はあるが、
		// D3DX の D3DX11CreateShaderResourceViewFromFile() などのように D3D11 テクスチャの生成と SRV の生成までまとめて行なう機能は
		// ソースファイルのみの提供になっている（DDSTextureLoader.h/.cpp）。
		// なお、GLI という OpenGL 向けテクスチャ ヘルパー ライブラリ（DDS 対応らしい）がオープン開発されている模様。
		// http://www.g-truc.net/project-0024.html
		// OpenGL での DDS 相当ファイル フォーマットとしては KTX というものがあるらしい。
		// http://dench.flatlib.jp/opengl/texturefileformat

		const CPathW texFilePath(fileDlg.GetPathName());
		const CStringW strFileExtension(texFilePath.GetExtension());

		DirectX::TexMetadata texMetadata = {};
		DirectX::ScratchImage texImage;

		HRESULT hr = E_FAIL;

		if (strFileExtension.CompareNoCase(L".dds") == 0)
		{
			ATLTRACE("DDS CubeMap/SphereMap.\n");
			hr = DirectX::LoadFromDDSFile(texFilePath.m_strPath.GetString(), DirectX::DDS_FLAGS_NONE, &texMetadata, texImage);
		}
		else if (strFileExtension.CompareNoCase(L".png") == 0)
		{
			ATLTRACE("PNG SphereMap.\n");
			hr = DirectX::LoadFromWICFile(texFilePath.m_strPath.GetString(), DirectX::WIC_FLAGS_NONE, &texMetadata, texImage);
		}
		else
		{
			// 拡張子なしなどには非対応。
			ATLTRACE("Unknown.\n");
		}

		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to load the image!!"));
		}
		else
		{
			ATLTRACE("IsCubeMap = %d\n", texMetadata.IsCubemap());
			ATLTRACE("Width = %Iu, Height = %Iu, Depth = %Iu\n", texMetadata.width, texMetadata.height, texMetadata.depth);
		}

#if 1
		// OpenGL/Direct3D 圧縮キューブマップ テクスチャの作成テスト。
		if (texMetadata.IsCubemap() && texMetadata.format == DXGI_FORMAT_BC3_UNORM && texMetadata.depth == 1)
		{
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
			if (!m_pOGLManager->CreateEnvCubeMap(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
				GLsizei(texMetadata.width), GLsizei(texMetadata.height), GLint(texMetadata.mipLevels),
				texImage.GetPixels(), texImage.GetPixelsSize()))
			{
				AfxMessageBox(_T("Failed to create OpenGL texture!!"));
			}
#endif

			if (!m_pD3DManager->CreateEnvCubeMap(texMetadata.format,
				uint32_t(texMetadata.width), uint32_t(texMetadata.height), uint32_t(texMetadata.mipLevels),
				texImage.GetPixels(), texImage.GetPixelsSize()))
			{
				AfxMessageBox(_T("Failed to create Direct3D texture!!"));
			}
		}
#endif
	}
}


void CMainFrame::OnMyComboAnimTrack()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。

	// コンボボックスの選択項目が変更されたり、テキストが入力されたときに、
	// 直接コントロール ID に関連付けられたこのコマンド メッセージ ハンドラーが呼ばれるらしい。
	// ちなみにリボン用のコンボボックスにはなぜかリスト部分をユーザー操作によってリサイズできる珍妙なスタイルがある。

	const int selectedAnimIndex = this->GetRibbonComboBoxSelectedIndex(ID_MY_COMBO_ANIM_TRACK);
	ATLTRACE(__FUNCTION__"(), SelectedAnimIndex = %d\n", selectedAnimIndex);

	// TODO: アニメーション ミキサーをセットアップする。
	// モーション ブレンドを行なうマニュアル モードも用意する。エディタもしくはスクリプト対応も必要。
	// Blender や LightWave のようなタイムライン エディタを作るのは厳しいかも。
	// [Load script] ボタンを用意して、フレーム単位のアニメーション遷移アルゴリズムを記述する Lua/Squirrel スクリプトをホット スワップできるとよい。

	MyCommon::SetSingleAnim(m_animMixerArrayArray, selectedAnimIndex);

	this->RenderMyScene(false);
}

