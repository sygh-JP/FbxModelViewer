#include "stdafx.h"
#include "MyProcessHelpers.hpp"

namespace MyUtils
{
	bool OpenIncludeDirByWindowsExplorer(LPCTSTR pTargetPath)
	{
		if (::PathFileExists(pTargetPath))
		{
			CString strCmdLine;
			strCmdLine.Format(_T("explorer.exe /n,/e,/select,\"%s\""), pTargetPath);
			STARTUPINFO si = {};
			PROCESS_INFORMATION pi = {};
			si.cb = sizeof(si);
			const BOOL isSuccess = ::CreateProcess(nullptr, strCmdLine.GetBuffer(strCmdLine.GetLength()), nullptr, nullptr, true, 0, nullptr, nullptr, &si, &pi);
			strCmdLine.ReleaseBuffer();
			return !!isSuccess;
		}
		else
		{
			return false;
		}
	}
}
