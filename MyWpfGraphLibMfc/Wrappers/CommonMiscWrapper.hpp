//! @file
//! @brief  ネイティブ C++ 用グローバル ヘルパー メソッドのラッパーインターフェイスを提供するヘッダー。<br>

#pragma once


namespace MyWpfGraphLibWrapper
{
	namespace CommonMisc
	{
		extern AFX_EXT_API void ExecuteGarbageCollect();

		extern AFX_EXT_API void SetWpfResourcesCulture(CStringW strCultureName);

		extern AFX_EXT_API void DoWpfEvents();

		extern AFX_EXT_API bool GetCurrentKeyboardFocusedWpfControlExists();
	}
} // end of namespace
