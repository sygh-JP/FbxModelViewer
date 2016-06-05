#include "stdafx.h"
#include "AutoFileExtUpdateDlg.h"
//#include <dlgs.h> // cmb13

#include "DebugNew.h"


// なお、Debug ビルドで IDE からではなくエクスプローラーから実行して、
// 拡張子部分を入力せずに「保存」を押すと、「ファイル名は有効ではありません。」と表示されてしまう。


void CMyAutoFileExtUpdateDialog::OnTypeChange()
{
	// インデックス値から現在の拡張子フィルタ文字列を取得する。
	LPCTSTR ptr = m_ofn.lpstrFilter;
	int nTarget = ((m_ofn.nFilterIndex - 1) * 2) + 1;
	while (nTarget--)
	{
		while (*(ptr++));
	}
	ATLTRACE(_T(__FUNCTION__) _T("(): Filter[%d]: %s\n"), m_ofn.nFilterIndex, ptr);
	CString strExt(ptr);
	strExt.Remove(' '); // 半角スペースがもし含まれていれば除去する。
	strExt.Replace(';', '\0'); // 複数の拡張子がセミコロンで区切られている場合、最初の拡張子を利用する（セミコロン以前を有効な文字列とする）。

	// 拡張子フィルタから拡張子のボディを取得する。"*.txt" の場合 "*." を除去して "txt" を取得する。"*.*" の場合、空の文字列になる。
	CString left;
	left = strExt.Left(1);
	if (left == _T("*"))
	{
		strExt = strExt.TrimLeft(_T("*"));
	}
	left = strExt.Left(1);
	if (left == _T("."))
	{
		strExt = strExt.TrimLeft(_T("."));
	}
	left = strExt.Left(1);
	if (left == _T("*"))
	{
		strExt = strExt.TrimLeft(_T("*"));
	}

#if (0x0900 <= _MFC_VER) && (_MFC_VER < 0x0B00)
	// MFC 9.0 / 10.0 のバグというか CFileDialog::SetDefExt() 関数の引数の定義ミス。
	// 本来 LPCTSTR (LPCWSTR for UNICODE / LPCSTR for MBCS) とすべきところを LPCSTR (MBCS) としてしまっている。
	// なお、MFC 9.0 / 10.0 の実装では、OS が Vista 以降でなおかつ Vista スタイルの場合は、
	// 内部で CStringW 変数で受け取って変換しているが、OS が XP 以前もしくは非 Vista スタイルの場合は、
	// 受け取ったポインタを SendMessage() に渡しているので、
	// プロジェクト設定で Unicode 文字セットを使用するようにしている場合、
	// 非 Vista スタイルで素直に LPCSTR を渡してしまうと文字化けが発生する。
	// 逆に Vista スタイルの場合、LPCWSTR を LPCSTR に強制キャストして渡してしまうと
	// CStringW の LPCSTR を受け取るコンストラクタが誤認するので、文字化けが発生する。
	// 実際には、単に afxdlgs.h および dlgfile.cpp の関数インターフェイスが間違っているだけなので、
	// 非 Vista スタイルであれば LPCTSTR を無理やり LPCSTR にキャストして渡してコンパイラを
	// だましてしまえば一応正常に動作する模様。
	// ただし、Vista スタイルを使う場合は絶対に SetDefExt() に LPCWSTR を使うことはできない。
	// Visual Studio 2012 の MFC 11.0 ではこの問題は修正されている。
#pragma message("MFC 9.0 / 10.0 CFileDialog::SetDefExt()")
	if (m_bVistaStyle)
	{
		this->SetDefExt(CStringA(strExt));
	}
	else
	{
		// ココで素直に引数どおりに LPCSTR を渡してしまうと、Unicode ビルドでは文字化けが発生する。
		//this->SetDefExt(CStringA(strExt)); // NG.
		this->SetDefExt(reinterpret_cast<LPCSTR>(static_cast<LPCTSTR>(strExt)));
	}
#else
	this->SetDefExt(strExt);
#endif

	__super::OnTypeChange();

	// コンストラクタの lpszFileName パラメータに "*.<lpszDefExt>" や "XXX.<lpszDefExt>" 以外の形式の文字列をセットし、
	// 初回の OnTypeChange() 呼び出しで IFileDialog::SetFileName() をコールすると、
	// CFileDialog::DoModal() 呼び出しの際、_AfxActivationWndProc() の内部、
	// CallWindowProc() 呼び出しの箇所で Win32 アクセス違反例外（0xC0000005）が発生する。
	// なお、初回の OnTypeChange() では CFileDialog::GetFileName() は常に空文字列を返す模様。
	// CallWindowProc() API 内部で Win32 例外はハンドリングされているようなのでクラッシュはしないようだが、
	// 不要な例外発生を防ぐため、初回は SetFileName() をコールしないようにする。
	// つまり、ダイアログが表示される最初のタイミングでは、拡張子の自動変更は機能しないため、
	// lpszFileName パラメータは空文字列にするか、もしくは lpszDefExt に合った拡張子にしておく必要がある。
	// ちなみに Vista スタイルのファイル ダイアログを使うと、
	// RPC 例外（0x000006BA: RPC サーバーを利用できません。）が毎回発生するが、
	// これも API 内部でハンドリングされているため、プログラムの実行自体には問題ない模様。
	// Win32 例外でブレークする設定にしているとデバッグが中断するので本当はなんとかしたいが……
	if (!m_hasOnTypeChangeBeenCalled)
	{
		m_hasOnTypeChangeBeenCalled = true;
		return;
	}

	if (this->GetOFN().Flags & OFN_ALLOWMULTISELECT)
	{
		// マルチセレクトには現時点で非対応。
		ATLTRACE("Not Implemented!!\n");
	}
	else
	{
		// マルチセレクトでない場合。
		const CString strOriginalFileName = this->GetFileName();
		CPath newName(strOriginalFileName);
		if (!strExt.IsEmpty())
		{
			if (newName.m_strPath.IsEmpty())
			{
				newName = _T("*.*");
			}

			if (newName.FindFileName() != -1)
			{
				newName.RenameExtension(_T(".") + strExt);
			}
		}
		// 選択された拡張子に応じて、ファイル ダイアログのファイル名入力欄（オートコンプリート コンボボックス）を更新する。
		if (m_bVistaStyle)
		{
			// CFileDialog::SetControlText() は Vista スタイルのダイアログでうまく動作しない模様（ただし MSDN にはその旨の記載がない）。
			// 内部で IFileDialogCustomize::SetControlLabel() が失敗して E_INVALIDARG が返る。
			// 結果として ENSURE() が失敗する羽目になる。
			// 代わりに IFileDialog::SetFileName() を使えばいいらしい。ただし COM オブジェクトなので参照カウントに注意。

			//ATLTRACE("CFileDialog::SetControlText() seems to fail on Vista-style dialog...\n");
			//this->SetControlText(cmb13, newName); // NG.
#if (WINVER >= 0x0600)
			IFileDialog* pVistaFileDialog = NULL;
			if (m_bOpenFileDialog)
			{
				pVistaFileDialog = this->GetIFileOpenDialog();
			}
			else
			{
				pVistaFileDialog = this->GetIFileSaveDialog();
			}
			_ASSERTE(pVistaFileDialog);
			pVistaFileDialog->SetFileName(CStringW(newName));
			pVistaFileDialog->Release();
#endif
			// IFileDialog は Vista 以降でサポートされている。
			// なお、WINVER >= 0x0600 となるように定義していなくても、COM は基本的に遅延バインディングなので、
			// MFC CFileDialog によるラッパーを使えば、Vista 以降の OS で Vista スタイルのダイアログを使うことができるが、
			// やはり本来は相応の WINVER をきちんと定義するべき。そのほうがコーディングの難易度も下がる。
			// なお、開発環境・ターゲット環境が完全に Vista 以降となるまでは、Vista 以降のみに対応した各種 API を使う場合、
			// 遅延ロード・遅延バインディングを駆使しないといけない。
		}
		else
		{
			// コレ、ひどい名前のマクロ定数だと思わないか？　だから Win32 API や MFC はダメなんだよ……
			this->SetControlText(cmb13, newName);
		}
	}
}
