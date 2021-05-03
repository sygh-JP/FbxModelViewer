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

// FbxModelMonitor.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include <afxwinappex.h>
#include "FbxModelMonitor.h"
#include "MainFrm.h"
#include "UserDefWinMsg.hpp"

#include "FbxModelMonitorDoc.h"
#include "FbxModelMonitorView.h"
#include "MyFbx.h"
#include "MyStopwatch.hpp"
#include "AutoFileExtUpdateDlg.h"
#include "../MyWpfGraphLibMfc/Wrappers/TaskProgressDialogWrapper.hpp"

#include "DebugNew.h"


#ifdef _DEBUG
// Math ライブラリのテスト コード。
// TODO: ユニット テスト専用プロジェクトに移動する。
namespace
{
	void DumpQuat(const char* pName, const MyMath::QuaternionF& quat)
	{
		ATLTRACE("%s = (%+f, %+f, %+f, %+f)\n", pName, quat.x, quat.y, quat.z, quat.w);
	}

	void DoMyMathTest()
	{
		ATLTRACE("---- Begin of Test ----\n");
		constexpr auto fnan = std::numeric_limits<float>::quiet_NaN();
		constexpr auto dnan = std::numeric_limits<double>::quiet_NaN();
		constexpr auto finf = std::numeric_limits<float>::infinity();
		constexpr auto dinf = std::numeric_limits<double>::infinity();
		// MSVC 2013 には C99 の isnan() や isfinite() が実装されていないかのように記載されているが、実際にはちゃんと実装されている。
		// C はマクロ実装。C++ はテンプレート実装。
		// https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2013/tzthab44(v=vs.120)
		// https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2013/sb8es7a8(v=vs.120)
		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/finite-finitef?view=msvc-140
		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/isnan-isnan-isnanf?view=msvc-140
		_ASSERTE(_isnan(fnan) && _isnan(dnan));
		_ASSERTE(!_finite(fnan) && !_finite(dnan)); // 非数 NaN もまた有限ではない。
		_ASSERTE(!_finite(finf) && !_finite(dinf));
		_ASSERTE(std::isnan(fnan) && std::isnan(dnan));
		_ASSERTE(!std::isfinite(fnan) && !std::isfinite(dnan));
		_ASSERTE(!std::isfinite(finf) && !std::isfinite(dinf));

		auto testMat1 = MyMath::IDENTITY_MATRIXF;
		auto testMat2 = MyMath::IDENTITY_MATRIXF;
		auto testMatA = MyMath::IDENTITY_MATRIXF;
		auto testMatB = MyMath::IDENTITY_MATRIXF;
		MyMath::CreateMatrixRotationZXY(&testMatA, &MyMath::Vector3F(MyMath::F_PIDIV2, 0, 0));
		MyMath::CreateMatrixRotationZXY(&testMatB, &MyMath::Vector3F(0, MyMath::F_PIDIV4, 0));
		MyMath::CreateMatrixRotationZXY(&testMat1, &MyMath::Vector3F(MyMath::F_PIDIV2, MyMath::F_PIDIV4, 0));
		const auto testMatC = testMatA * testMatB;
#if 0
		testMat(3, 0) = 10;
		testMat(3, 1) = 11;
		testMat(3, 2) = 12;
#endif
		MyMath::QuaternionF testQuatA = MyMath::ZERO_VECTOR4F;
		MyMath::QuaternionF testQuatB = MyMath::ZERO_VECTOR4F;
		MyMath::QuaternionF testQuatC = MyMath::ZERO_VECTOR4F;
		MyMath::QuaternionF testQuat1 = MyMath::ZERO_VECTOR4F;
		MyMath::QuaternionF testQuat2 = MyMath::ZERO_VECTOR4F;
		MyMath::QuaternionF testQuat3 = MyMath::ZERO_VECTOR4F;
		MyMath::CreateQuaternionFromRotationMatrix(&testQuatA, &testMatA);
		MyMath::CreateQuaternionFromRotationMatrix(&testQuatB, &testMatB);
		MyMath::CreateQuaternionFromRotationMatrix(&testQuatC, &testMatC);
		MyMath::CreateQuaternionFromRotationMatrix(&testQuat1, &testMat1);
		MyMath::JointQuaternion(&testQuat2, &testQuatA, &testQuatB); // mC = mA * mB と一致する。
		MyMath::JointQuaternion(&testQuat3, &testQuatB, &testQuatA); // mC = mA * mB と一致しない。
		const auto testQuat4 = MyMath::QuaternionF::Concatenate(testQuatA, testQuatB); // mC = mA * mB と一致しない。
		const auto testQuat5 = MyMath::QuaternionF::Concatenate(testQuatB, testQuatA); // mC = mA * mB と一致する。
		const auto testQuat6 = testQuatA * testQuatB; // mC = mA * mB と一致する。
		const auto testQuat7 = testQuatB * testQuatA; // mC = mA * mB と一致しない。
		DumpQuat("testQuatA", testQuatA);
		DumpQuat("testQuatB", testQuatB);
		DumpQuat("testQuatC", testQuatC);
		DumpQuat("testQuat1", testQuat1);
		DumpQuat("testQuat2", testQuat2);
		DumpQuat("testQuat3", testQuat3);
		DumpQuat("testQuat4", testQuat4);
		DumpQuat("testQuat5", testQuat5);
		DumpQuat("testQuat6", testQuat6);
		DumpQuat("testQuat7", testQuat7);
		MyMath::CreateMatrixFromRotationQuaternion(&testMat2, &testQuat1);
		MyMath::QuatTransform trans1(testQuatA, MyMath::Vector3F(1,0,0));
		MyMath::QuatTransform trans2(testQuatB, MyMath::Vector3F(0,1,0));
		MyMath::QuatTransform trans1x2 = MyMath::QuatTransform::Multiply(trans1, trans2);
		MyMath::QuatTransform trans3 = MyMath::QuatTransform::MultiplyInverse(trans1x2, trans2);
		ATLTRACE("---- End of Test ----\n");
	}

	void DoMyUnicodeTest()
	{
		ATLTRACE("---- Begin of Test ----\n");

		_ASSERTE(MyUtils::SafeConvertUtf16toUtf8(nullptr) == "");
		_ASSERTE(MyUtils::SafeConvertUtf8toUtf16(nullptr) == L"");

		// nullptr と比較するとアサーションが失敗する。
		//ATLTRACE("CString().Compare(nullptr) = %d\n", CString().Compare(nullptr));
#if 0
		std::unordered_map<MyCharHelpers::UnicodePair, int, std::function<decltype(MyCharHelpers::GetUnicodePairHashCode)>> umap(
			100, MyCharHelpers::GetUnicodePairHashCode);
#else
		//std::unordered_map<MyCharHelpers::UnicodePair, int> umap;
		std::unordered_map<MyCharHelpers::UnicodePair, int, MyCharHelpers::UnicodePair::HashFunctor> umap;
#endif
		// サロゲート ペアとハッシュ テーブルのテスト。
		const wchar_t testString[] = L"test𩸽𠮷𠮟1234日本語圡©®";
		const size_t testLen = wcslen(testString);
		for (size_t i = 0; i < testLen; ++i)
		{
			if (MyCharHelpers::IsHighSurrogate(testString[i]) &&
				MyCharHelpers::IsLowSurrogate(testString[i + 1]))
			{
				umap[MyCharHelpers::UnicodePair(testString[i], testString[i + 1])] = -int(i);
				++i;
			}
			else
			{
				umap[MyCharHelpers::UnicodePair(testString[i])] = int(i);
			}
		}
		ATLTRACE("StrLen = %Iu, MapSize = %Iu\n", testLen, umap.size());
		//ATLTRACE("size_t(-1) = %zu, ptrdiff_t(-1) = %td\n", size_t(-1), ptrdiff_t(-1));
		//CStringW str;
		//str.Format(L"size_t(-1) = %zu, ptrdiff_t(-1) = %td", size_t(-1), ptrdiff_t(-1));
		ATLTRACE("---- End of Test ----\n");
	}
}
#endif


// CFbxModelMonitorApp

BEGIN_MESSAGE_MAP(CFbxModelMonitorApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CFbxModelMonitorApp::OnAppAbout)
	// 標準のファイル基本ドキュメント コマンド
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	ON_COMMAND(ID_FILE_OPEN, &CFbxModelMonitorApp::OnFileOpen)
	//ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, &CWinApp::OnOpenRecentFile)
	ON_COMMAND_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, &CFbxModelMonitorApp::OnOpenRecentFile)
	// 標準の印刷セットアップ コマンド
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// CFbxModelMonitorApp コンストラクション

CFbxModelMonitorApp::CFbxModelMonitorApp()
{
	// 再起動マネージャー関連コードおよび AppID 関連コードは VS 2010 以降のウィザードで挿入される。
	// AppID は Windows 7 タスク バーに、複数の同一プログラム（マルチブート プログラム）をまとめて表示する際の指標となるらしい。
#if 1
	// 再起動マネージャーをサポートします
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// アプリケーションが共通言語ランタイム サポート (/clr) を使用して作成されている場合:
	//     1) この追加設定は、再起動マネージャー サポートが正常に機能するために必要です。
	//     2) 作成するには、プロジェクトに System.Windows.Forms への参照を追加する必要があります。
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: 下のアプリケーション ID 文字列を一意の ID 文字列で置換します。推奨される
	// 文字列の形式は CompanyName.ProductName.SubProduct.VersionInformation です
	SetAppID(_T("FbxModelMonitor.AppID.NoVersion"));
#endif

	m_bHiColorIcons = TRUE; // VS 2010 以降のウィザードでは、このフラグ関連コードが出力されない。フルカラー前提になったらしい。
	// なお、最近の PC であればデフォルトの画面設定が 32bit フルカラー色深度となっているので、たいていの場合問題ないが、
	// リモート デスクトップなどで 16bit ハイカラーなどの低品質設定になっていると、32bit ピクセルは自動減色の対象となる。

	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。

	MyUtils::HRStopwatch::Initialize();
}

// 唯一の CFbxModelMonitorApp オブジェクトです。

CFbxModelMonitorApp g_theApp;


// CFbxModelMonitorApp 初期化

BOOL CFbxModelMonitorApp::InitInstance()
{
	// TRACE(), ATLTRACE(), およびそれらの内部で使用されている OutputDebugStringW() で
	// 非 ASCII のワイド文字を出力する場合、ロケールの設定をする必要があるが、
	// ロケールを設定するとヨーロッパ圏の一部ではコンマ・ピリオドの扱いが変わってしまうことにも注意。
	// また、ロケールを設定しても、現在の ANSI コードページで表示できない Unicode 文字を OutputDebugStringW() で
	// 出力ウィンドウに表示することはできないので注意。
	_tsetlocale(LC_ALL, _T(""));

	// アプリケーション マニフェストが visual スタイルを有効にするために、
	// ComCtl32.dll Version 6 以降の使用を指定する場合は、
	// Windows XP に InitCommonControlsEx() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	INITCOMMONCONTROLSEX InitCtrls = {};
	InitCtrls.dwSize = sizeof(InitCtrls);
	// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
	// これを設定します。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	_ASSERTE(CTaskDialog::IsSupported()); // Windows 7 以降がターゲットで、さらに必ず Unicode ビルドしているはずなので保証される。

#ifdef _DEBUG
	DoMyMathTest();
	DoMyUnicodeTest();
#endif

	// Visual C++ のアサーションにはいくつか方法（マクロ）が用意されている。
	// Visual C++ 環境前提であれば、CRT Debug ライブラリ <crtdbg.h> の _ASSERTE() が使える。
	// _ASSERTE() は _DEBUG が定義されているときに有効になる。
	// ATL が使える Visual C++では、<atldef.h> の ATLASSERT() を使ってもよい。
	// 以前は ATL/MFC は有償版（2005/2008 の場合は Standard エディション以上、2010/2012 は Professional エディション以上）でなければ
	// 使えなかったが、2013 以降は Community エディションでも使える。
	// ATLASSERT() は _ASSERTE() に置き換わるようになっている。
	// なお、MFC 環境であれば <afx.h> の ASSERT() を使ってもよいが、MFC はストア アプリ (WinRT) では使えないので、
	// コードの再利用性が下がる。今後は使用を控えたほうがいい。
	// 非 MFC 環境では ASSERT が _ASSERTE に置換されるようマクロ定義する手もある。
	// 標準 C/C++ の assert() は、NDEBUG が定義されているときに無効になる。
	// MSVC 実装の assert() はアサーションが失敗したときに
	// 内部の DebugBreak() 呼び出しまでさかのぼってしまうので使いづらい。
	// どうしても使わざるを得ない状況を除いて、使用は避けたほうがいい。
	// ……と思っていたが、Visual C++ 2013 では、assert() マクロでもアサートが失敗した位置で停止するようになっている。
	// なお、デバッグ中に assert() で停止した際に
	// "XXX.exe によってブレークポイントが発生しました。"
	// というメッセージダイアログが Visual Studio から表示されるが、
	// [中断(B)]/[継続(C)] で [継続] を選択してもプログラムを続行できずに即座に終了する。abort() が呼ばれる模様。
	// また、停止位置などの情報を含むエラーメッセージはコンソール（おそらく標準エラー）にも出力される模様。
	// _ASSERTE() などは [継続] で続行できる。abort() は呼ばれない。
	// なお、VC2013 までは、どのアサートマクロでも [Microsoft Visual C++ Runtime Library] というタイトルと
	// [中止(A)]/[再試行(R)]/[無視(I)] のボタンを持つエラーダイアログが出るようになっていたが、
	// VC2015 ではデバッガーをアタッチするとダイアログが出なくなるようになってしまっている。アタッチしないと以前のバージョン同様に出る。
	// 仕様なのか、それともバグなのかは不明。
	// 
	// <afx.h> の TRACE() に関しても ASSERT() 同様に MFC 専用。<atltrace.h> の ATLTRACE() を使ったほうがいい。
	// ちなみに Visual Studio 2013 の ATLTRACE() では、
	// hoge.cpp(123) : atlTraceGeneral - 出力メッセージ
	// というような感じで、ファイルパスと行番号と診断レベルが出力メッセージ先頭に付加されるようになった。
	// Visual Studio IDE には「FilePath(LineNumber):」の形で出力ウィンドウに表示されたメッセージを
	// ダブルクリックすることで該当行にジャンプできる機能が昔からあったのだが、
	// プログラマーが __FILE__ と __LINE__ を使って特に工夫するまでもなく自動的にこの機能が利用できる形になった。
	// ただ、常に情報量が増えることでノイズも増え、メッセージ本体が埋もれてしまってうっとうしいと感じるかもしれない。

	//_ASSERT(false);
	//_ASSERTE(false);
	//_ASSERT_EXPR(false, L"My assertion failure message!!");
	//ATLASSERT(false);
	//ASSERT(false);
	//assert(false);

	// OLE ライブラリを初期化します。
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED, MB_ICONERROR);
		return FALSE;
	}

	//CMFCButton::EnableWindowsTheming();

	//AfxInitRichEdit();
	AfxInitRichEdit2();

	AfxEnableControlContainer();
	// 標準初期化
	// これらの機能を使わずに最終的な実行可能ファイルの
	// サイズを縮小したい場合は、以下から不要な初期化
	// ルーチンを削除してください。
	// 設定が格納されているレジストリ キーを変更します。
	// TODO: 会社名または組織名などの適切な文字列に
	// この文字列を変更してください。
	SetRegistryKey(_T("アプリケーション ウィザードで生成されたローカル アプリケーション"));
	const UINT maxMruCount = 16; // 「最近使ったドキュメント」で管理される最大項目数
	LoadStdProfileSettings(maxMruCount);  // 標準の INI ファイルのオプションを読み込みます

	CString exeFileName;
	MyDesktopHelpers::GetModuleFilePath(nullptr, exeFileName);
	ATLTRACE(_T(__FUNCTION__) _T("(), ModuleFileName = \"%s\"\n"), exeFileName.GetString());

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	this->GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// アプリケーション用のドキュメント テンプレートを登録します。ドキュメント テンプレート
	//  はドキュメント、フレーム ウィンドウとビューを結合するために機能します。
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFbxModelMonitorDoc),
		RUNTIME_CLASS(CMainFrame),       // メイン SDI フレーム ウィンドウ
		RUNTIME_CLASS(CFbxModelMonitorView));
	if (!pDocTemplate)
	{
		return FALSE;
	}
	AddDocTemplate(pDocTemplate);



	// DDE、file open など標準のシェル コマンドのコマンド ラインを解析します。
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// HACK: ファイル オープン コマンドが渡されても、メイン ウィンドウと D3D/GL が初期化されていないとメッシュが作成できない。


	// コマンド ラインで指定されたディスパッチ コマンドです。アプリケーションが
	// /RegServer、/Register、/Unregserver または /Unregister で起動された場合、False を返します。
	if (!ProcessShellCommand(cmdInfo))
	{
		return FALSE;
	}

#pragma region // 分割ウィンドウのサイズの初期化。//
	auto* pMainFrame = CMainFrame::GetTheMainFrame();
	ATLASSERT(pMainFrame != nullptr);
	CSize spwSize;
	if (pMainFrame->GetSplitterWndClientSize(spwSize))
	{
		ATLTRACE(__FUNCTION__"(), SplitterWndClientSize = (%d, %d)\n", spwSize.cx, spwSize.cy);
#if 0
		// 幅・高さがそれぞれ非分割の場合のほぼ1/2になるように適切なサイズを決定する。
		pMainFrame->UpdateMySplitterWndRowColumnSize(0, 0, spwSize.cx / 2, spwSize.cy / 2, 100, 100);
		pMainFrame->UpdateMySplitterWndRowColumnSize(0, 1, spwSize.cx / 2, spwSize.cy / 2, 100, 100);
		pMainFrame->UpdateMySplitterWndRowColumnSize(1, 0, spwSize.cx / 2, spwSize.cy / 2, 100, 100);
		pMainFrame->UpdateMySplitterWndRowColumnSize(1, 1, spwSize.cx / 2, spwSize.cy / 2, 100, 100);
#else
		// 高さがそれぞれ非分割の場合のほぼ1/2になるように適切なサイズを決定する。
		pMainFrame->UpdateMySplitterWndRowColumnSize(0, 0, spwSize.cx, spwSize.cy / 2, 100, 100);
		pMainFrame->UpdateMySplitterWndRowColumnSize(1, 0, spwSize.cx, spwSize.cy / 2, 100, 100);
#endif
		pMainFrame->RecalcSplitterWndLayout();
	}
#pragma endregion
	if (!pMainFrame->InitDirect3DAndOpenGL())
	{
		pMainFrame->SendMessage(WM_CLOSE);
		return FALSE;
	}

	// メイン ウィンドウが初期化されたので、表示と更新を行います。
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// 接尾辞が存在する場合にのみ DragAcceptFiles を呼び出してください。
	//  SDI アプリケーションでは、ProcessShellCommand の直後にこの呼び出しが発生しなければなりません。

	return TRUE;
}

int CFbxModelMonitorApp::ExitInstance()
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。

	//AfxOleTerm(FALSE);

	return __super::ExitInstance();
}


#pragma region // アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

#pragma endregion


// ダイアログを実行するためのアプリケーション コマンド
void CFbxModelMonitorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

namespace
{
	// FBX ファイルの読み込み・解析と、メッシュ作成（GPU リソース確保）は分けている。
	// ただし、どちらもサブスレッドに逃がす。メインスレッド以外から実行されることを想定してコードを記述する必要がある。

	MyFbx::MyFbxNodeAnalyzerBase::TSharedPtr LoadFbxFileImpl(CMainFrame* pMainFrame, CStringW strFilePath)
	{
		_ASSERTE(pMainFrame);
		auto hMainWnd = pMainFrame->GetSafeHwnd();
		auto progressDlg = MyWpfGraphLibWrapper::MiscControls::IMyTaskProgressDialogWrapper::Create(hMainWnd, L"MyTaskProgressDlg");

		//auto task1 = concurrency::create_task([=]() { return pMainFrame->LoadFbxFile(strFilePath); });
		//auto task2 = task1.then([=](decltype(task1)) { MyAppHelpers::InvokeWith(hMainWnd, [=]() { progressDlg->EnforcedClose(); return 0; }); });
		//progressDlg->ShowModalDialog();
		//task2.get();
		// タスク1が完了する前にタスク2の結果を get() で取得すると、呼び出しスレッドはタスク1の結果が返るまで待機させられるはず。
		// UNDONE: ダイアログの表示前に直接 task.then() を使う場合、ダイアログの表示の前にタスクが先に完了してしまうとダイアログが閉じられない。
		// モーダルではなくモードレスにするべき。あるいは ContentRendered イベント ハンドラーでタスクを開始する。

		MyFbx::MyFbxNodeAnalyzerBase::TSharedPtr retVal;
#if 1
		// マネージ コード側で非同期処理をする。
		progressDlg->SetEventMainWorkStarted([=, &retVal]()
		{ retVal = pMainFrame->LoadFbxFile(strFilePath); });
#else
		// ネイティブ コード側で非同期処理をする。
		// concurrency::create_task() 呼び出しでタスクの開始が Fire される。
		// C# 側で async/await を使った非同期処理をすれば、await 後にメインスレッドへのコンテキスト自動復帰がなされる。
		// C++ 側で TPL や PPL を使った非同期処理をすると、ContinueWith や then 後にメインスレッドへのコンテキスト自動復帰は
		// されない（ただし C++/CX すなわち WinRT は例外）。
		// HACK: MFC UI スレッドにむりやり戻した後で WPF を操作しようとすると、RuntimeCallableWrapper に絡む例外がスローされてしまう。
		// 非同期処理の開始はマネージ側で Fire する形にしたほうがよいかも。
		progressDlg->SetEventMainWorkStarted([=, &retVal]()
		{
			concurrency::create_task([=, &retVal]()
			{
				retVal = pMainFrame->LoadFbxFile(strFilePath);
				//progressDlg->EnforcedClose();
				//MyAppHelpers::InvokeWith(hMainWnd, [=]() { progressDlg->EnforcedClose(); return 0; });
			}
			).then([=]() { progressDlg->EnforcedClose(); });
		});
#endif
		progressDlg->ShowModalDialog();
		// TODO: 途中で閉じられたら中断フラグを立てる。
		// Visual Studio のソリューション読込時の動作と同様に、中断はできないようにする？
		return retVal;
	}

	bool CreateDeviceMeshImpl(CMainFrame* pMainFrame, CStringW strFilePath, const MyFbx::MyFbxNodeAnalyzerBase* pNodeAnalyzer)
	{
		_ASSERTE(pMainFrame);
		_ASSERTE(pNodeAnalyzer);
		auto hMainWnd = pMainFrame->GetSafeHwnd();
		auto progressDlg = MyWpfGraphLibWrapper::MiscControls::IMyTaskProgressDialogWrapper::Create(hMainWnd, L"MyTaskProgressDlg");

		bool retVal = false;
#if 1
		progressDlg->SetEventMainWorkStarted([=, &retVal]()
		{ retVal = pMainFrame->CreateDeviceMeshFromFbx(strFilePath, *pNodeAnalyzer); });
#else
		progressDlg->SetEventMainWorkStarted([=, &retVal]()
		{
			concurrency::create_task([=, &retVal]()
			{
				retVal = pMainFrame->CreateDeviceMeshFromFbx(strFilePath, *pNodeAnalyzer);
				//progressDlg->EnforcedClose();
				//MyAppHelpers::InvokeWith(hMainWnd, [=]() { progressDlg->EnforcedClose(); return 0; });
			}
			).then([=]() { progressDlg->EnforcedClose(); });
		});
#endif
		progressDlg->ShowModalDialog();

		return retVal;
	}
}

void CFbxModelMonitorApp::LoadFbxFile(const CStringW& strFilePath)
{
	// TODO: 非同期で FBX ファイル読み込みと解析を行なう。
	auto pMainFrame = CMainFrame::GetTheMainFrame();
	pMainFrame->ClearMeshTree();
	auto nodeAnalyzer = LoadFbxFileImpl(pMainFrame, strFilePath);
	if (!nodeAnalyzer)
	{
		return;
	}
	pMainFrame->SetIsLoadingResourcesNow(true);
	// TODO: 非同期で GPU リソース生成を行なう。
#if 0
	if (pMainFrame->CreateDeviceMeshFromFbx(strFilePath, *nodeAnalyzer))
#else
	if (CreateDeviceMeshImpl(pMainFrame, strFilePath, nodeAnalyzer.get()))
#endif
	{
		this->OpenDocumentFile(strFilePath);
	}
	else
	{
		pMainFrame->ClearMeshTree();
	}
	pMainFrame->SetIsLoadingResourcesNow(false);
	// 失敗した場合、中途半端に生成されているものがあれば削除しておく。
	// HACK: 後始末をしなくて済むように、正常終了したときのみにスワップするようにする。
	// また、サブスレッドで生成途中のデータを使ってレンダリングしてはいけない。
}

void CFbxModelMonitorApp::OnFileOpen()
{
	ATLTRACE(__FUNCTION__"()\n");

	CMyAutoFileExtUpdateDialog fileDlg(
		true, _T("fbx"), _T("*.fbx"), OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		_T("FBX Files (*.fbx)|*.fbx|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() != IDOK)
	{
		return;
	}

	const CStringW strFilePath = fileDlg.GetPathName();
	this->LoadFbxFile(strFilePath);

	// デフォルトの実装では、
	// CWinApp::OnFileOpen() → CDocManager::OnFileOpen() → CDocManager::DoPromptFileName()
	// によってダイアログが表示され、その後
	// CWinApp::OpenDocumentFile() → CDocManager::OpenDocumentFile() → CDocTemplate::OpenDocumentFile() →
	// CMultiDocTemplate/CSingleDocTemplate::OpenDocumentFile() → CDocument::OnOpenDocument()
	// という流れになる。
	// ファイル読み込みを非同期処理するならば、戻り値を持たない ID_FILE_OPEN メッセージ ハンドラーからのカスタマイズが必須。
	// C# でいうと async void イベント ハンドラーに相当。
	// まずファイル ダイアログは明示的に表示するようにして、ファイル名を取得してから
	// メインスレッドでは独自のモーダル プログレス ダイアログを表示しつつサブスレッドでファイル読込し、処理が完了した後で
	// CWinApp::OpenDocumentFile() を明示的に呼び出せばよいはず。
	// CDocument::OnOpenDocument() はデフォルトの処理にする（特に読み込みは行なわない）。
	// これにより、MRU などのこまごまとした部分は MFC に管理させることができるはず。
	// MRU ファイル リストから開く場合は、MRU の各コマンド ハンドラーを独自の非同期読み込み処理に置き換える必要がある。
	// デフォルトでは、
	// ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, &CWinApp::OnOpenRecentFile)
	// として登録されているので、
	// 派生クラスで別のメッセージ ハンドラー OnOpenRecentFile() を明示的に作成して登録し直す。
}

void CFbxModelMonitorApp::OnOpenRecentFile(UINT cmdId)
{
	ATLTRACE(__FUNCTION__"()\n");

	if (!this->m_pRecentFileList)
	{
		return;
	}

	const int mruSize = m_pRecentFileList->GetSize();
	const int mruIndex = cmdId - ID_FILE_MRU_FILE1; // ID_FILE_MRU_FILE1～ID_FILE_MRU_FILE16 は連続している。
	if (mruIndex < 0 || mruSize <= mruIndex)
	{
		return;
	}

	const CStringW strFilePath = (*m_pRecentFileList)[mruIndex];
	this->LoadFbxFile(strFilePath);
}

// CFbxModelMonitorApp のカスタマイズされた読み込みメソッドと保存メソッド

void CFbxModelMonitorApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CFbxModelMonitorApp::LoadCustomState()
{
}

void CFbxModelMonitorApp::SaveCustomState()
{
}

// CFbxModelMonitorApp メッセージ ハンドラ


