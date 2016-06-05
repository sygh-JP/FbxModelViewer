#include "stdafx.h"
#include "../stdafx.h" // インテリセンス用の冗長インクルード。
//#include "../CppCliInterop.hpp"
#include "../CppCliMisc.hpp"
#include "HwndSrcHolderBase.hpp"
#include "WebBrowserWrapper.hpp"

#include "../../SharedUtility/PublicInclude/DebugNew.h"


namespace
{
	//! @brief  WPF コントロールをラップする混合クラス。非公開。<br>
	class MyWebBrowserWrapperImpl
		: public MyWpfGraphLibWrapper::HwndSourceHolderBase
		, public MyWpfGraphLibWrapper::MiscControls::IMyWebBrowserWrapper
	{
	private:
		bool m_isWpfCtrlInitialized;
		HWND m_hWndWpfHost;
		gcroot<System::Windows::Controls::WebBrowser^> m_gchWpfUserCtrl;

	private:
		void SetPositiveSize(int width, int height)
		{
			m_gchWpfUserCtrl->Width = width > 0 ? width : 1;
			m_gchWpfUserCtrl->Height = height > 0 ? height : 1;
		}

	public:
		MyWebBrowserWrapperImpl(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
			: MyWpfGraphLibWrapper::HwndSourceHolderBase(hParent, x, y, width, height, pName)
			, m_isWpfCtrlInitialized()
			, m_hWndWpfHost()
		{
			m_gchWpfUserCtrl = gcnew System::Windows::Controls::WebBrowser();
			System::Diagnostics::Debug::Assert(width > 0 && height > 0);
			this->SetPositiveSize(width, height);
			this->GetHwndSource()->SizeToContent = System::Windows::SizeToContent::WidthAndHeight;
			this->GetHwndSource()->RootVisual = m_gchWpfUserCtrl;
			m_hWndWpfHost = static_cast<HWND>(this->GetHwndSource()->Handle.ToPointer());
			this->GetHwndSource()->AddHook(gcnew System::Windows::Interop::HwndSourceHook(MyWpfGraphLibWrapper::ChildHwndSourceHook));

			m_isWpfCtrlInitialized = true;
		}

		~MyWebBrowserWrapperImpl()
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
			// WebBrowser は他の UserControl 派生クラスと違って、IsKeyboardFocusWithin で判定してはダメ。
			// Enter キーや Tab キー、カーソル キーが効かなくなる。
			return MyWpfGraphLibWrapper::DoTranslateAcceleratorImpl(this->GetHwndSource(), this->GetVisibility(), pMsg);
		}

		virtual void SetFocusToControl() override
		{
			if (m_isWpfCtrlInitialized)
			{
				m_gchWpfUserCtrl->Focus();
			}
		}

		virtual void ChangeCulture(CStringW strCultureName) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			//m_gchWpfUserCtrl->ChangeCulture(CppCliMisc::CreateCultureInfoByName(strCultureName));
		}

		virtual bool OnSize(UINT sizingType, int width, int height) override
		{
			if (m_isWpfCtrlInitialized && sizingType == SIZE_RESTORED)
			{
				this->SetPositiveSize(width, height);
				return true;
			}
			else
			{
				return false;
			}
		}

		virtual bool SetSourceUri(CStringW strUri) override
		{
			if (m_isWpfCtrlInitialized)
			{
				try
				{
					m_gchWpfUserCtrl->Source = gcnew System::Uri(gcnew System::String(strUri));
					return true;
				}
				catch (...)
				{
				}
			}
			return false;
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
	};

} // end of namespace


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		IMyWebBrowserWrapper::TSharedPtr IMyWebBrowserWrapper::Create(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
		{
			try
			{
				return IMyWebBrowserWrapper::TSharedPtr(new MyWebBrowserWrapperImpl(hParent, x, y, width, height, pName));
			}
			catch (...)
			{
			}
			return IMyWebBrowserWrapper::TSharedPtr();
		}

	} // end of namespace
} // end of namespace
