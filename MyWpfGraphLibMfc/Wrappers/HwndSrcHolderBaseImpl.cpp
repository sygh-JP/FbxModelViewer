#include "stdafx.h"
#include "../stdafx.h" // インテリセンス用の冗長インクルード。
//#include "../CppCliInterop.hpp"
#include "../CppCliMisc.hpp"
#include "HwndSrcHolderBase.hpp"

#include "../../SharedUtility/PublicInclude/DebugNew.h"


namespace MyWpfGraphLibWrapper
{
	//! @brief  CWnd::IsDialogMessage() で WM_CHAR メッセージが食われてしまい、WPF 側に通知されないことへの対処。<br>
	//! cf. http://dev-onethatdevelops.blogspot.jp/2007/10/wpf-win32-interop-with-hwndsource.html <br>
	System::IntPtr ChildHwndSourceHook(System::IntPtr hwnd, int msg, System::IntPtr wParam, System::IntPtr lParam, bool% handled)
	{
		if (msg == WM_GETDLGCODE)
		{
			handled = true;
			return System::IntPtr(DLGC_WANTCHARS);
		}
		return System::IntPtr(0);
	}

	bool DoTranslateAcceleratorImpl(System::Windows::Interop::IKeyboardInputSink^ sink, bool isKeyboardFocusWithin, _Inout_ MSG* pMsg)
	{
		if (!isKeyboardFocusWithin)
		{
			return false;
		}

		// キーボード シンク経由のメッセージ処理カスタマイズ。
		switch (pMsg->message)
		{
			//case WM_KEYUP:
		case WM_KEYDOWN:
			//case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			// MFC アプリケーションでホストした WPF ウィンドウ上の WindowsFormsHost 内コントロールに Focus ON の状態で
			// モーダル メッセージ ボックスを出すとハング アップする。
			// Windows Forms の NumericUpDown の上下キーによる操作をそのまま処理させる場合は、ここでフィルターをかける。
			// が、そうすると WPF の ComboBox の上下キーによる選択変更のほうは、うまくいかなくなる。
			// Win32 - WPF 間のフォーカス移動になってしまう。
			// そういった問題点もあるので、ここでは WindowsFormsHost が WPF コントロール内に含まれることを想定していない。
			// WPF 経由の Windows Forms 相互運用はあきらめて、Extended WPF Toolkit で代用すること。

			//if (pMsg->wParam != VK_UP && pMsg->wParam != VK_DOWN)
			{
				//System::Windows::Interop::HwndSource^ source = pHolder->GetHwndSource();
				//System::Windows::Interop::IKeyboardInputSink^ sink = source;
				return CallTranslateAccelerator(sink, *reinterpret_cast<System::Windows::Interop::MSG*>(pMsg),
					System::Windows::Input::Keyboard::Modifiers);
			}
			break;
		default:
			break;
		}
		return false;
	}
} // end of namespace


// System::Windows::Interop::IKeyboardInputSink::TranslateAccelerator() を使う際、
// Win32 API でマクロ定義されている名前とカブるらしく、undef しないとコンパイルが通らない。
// このため、このブロック以降には Win32 API のほうの TranslateAccelerator を使うコードを一切記述しないようにすること。
#ifdef TranslateAccelerator
#undef TranslateAccelerator
#endif

namespace MyWpfGraphLibWrapper
{
	bool CallTranslateAccelerator(System::Windows::Interop::IKeyboardInputSink^ sink, System::Windows::Interop::MSG% msg, System::Windows::Input::ModifierKeys modifiers)
	{
		return sink->TranslateAccelerator(msg, modifiers);
	}

} // end of namespace
