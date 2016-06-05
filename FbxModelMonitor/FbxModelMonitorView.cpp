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

// FbxModelMonitorView.cpp : CFbxModelMonitorView クラスの実装
//

#include "stdafx.h"
// SHARED_HANDLERS は、プレビュー、サムネイル、および検索フィルター ハンドラーを実装している ATL プロジェクトで定義でき、
// そのプロジェクトとのドキュメント コードの共有を可能にします。
#ifndef SHARED_HANDLERS
#include "FbxModelMonitor.h"
#endif

#include "FbxModelMonitorDoc.h"
#include "FbxModelMonitorView.h"
#include "MainFrm.h"
#include "MyUtil.h"

#include "DebugNew.h"


// CFbxModelMonitorView

IMPLEMENT_DYNCREATE(CFbxModelMonitorView, CView);

BEGIN_MESSAGE_MAP(CFbxModelMonitorView, CView)
	// 標準印刷コマンド
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CFbxModelMonitorView::OnFilePrintPreview)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// CFbxModelMonitorView コンストラクション/デストラクション

CFbxModelMonitorView::CFbxModelMonitorView()
	: m_isMouseButtonPressedL()
	, m_isMouseButtonPressedR()
	, m_isMouseButtonPressedM()
	, m_mousePointOld(0, 0)
{
	// TODO: 構築コードをここに追加します。

}

CFbxModelMonitorView::~CFbxModelMonitorView()
{
}

BOOL CFbxModelMonitorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	return CView::PreCreateWindow(cs);
}

// CFbxModelMonitorView 描画

void CFbxModelMonitorView::OnDraw(CDC* /*pDC*/)
{
	CFbxModelMonitorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
	{
		return;
	}

	// TODO: この場所にネイティブ データ用の描画コードを追加します。
	auto* pMainFrame = CMainFrame::GetTheMainFrame();
	_ASSERTE(pMainFrame != nullptr);

	if (!pMainFrame->GetIsTimerValid())
	{
		pMainFrame->RenderMyScene(false, this);
	}
}


// CFbxModelMonitorView 印刷


void CFbxModelMonitorView::OnFilePrintPreview()
{
	AFXPrintPreview(this);
}

BOOL CFbxModelMonitorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 既定の印刷準備
	return DoPreparePrinting(pInfo);
}

void CFbxModelMonitorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 印刷前の特別な初期化処理を追加してください。
}

void CFbxModelMonitorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 印刷後の後処理を追加してください。
}

void CFbxModelMonitorView::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (this->GetIsNoButtonPressed())
	{
		m_isMouseButtonPressedR = true;
		this->SetCapture();
		m_mousePointOld = point;
	}

	CView::OnRButtonDown(nFlags, point);
}

void CFbxModelMonitorView::OnRButtonUp(UINT nFlags, CPoint point)
{
#if 0
	ClientToScreen(&point);
	OnContextMenu(this, point);
#endif

	if (m_isMouseButtonPressedR)
	{
		m_isMouseButtonPressedR = false;
		::ReleaseCapture();
	}

	CView::OnRButtonUp(nFlags, point);
}

void CFbxModelMonitorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (this->GetIsNoButtonPressed())
	{
		m_isMouseButtonPressedL = true;
		this->SetCapture();
		m_mousePointOld = point;
	}

	CView::OnLButtonDown(nFlags, point);
}

void CFbxModelMonitorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_isMouseButtonPressedL)
	{
		m_isMouseButtonPressedL = false;
		::ReleaseCapture();
	}

	CView::OnLButtonUp(nFlags, point);
}

void CFbxModelMonitorView::OnMButtonDown(UINT nFlags, CPoint point)
{
	if (this->GetIsNoButtonPressed())
	{
		m_isMouseButtonPressedM = true;
		this->SetCapture();
		m_mousePointOld = point;
	}

	CView::OnMButtonDown(nFlags, point);
}

void CFbxModelMonitorView::OnMButtonUp(UINT nFlags, CPoint point)
{
	if (m_isMouseButtonPressedM)
	{
		m_isMouseButtonPressedM = false;
		::ReleaseCapture();
	}

	CView::OnMButtonUp(nFlags, point);
}

void CFbxModelMonitorView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。

	const float ThresholdVal = 1e-4f;

	if (m_isMouseButtonPressedR)
	{
		auto* pMainFrame = CMainFrame::GetTheMainFrame();
		ATLASSERT(pMainFrame != nullptr);

		// カメラ側ではなく、すべてのオブジェクトを回転させる。
		// ただしライトは回転しない。

		MyMath::Vector3F vRotation;
		pMainFrame->GetRotationAmount(&vRotation);
		// マウス座標の (X, Y) と回転方向の (X, Y) の違いに注意。右手系を基準にしている。
		vRotation.x += 0.01f * (point.y - m_mousePointOld.y); // 逆回転にすると、FPS ゲーム風になる。
		vRotation.y += 0.01f * (point.x - m_mousePointOld.x);
		vRotation.z = 0.0f;
		vRotation.x = fmod(vRotation.x, 2 * MyMath::F_PI);
		vRotation.y = fmod(vRotation.y, 2 * MyMath::F_PI);
		// -PI/2 < Pitch < +PI/2 に制限する。
		vRotation.x = MyUtil::Clamp(vRotation.x, -MyMath::F_PI * 0.5f + ThresholdVal, +MyMath::F_PI * 0.5f - ThresholdVal);
		// ちなみに Metasequoia の場合、右ドラッグ中にマウスを上下に移動させると、画面に平行な軸まわりに回転する。X 軸まわりというわけではない。
		// また、中ドラッグ中は画面座標での移動量に応じてカメラがパンするようになっている。
		// 他の 3DCG ソフトと比べても非常に操作性がよいが、パース表示で拡大縮小を繰り返していると、やがて表示が崩れていく現象が発生する。
		pMainFrame->SetRotationAmount(&vRotation);

		m_mousePointOld = point;

		//this->Invalidate(false);
		pMainFrame->InvalidateAllViews();
	}
	else if (m_isMouseButtonPressedL)
	{
		auto* pMainFrame = CMainFrame::GetTheMainFrame();
		ATLASSERT(pMainFrame != nullptr);

		// メイン ライトの方向ベクトルを回転させる。
		MyMath::Vector3F vRotation;
		pMainFrame->GetMainLightRotationAmount(&vRotation);
		vRotation.x += 0.01f * (point.y - m_mousePointOld.y);
		vRotation.y += 0.01f * (point.x - m_mousePointOld.x);
		vRotation.z = 0.0f;
		vRotation.x = fmod(vRotation.x, 2 * MyMath::F_PI);
		vRotation.y = fmod(vRotation.y, 2 * MyMath::F_PI);
		vRotation.x = MyUtil::Clamp(vRotation.x, -MyMath::F_PI * 0.5f + ThresholdVal, +MyMath::F_PI * 0.5f - ThresholdVal);
		pMainFrame->SetMainLightRotationAmount(&vRotation);

		m_mousePointOld = point;

		//this->Invalidate(false);
		pMainFrame->InvalidateAllViews();
	}
	else if (m_isMouseButtonPressedM)
	{
		auto* pMainFrame = CMainFrame::GetTheMainFrame();
		ATLASSERT(pMainFrame != nullptr);

		// オブジェクトではなく、カメラのほうをパンする。
		const MyMath::Vector2F shiftInPix(
			-float(point.x - m_mousePointOld.x),
			+float(point.y - m_mousePointOld.y)
			);

		pMainFrame->PanCamera(shiftInPix);

		m_mousePointOld = point;

		//this->Invalidate(false);
		pMainFrame->InvalidateAllViews();
	}

	CView::OnMouseMove(nFlags, point);
}

void CFbxModelMonitorView::OnContextMenu(CWnd* pWnd, CPoint point)
{
#ifndef SHARED_HANDLERS
	g_theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CFbxModelMonitorView 診断

#ifdef _DEBUG
void CFbxModelMonitorView::AssertValid() const
{
	CView::AssertValid();
}

void CFbxModelMonitorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFbxModelMonitorDoc* CFbxModelMonitorView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFbxModelMonitorDoc)));
	return static_cast<CFbxModelMonitorDoc*>(m_pDocument);
}
#endif


// CFbxModelMonitorView メッセージ ハンドラ

void CFbxModelMonitorView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	// TODO: ここにメッセージ ハンドラ コードを追加します。
	ATLTRACE(__FUNCTION__"(), this = 0x%p, cx = %d, cy = %d\n", this, cx, cy);
#if 0
	auto* pMainFrame = CMainFrame::GetTheMainFrame();
	ATLASSERT(pMainFrame != nullptr);

	CSize spwSize;
	if (pMainFrame->GetSplitterWndClientSize(spwSize))
	{
		// UNDONE: D3D スワップ チェーンのリサイズ（リセット）を行なう。
	}
#endif
}

BOOL CFbxModelMonitorView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。

	{
		auto* pMainFrame = CMainFrame::GetTheMainFrame();
		ATLASSERT(pMainFrame != nullptr);

		// オブジェクトではなく、カメラのほうをズームする。
		MyMath::Vector3F vEyePos;
		pMainFrame->GetCameraEye(&vEyePos);
		// 上方向に回すと近づき、下方向に回すと遠ざかる。
		//vEyePos.z -= 5.0f * static_cast<float>(zDelta) / WHEEL_DELTA;
		vEyePos.z -= 20.0f * static_cast<float>(zDelta) / WHEEL_DELTA;
		pMainFrame->SetCameraEye(&vEyePos);
		//this->Invalidate(false);
		pMainFrame->InvalidateAllViews();
	}

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}


BOOL CFbxModelMonitorView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	//return CView::OnEraseBkgnd(pDC);
	return true;
	//return false;
}
