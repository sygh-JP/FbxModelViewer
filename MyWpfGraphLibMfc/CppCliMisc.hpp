#pragma once

//#include <vcclr.h>
//#using <mscorlib.dll>

//! @brief  Nullable やその他マネージ型とのやりとりを簡易化する C++/CLI ヘルパー用の名前空間。<br>
namespace CppCliMisc
{
	// C# だと "??" 演算子で簡単に書ける null 許容型の判定だが、C++/CLI だとかなり冗長で面倒。
	// 関数テンプレートなどでヘルパーを作ったほうがよさそう。
	inline bool CheckNullableBoolIsTrue(System::Nullable<bool> result)
	{
		// Equals(true) とはしない。
		return (result.HasValue && !result.Equals(false));
	}

	inline System::String^ ToCliString(LPCWSTR pStr)
	{
		return gcnew System::String(pStr);
	}

	inline System::String^ ToCliString(LPCSTR pStr)
	{
		return gcnew System::String(pStr);
	}

	inline System::Globalization::CultureInfo^ CreateCultureInfoByName(LPCWSTR pCultureName)
	{
		// リテラル文字列は暗黙変換により直接コンストラクタに渡せるが、ポインタは直接渡せないのでラッパーを作る。
		return gcnew System::Globalization::CultureInfo(gcnew System::String(pCultureName));
	}

	inline System::IntPtr ConvertHwndToIntPtr(HWND hwnd)
	{ return static_cast<System::IntPtr>(hwnd); }

	inline HWND ConvertIntPtrToHwnd(System::IntPtr hwnd)
	{ return static_cast<HWND>(hwnd.ToPointer()); }

#if 0
	inline bool CheckWpfModalDialogIsOK(System::Windows::Window^ modalDialog)
	{
		//System::Diagnostics::Debug::Assert(modalDialog->Owner != nullptr);
		// --> たとえ WindowInteropHelper::Owner 経由でネイティブ Win32 ウィンドウがオーナーとして設定されていても、
		// WPF の Window::Owner は nullptr のままであることはありえる。
		return CheckNullableBoolIsTrue(modalDialog->ShowDialog());
	}
#endif
} // end of namespace
