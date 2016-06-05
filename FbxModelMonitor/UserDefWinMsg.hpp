#pragma once

#include "MyDesktopHelpers.hpp"

enum
{
	UDWM_INVOKE_SIMPLE_DELEGATE_BY_UI_THREAD = WM_APP + 1,
};

namespace MyAppHelpers
{
	inline LRESULT InvokeWith(HWND hWnd, const MyDesktopHelpers::MyWin32DelegateWrapper::TSimpleDelegate& func)
	{
		if (!::IsWindow(hWnd))
		{
			_ASSERTE(false);
			return 0;
		}
		return MyDesktopHelpers::MyWin32DelegateWrapper::Invoke(hWnd, UDWM_INVOKE_SIMPLE_DELEGATE_BY_UI_THREAD, func);
	}

	// AfxGetMainWnd() は MFC でないスレッドから呼び出すと NULL を返すらしい？
}
