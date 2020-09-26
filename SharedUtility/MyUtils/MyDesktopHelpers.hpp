#pragma once

#include "MyNoncopyable.hpp"


namespace MyDesktopHelpers
{
	// TODO: MFC およびデスクトップ アプリ関連の ATL ヘルパーは分離する。Windows ストア アプリでは実行プログラムのパスも取得できない。
	// コマンドライン引数とかも NG。CStringW は使えるが、CStringA は使えない。


	inline void GetModuleFilePath(CStringW& strPath)
	{
		// __wargv, __argv はアプリケーション エントリ ポイントの引数による（プロジェクト設定に左右される）ので、直接使わない。
		// UNICODE アプリケーションでは __argv を使うことができないし、MBCS アプリケーションでは __wargv を使うことができない。
		strPath = __targv[0];
	}

	inline void GetModuleFilePath(CStringA& strPath)
	{
		strPath = __targv[0];
	}

	inline void GetModuleFilePath(_In_opt_ HMODULE hModule, CString& strPath)
	{
		TCHAR tempFileName[MAX_PATH] = {};
		::GetModuleFileName(hModule, tempFileName, ARRAYSIZE(tempFileName));
		strPath = tempFileName;
	}

	template<typename T> bool GetModuleDirPathImpl(ATL::CPathT<T>& dirPath)
	{
		GetModuleFilePath(dirPath.m_strPath);
		return !!dirPath.RemoveFileSpec();
	}

	inline bool GetModuleDirPath(CPathW& dirPath)
	{
		return GetModuleDirPathImpl(dirPath);
	}

	inline bool GetModuleDirPath(CPathA& dirPath)
	{
		return GetModuleDirPathImpl(dirPath);
	}

	// メインスレッドに処理を委譲するため、カスタム メッセージと std::function を利用する。
	// System.Windows.Forms.Control.Invoke() や、System.Windows.Threading.Dispatcher.Invoke() に相当する。
	class MyWin32DelegateWrapper : MyUtils::MyNoncopyable<MyWin32DelegateWrapper>
	{
	private:
		MyWin32DelegateWrapper() = delete;
	public:
		typedef std::function<LRESULT()> TSimpleDelegate;
	private:
		TSimpleDelegate m_func;
	public:
		MyWin32DelegateWrapper(const TSimpleDelegate& func)
			: m_func(func)
		{}
	public:
		static LRESULT Invoke(HWND hWnd, UINT msgId, const TSimpleDelegate& func)
		{
			_ASSERTE(::IsWindow(hWnd));

			// アドレス渡しなので、Post ではダメ。
			return ::SendMessage(hWnd, msgId,
				reinterpret_cast<WPARAM>(&MyWin32DelegateWrapper(func)), 0);
		}
	public:
		static LRESULT OnInvoke(WPARAM wParam, LPARAM lParam)
		{
			auto* pMyDelegate = reinterpret_cast<MyWin32DelegateWrapper*>(wParam);
			_ASSERTE(pMyDelegate);
			if (pMyDelegate && pMyDelegate->m_func)
			{
				return pMyDelegate->m_func();
			}
			return 0;
		}
	};
} // end of namespace
