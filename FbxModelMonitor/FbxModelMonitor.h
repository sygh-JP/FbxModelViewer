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

// FbxModelMonitor.h : FbxModelMonitor アプリケーションのメイン ヘッダー ファイル
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"       // メイン シンボル


#include "GdiPlusUtility.h"


// CFbxModelMonitorApp:
// このクラスの実装については、FbxModelMonitor.cpp を参照してください。
//

class CFbxModelMonitorApp : public CWinAppEx
{
public:
	CFbxModelMonitorApp();


// オーバーライド
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

// 実装
public:
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

private:
	Misc::GdiPlusInitializer m_gdipInitializer;

private:
	void LoadFbxFile(const CStringW& strFilePath);

public:
	virtual void PreLoadState() override;
	virtual void LoadCustomState() override;
	virtual void SaveCustomState() override;
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU = true) override
	{
		ATLTRACE(__FUNCTION__"()\n");
		return __super::OpenDocumentFile(lpszFileName, bAddToMRU);
	}

	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	afx_msg void OnOpenRecentFile(UINT cmdId);
	DECLARE_MESSAGE_MAP()
};

extern CFbxModelMonitorApp g_theApp;
