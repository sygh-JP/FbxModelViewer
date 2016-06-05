#pragma once


namespace MyDesktopHelpers
{

#pragma region // MFC ツリー コントロール関連のヘルパー関数。 //
	//! @brief  再帰的に探索して全ての子ノードを展開あるいは折りたたむ。<br>
	extern void ExpandAllChildren(CTreeCtrl& treeCtrl, HTREEITEM hItem, bool expands = true);

	inline void ExpandAll(CTreeCtrl& treeCtrl, bool expands = true)
	{
		// CTreeCtrl::GetRootItem() の戻り値と TVI_ROOT は同じではない。
		// TVI_ROOT に複数の子アイテムを追加していた場合、GetRootItem() では対処できないが、
		// TVI_ROOT を使ってしまうと、CTreeCtrl::ItemHasChildren() の呼び出し中に
		// AfxUnlockGlobals() の LeaveCriticalSection() 呼び出しのあたりで
		// アクセス違反（0xC0000005 例外）が発生する模様（ただしアプリはクラッシュしない）。
		treeCtrl.Expand(treeCtrl.GetRootItem(), expands ? TVE_EXPAND : TVE_COLLAPSE); // ルートは別途展開。
		ExpandAllChildren(treeCtrl, treeCtrl.GetRootItem(), expands);
	}
#pragma endregion

#pragma region // MFC エディット コントロール関連のヘルパー関数。CEdit もしくは CRichEditCtrl に対して使用可能。 //
	//! @brief  エディット コントロールをスクロールさせて最終行に移動する。<br>
	//! 
	//! マルチラインのエディット コントロール用。<br>
	template<typename TEdit> void ScrollEditCtrlToLastLine(TEdit& editCtrl)
	{
		editCtrl.LineScroll(editCtrl.GetLineCount());
	}

	//! @brief  エディット コントロールのキャレットを末尾に移動する。<br>
	template<typename TEdit> void SetEditCtrlCaretToEnd(TEdit& editCtrl)
	{
		const int len = editCtrl.GetWindowTextLength();
		editCtrl.SetSel(len, len);
	}

	//! @brief  エディット コントロールの末尾に文字列を追加挿入する。<br>
	template<typename TEdit> void AddStringToEditCtrl(TEdit& editCtrl, LPCTSTR pString)
	{
		SetEditCtrlCaretToEnd(editCtrl);
		editCtrl.ReplaceSel(pString);
	}

	template<typename TEdit> void AddStringLineToEditCtrl(TEdit& editCtrl, LPCTSTR pString)
	{
		CString str(pString);
		str += _T("\r\n");
		AddStringToEditCtrl(editCtrl, str);
	}

	template<typename TEdit> void AddStringToEditCtrlAndScrollToLastLine(TEdit& editCtrl, LPCTSTR pString)
	{
		AddStringToEditCtrl(editCtrl, pString);
		ScrollEditCtrlToLastLine(editCtrl);
		SetEditCtrlCaretToEnd(editCtrl);
	}

	template<typename TEdit> void AddStringLineToEditCtrlAndScrollToLastLine(TEdit& editCtrl, LPCTSTR pString)
	{
		CString str(pString);
		str += _T("\r\n");
		AddStringToEditCtrlAndScrollToLastLine(editCtrl, str);
	}
#pragma endregion
} // end of namespace
