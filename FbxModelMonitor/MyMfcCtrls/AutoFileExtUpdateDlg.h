#pragma once


//! @brief  ファイルの種類を変更すると、入力したファイル名の拡張子部分を自動的に更新するファイル ダイアログ。<br>
class CMyAutoFileExtUpdateDialog sealed : public CFileDialog
{
private:
	bool m_hasOnTypeChangeBeenCalled; //!< ダイアログ起動直後はファイル名を強制セットしないようにするためのフラグ。<br>

public:
	//! @brief  コンストラクタ。<br>
	//! 
	//! @param  bOpenFileDialog  TRUE for FileOpen, FALSE for FileSaveAs. <br>
	//! @param  lpszDefExt  コレはとりあえず lpszFileName に合わせた拡張子（ドットなし）にしておけばよい。<br>
	//! @param  bVistaStyle  Vista スタイルのダイアログを使う場合、WINVER >= 0x0600 となるように定義しておかないかぎり、自動更新が動作しないので注意。<br>
	CMyAutoFileExtUpdateDialog(BOOL bOpenFileDialog,
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0,
		//BOOL bVistaStyle = false
		BOOL bVistaStyle = true
		)
		: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName
		, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
		, m_hasOnTypeChangeBeenCalled()
	{}

	virtual ~CMyAutoFileExtUpdateDialog()
	{}

	virtual void OnInitDone() override
	{
		// Windows Vista スタイルのダイアログでは、OnInitDone() はサポートされないらしい。
		// フレームワークによってコールされることもないらしい。
		// http://msdn.microsoft.com/ja-jp/library/77x9ftyf%28v=vs.90%29.aspx

		this->OnTypeChange();
		__super::OnInitDone();
	}

	virtual void OnTypeChange() override;
};
