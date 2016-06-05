#include "stdafx.h"
#include "MyMfcHelpers.hpp"

#include "DebugNew.h"


namespace MyDesktopHelpers
{
	void ExpandAllChildren(CTreeCtrl& treeCtrl, HTREEITEM hItem, bool expands)
	{
		if (treeCtrl.ItemHasChildren(hItem))
		{
			HTREEITEM hChild = treeCtrl.GetChildItem(hItem); // 最初の子を取得。

			// 全ての子に対して処理する。
			while (hChild != nullptr)
			{
				treeCtrl.Expand(hChild, expands ? TVE_EXPAND : TVE_COLLAPSE);
				ExpandAllChildren(treeCtrl, hChild, expands); // 再帰処理。
				hChild = treeCtrl.GetNextItem(hChild, TVGN_NEXT); // 兄弟を取得。
			}
		}
	}

} // end of namespace
