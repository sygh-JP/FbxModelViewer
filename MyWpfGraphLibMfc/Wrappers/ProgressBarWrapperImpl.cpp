#include "stdafx.h"
#include "../stdafx.h" // インテリセンス用の冗長インクルード。
//#include "../CppCliInterop.hpp"
#include "../CppCliMisc.hpp"
#include "HwndSrcHolderBase.hpp"
#include "ProgressBarWrapper.hpp"

#include "../../SharedUtility/PublicInclude/DebugNew.h"


namespace
{
	//! @brief  デリゲート メソッドを定義するマネージ クラス。<br>
	private ref class ProgressBarThreadInvoker abstract sealed
	{
	public:
		static void OnSetValue(System::Windows::Controls::ProgressBar^ progressBar, double value)
		{
			progressBar->Value = value;
		}
	};


	//! @brief  WPF コントロールをラップする混合クラス。非公開。<br>
	class MyProgressBarWrapperImpl
		: public MyWpfGraphLibWrapper::HwndSourceHolderBase
		, public MyWpfGraphLibWrapper::MiscControls::IMyProgressBarWrapper
	{
	private:
		bool m_isWpfCtrlInitialized;
		HWND m_hWndWpfHost;
		gcroot<System::Windows::Controls::ProgressBar^> m_gchWpfUserCtrl;

	private:
		void SetPositiveSize(int width, int height)
		{
			m_gchWpfUserCtrl->Width = width > 0 ? width : 1;
			m_gchWpfUserCtrl->Height = height > 0 ? height : 1;
		}

	public:
		MyProgressBarWrapperImpl(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
			: MyWpfGraphLibWrapper::HwndSourceHolderBase(hParent, x, y, width, height, pName)
			, m_isWpfCtrlInitialized()
			, m_hWndWpfHost()
		{
			m_gchWpfUserCtrl = gcnew System::Windows::Controls::ProgressBar();
			System::Diagnostics::Debug::Assert(width > 0 && height > 0);
			this->SetPositiveSize(width, height);
			this->GetHwndSource()->SizeToContent = System::Windows::SizeToContent::WidthAndHeight;
			this->GetHwndSource()->RootVisual = m_gchWpfUserCtrl;
			m_hWndWpfHost = static_cast<HWND>(this->GetHwndSource()->Handle.ToPointer());
			this->GetHwndSource()->AddHook(gcnew System::Windows::Interop::HwndSourceHook(MyWpfGraphLibWrapper::ChildHwndSourceHook));

			m_isWpfCtrlInitialized = true;
		}

		~MyProgressBarWrapperImpl()
		{
			// イベント リスナーの解除。
			//this->SetNativeEventListener(nullptr);
		}

		virtual bool GetIsKeyboardFocusWithin() const override
		{
			return m_isWpfCtrlInitialized && m_gchWpfUserCtrl->IsKeyboardFocusWithin;
		}

		virtual bool DoTranslateAccelerator(_Inout_ MSG* pMsg) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return MyWpfGraphLibWrapper::DoTranslateAcceleratorImpl(this->GetHwndSource(), this->GetIsKeyboardFocusWithin(), pMsg);
		}

		virtual void SetFocusToControl() override
		{
			if (m_isWpfCtrlInitialized)
			{
				m_gchWpfUserCtrl->Focus();
			}
		}

		virtual bool SetSize(int width, int height) override
		{
			if (m_isWpfCtrlInitialized)
			{
				this->SetPositiveSize(width, height);
				return true;
			}
			else
			{
				return false;
			}
		}

		virtual bool GetVisibility() const override
		{
			if (m_isWpfCtrlInitialized)
			{
				return m_gchWpfUserCtrl->Visibility == System::Windows::Visibility::Visible;
			}
			return false;
		}

		virtual void SetVisibility(bool isVisible) override
		{
			if (m_isWpfCtrlInitialized)
			{
				m_gchWpfUserCtrl->Visibility = isVisible ? System::Windows::Visibility::Visible : System::Windows::Visibility::Collapsed;
			}
		}

		virtual double GetValue() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->Value;
		}

		virtual void SetValue(double value) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->Value = value;
		}

		virtual void InvokeSetValue(double value) override
		{
			// C# 3.0 以降ではラムダ式が使えるので、わざわざデリゲート用のクラスを定義する必要はないが、VS 2008 の C++/CLI にはラムダが存在しない。
			// VS 2010 以降では gcroot を介することで、C++/CLI でも C++0x (C++11) のラムダを活用することができるが、
			// C++/CLI のラムダはあくまでネイティブ C++ のものなので、イベント ハンドラーなどのマネージ デリゲート用には直接使えない。
			m_gchWpfUserCtrl->Dispatcher->Invoke(
				gcnew System::Action<System::Windows::Controls::ProgressBar^, double>(ProgressBarThreadInvoker::OnSetValue),
				m_gchWpfUserCtrl, value);
		}

		virtual void Refresh() override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			MyMiscHelpers::MyThreadHelper::Refresh(m_gchWpfUserCtrl);
		}

		virtual double GetMinimum() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->Minimum;
		}

		virtual void SetMinimum(double value) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->Minimum = value;
		}

		virtual double GetMaximum() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->Maximum;
		}

		virtual void SetMaximum(double value) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->Maximum = value;
		}

		virtual bool GetIsIndeterminate() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->IsIndeterminate;
		}

		virtual void SetIsIndeterminate(bool isIndeterminate) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->IsIndeterminate = isIndeterminate;
		}
	};

} // end of namespace


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		IMyProgressBarWrapper::TSharedPtr IMyProgressBarWrapper::Create(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
		{
			try
			{
				return IMyProgressBarWrapper::TSharedPtr(new MyProgressBarWrapperImpl(hParent, x, y, width, height, pName));
			}
			catch (...)
			{
			}
			return IMyProgressBarWrapper::TSharedPtr();
		}

	} // end of namespace
} // end of namespace
