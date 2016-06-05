#pragma once

// C++/CLI 専用。


namespace MyWpfGraphLibWrapper
{

	class HwndSourceHolderBase abstract
	{
	private:
		gcroot<System::Windows::Interop::HwndSource^> m_gchWndSource;

	protected:
		virtual ~HwndSourceHolderBase() {}

	protected:
		HwndSourceHolderBase(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
		{
			using System::Windows::Interop::HwndSource;
			using System::Windows::Interop::HwndSourceParameters;

			HwndSourceParameters^ gchSourceParams = gcnew HwndSourceParameters(gcnew System::String(pName));
			gchSourceParams->PositionX    = x;
			gchSourceParams->PositionY    = y;
			gchSourceParams->Width        = width;
			gchSourceParams->Height       = height;
			gchSourceParams->ParentWindow = System::IntPtr(hParent);
			gchSourceParams->WindowStyle  = WS_VISIBLE | WS_CHILD;

			m_gchWndSource = gcnew HwndSource(*gchSourceParams);
		}

	public:
		System::Windows::Interop::HwndSource^ GetHwndSource() { return m_gchWndSource; }
	};


	extern System::IntPtr ChildHwndSourceHook(System::IntPtr hwnd, int msg, System::IntPtr wParam, System::IntPtr lParam, bool% handled);

	extern bool CallTranslateAccelerator(System::Windows::Interop::IKeyboardInputSink^ sink, System::Windows::Interop::MSG% msg, System::Windows::Input::ModifierKeys modifiers);

	extern bool DoTranslateAcceleratorImpl(System::Windows::Interop::IKeyboardInputSink^ sink, bool isKeyboardFocusWithin, _Inout_ MSG* pMsg);

} // end of namespace
