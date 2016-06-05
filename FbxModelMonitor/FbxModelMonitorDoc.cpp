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

// FbxModelMonitorDoc.cpp : CFbxModelMonitorDoc クラスの実装
//

#include "stdafx.h"
// SHARED_HANDLERS は、プレビュー、サムネイル、および検索フィルター ハンドラーを実装している ATL プロジェクトで定義でき、
// そのプロジェクトとのドキュメント コードの共有を可能にします。
#ifndef SHARED_HANDLERS
#include "FbxModelMonitor.h"
#endif

#include "FbxModelMonitorDoc.h"

#include <propkey.h>

#include "MainFrm.h"

#include "DebugNew.h"


// CFbxModelMonitorDoc

IMPLEMENT_DYNCREATE(CFbxModelMonitorDoc, CDocument);

BEGIN_MESSAGE_MAP(CFbxModelMonitorDoc, CDocument)
END_MESSAGE_MAP()


// CFbxModelMonitorDoc コンストラクション/デストラクション

CFbxModelMonitorDoc::CFbxModelMonitorDoc()
{
	// TODO: この位置に 1 度だけ呼ばれる構築用のコードを追加してください。

}

CFbxModelMonitorDoc::~CFbxModelMonitorDoc()
{
}

BOOL CFbxModelMonitorDoc::OnNewDocument()
{
	ATLTRACE(__FUNCTION__"()\n");
	if (!__super::OnNewDocument())
	{
		return FALSE;
	}

	// TODO: この位置に再初期化処理を追加してください。
	// (SDI ドキュメントはこのドキュメントを再利用します。)

	auto* pMainFrame = CMainFrame::GetTheMainFrame();
	_ASSERTE(pMainFrame != nullptr);

	pMainFrame->ClearMeshTree();

	return TRUE;
}




// CFbxModelMonitorDoc シリアル化

void CFbxModelMonitorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 格納するコードをここに追加してください。
	}
	else
	{
		// TODO: 読み込むコードをここに追加してください。
	}
}

#ifdef SHARED_HANDLERS

// サムネイルのサポート
void CFbxModelMonitorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// このコードを変更してドキュメントのデータを描画します
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 検索ハンドラーのサポート
void CFbxModelMonitorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// ドキュメントのデータから検索コンテンツを設定します。 
	// コンテンツの各部分は ";" で区切る必要があります

	// 例:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CFbxModelMonitorDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CFbxModelMonitorDoc 診断

#ifdef _DEBUG
void CFbxModelMonitorDoc::AssertValid() const
{
	__super::AssertValid();
}

void CFbxModelMonitorDoc::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}
#endif


// CFbxModelMonitorDoc コマンド

#if 0
BOOL CFbxModelMonitorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ATLTRACE(__FILE__"(%d): "__FUNCTION__"()\n", __LINE__);
	ATLTRACE(_T("PathName = <%s>\n"), lpszPathName);
	if (!__super::OnOpenDocument(lpszPathName))
	{
		return FALSE;
	}

	// TODO:  ここに特定な作成コードを追加してください。
#if 0
	return TRUE;
#else
	auto* pMainFrame = CMainFrame::GetTheMainFrame();
	_ASSERTE(pMainFrame != nullptr);

	return pMainFrame->LoadFromFbx(lpszPathName);
#endif
}
#endif
