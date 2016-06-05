#include "stdafx.h"
#include "../stdafx.h" // インテリセンス用の冗長インクルード。
//#include "../CppCliInterop.hpp"
#include "../CppCliMisc.hpp"
#include "TaskProgressDialogWrapper.hpp"

#include "../../SharedUtility/PublicInclude/DebugNew.h"


// C++/CLI では interface はキーワードとなっているが、combaseapi.h にて interface が struct のエイリアスとしてマクロ定義されてある。
// C++/CLI において interface を本来の意味で使う場合は、おそらく適宜 #undef によるエイリアスの解除をいったん行なうことが必要となる？
// ……と思ったが、interface class というコンテキスト キーワードなので問題ないらしい？

// マネージ クラスにはネイティブの非 POD 型を直接含めることはできない。生ポインタであれば OK。
// ただしマネージ クラスでネイティブ オブジェクトの寿命管理をさせるのは楽ではない。
// マネージ クラスでネイティブ オブジェクトを管理する場合は、必ずデストラクタとファイナライザを定義すること。
// なお、C++/CLI のマネージ クラスでデストラクタを定義すると、IDisposable を実装したことになるらしい。

// ネイティブ クラスにはマネージの GC ハンドルを直接含めることはできない。gcroot でラップする。

// C++/CLI はマネージ ラムダをサポートしない。
// ネイティブ クラスにはマネージ ハンドルを含められないことから、ネイティブ ラムダではマネージ ハンドルのキャプチャもできない。
// ネイティブ ラムダからマネージ デリゲートに明示的に変換するマネージ ラッパークラスを書かなければならないため、C# と比べて非常に冗長になる。
// http://www.codeproject.com/Articles/277612/Using-lambdas-Cplusplus-vs-Csharp-vs-Cplusplus-CX


namespace
{

	//! @brief  マネージ コールバックをネイティブ コールバックに変換するためのアダプター用マネージ クラス。非公開。<br>
	private ref class MySimpleManagedCallback sealed
	{
	public:
		using TCallbackFunc = std::function<void()>;
	private:
		// HACK: ポインタではなくコピーを使いたいところ。
		// ただしマネージ型には（プリミティブを除いて）ネイティブ型を含めることはできない。
		// new/delete を明示的に使用する方法もある。
		const TCallbackFunc* m_callbackFunc;
	public:
		MySimpleManagedCallback()
			: m_callbackFunc()
		{}
		MySimpleManagedCallback(const TCallbackFunc* func)
			: m_callbackFunc(func)
		{}
		void OnCall()
		{
			if (m_callbackFunc && *m_callbackFunc)
			{
				(*m_callbackFunc)();
			}
		}
		void OnCall(System::Object^ sender, System::EventArgs^ e)
		{
			this->OnCall();
		}
		void OnCall(System::Threading::Tasks::Task^ task)
		{
			this->OnCall();
		}
	};


	//! @brief  WPF コントロールをラップする混合クラス。非公開。<br>
	class MyTaskProgressDialogWrapperImpl
		: public MyWpfGraphLibWrapper::MiscControls::IMyTaskProgressDialogWrapper
	{
	private:
		bool m_isWpfCtrlInitialized;
		gcroot<MyWpfGraphLibrary::MyTaskProgressDialog^> m_gchWpfUserCtrl;
		gcroot<System::Windows::Interop::WindowInteropHelper^> m_gchWinInteropHelper;
		TEventMainWorkStarted m_eventMainWorkStarted;

	public:
		MyTaskProgressDialogWrapperImpl(HWND hParent, LPCWSTR pName)
			: m_isWpfCtrlInitialized()
		{
			m_gchWpfUserCtrl = gcnew MyWpfGraphLibrary::MyTaskProgressDialog();

			m_gchWinInteropHelper = gcnew System::Windows::Interop::WindowInteropHelper(m_gchWpfUserCtrl);
			m_gchWinInteropHelper->Owner = CppCliMisc::ConvertHwndToIntPtr(hParent);

			m_isWpfCtrlInitialized = true;
		}

		~MyTaskProgressDialogWrapperImpl()
		{
			// 完全クローズ。
			this->EnforcedClose();
			// イベント リスナーの解除。
			this->SetNativeEventListener(nullptr);
		}

		virtual void SetNativeEventListener(MyWpfGraphLibWrapper::MiscControls::IMyTaskProgressDialogWrapper::INativeEventListener* pEventListener) override
		{
			//m_gchManagedEventListenerImpl->SetNativeEventListener(pEventListener);
		}

		virtual void ChangeCulture(CStringW strCultureName) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			//m_gchWpfUserCtrl->ChangeCulture(CppCliMisc::CreateCultureInfoByName(strCultureName));
		}

		virtual bool ShowModalDialog(/*HWND hWndOwner*/) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			HWND hParent = CppCliMisc::ConvertIntPtrToHwnd(m_gchWinInteropHelper->Owner);
			System::Diagnostics::Debug::Assert(!!::IsWindow(hParent));
			// TODO: タスクの開始コールバックと中断コールバックのバインド。
			// C++/CLI におけるインスタンス メソッドを使ったデリゲートのコンストラクトは、下記の C# と C++ のコード例の比較を参照のこと。
			// https://msdn.microsoft.com/en-us/library/system.action.aspx
			// クラス メソッドの場合は簡潔に書ける。
			// https://msdn.microsoft.com/en-us/library/system.eventhandler.aspx

			//auto eventAdapter = gcnew MySimpleManagedCallback(&m_eventMainWorkStarted);
#if 0
			auto bindedAction = gcnew System::Action(eventAdapter, &MySimpleManagedCallback::OnCall);
			m_gchWpfUserCtrl->MainWorkStarted += bindedAction;
			const auto retVal = CppCliMisc::CheckNullableBoolIsTrue(m_gchWpfUserCtrl->ShowDialog());
			m_gchWpfUserCtrl->MainWorkStarted -= bindedAction;
#elif 0
			auto bindedAction = gcnew System::EventHandler(eventAdapter, &MySimpleManagedCallback::OnCall);
			m_gchWpfUserCtrl->ContentRendered += bindedAction;
			const auto retVal = CppCliMisc::CheckNullableBoolIsTrue(m_gchWpfUserCtrl->ShowDialog());
			m_gchWpfUserCtrl->ContentRendered -= bindedAction;
#else
			// C++/CLI には async/await がないので、TPL を直接使う。
			const MySimpleManagedCallback::TCallbackFunc closeFunc = [=]() { m_gchWpfUserCtrl->EnforcedClose(); };
			const MySimpleManagedCallback::TCallbackFunc endTaskFunc = [=]() {
				m_gchWpfUserCtrl->Dispatcher->Invoke(gcnew System::Action(gcnew MySimpleManagedCallback(&closeFunc), &MySimpleManagedCallback::OnCall));
			};
			const MySimpleManagedCallback::TCallbackFunc runTaskFunc = [=]() {
				auto task = System::Threading::Tasks::Task::Factory->StartNew(gcnew System::Action(gcnew MySimpleManagedCallback(&m_eventMainWorkStarted), &MySimpleManagedCallback::OnCall));
				task->ContinueWith(gcnew System::Action<System::Threading::Tasks::Task^>(gcnew MySimpleManagedCallback(&endTaskFunc), &MySimpleManagedCallback::OnCall));
			};
			auto bindedAction = gcnew System::EventHandler(gcnew MySimpleManagedCallback(&runTaskFunc), &MySimpleManagedCallback::OnCall);
			m_gchWpfUserCtrl->ContentRendered += bindedAction;
			const auto retVal = CppCliMisc::CheckNullableBoolIsTrue(m_gchWpfUserCtrl->ShowDialog());
			m_gchWpfUserCtrl->ContentRendered -= bindedAction;
#endif
			bindedAction = nullptr;
			//eventAdapter = nullptr;
			return retVal;
		}

		virtual bool Show() override
		{
			if (m_isWpfCtrlInitialized)
			{
				try
				{
					// Window 表示中に Show() を呼ぶと InvalidOperationException 例外が発生する。
					m_gchWpfUserCtrl->Show();
					return true;
				}
				catch (...)
				{
				}
			}
			return false;
		}

		virtual bool Hide() override
		{
			if (m_isWpfCtrlInitialized)
			{
				try
				{
					// Window 非表示中に Hide() を呼ぶと InvalidOperationException 例外が発生する。
					m_gchWpfUserCtrl->Hide();
					return true;
				}
				catch (...)
				{
				}
			}
			return false;
		}

		virtual void EnforcedClose() override
		{
			if (m_isWpfCtrlInitialized)
			{
				try
				{
					// Window が一度も Show() されていない状態で Close() しても例外は発生しないらしい。
					m_gchWpfUserCtrl->EnforcedClose();
				}
				catch (...)
				{
				}
			}
		}

		virtual bool SetTopOn(HWND hWnd) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			const BOOL retVal = ::SetWindowPos(
				CppCliMisc::ConvertIntPtrToHwnd(m_gchWinInteropHelper->Handle),
				hWnd,
				-1, -1, -1, -1, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			return !!retVal;
		}

		virtual void SetIsEnabled(bool isEnabled) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->IsEnabled = isEnabled;
		}

		virtual void SetVisibility(bool isVisible) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			if (isVisible)
			{
				// レイヤード ウィンドウで Visibility を制御すると、アクティブ ウィンドウも変動する。HUD として使う場合は注意。
				if (m_gchWpfUserCtrl->Visibility != System::Windows::Visibility::Visible)
				{
					m_gchWpfUserCtrl->Visibility = System::Windows::Visibility::Visible;
				}
			}
			else
			{
				if (m_gchWpfUserCtrl->Visibility != System::Windows::Visibility::Collapsed)
				{
					m_gchWpfUserCtrl->Visibility = System::Windows::Visibility::Collapsed;
				}
			}
		}

		virtual bool GetVisibility() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->Visibility == System::Windows::Visibility::Visible;
		}

		virtual void SetOpacity(double opacity) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->Opacity = opacity;
		}

		virtual double GetOpacity() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->Opacity;
		}

		virtual void SetEventMainWorkStarted(const TEventMainWorkStarted& mainWorkStarted) override
		{
			m_eventMainWorkStarted = mainWorkStarted;
		}

#if 0
		virtual void SetCanEnforcedClose(bool canEnforcedClose) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->CanEnforcedClose = canEnforcedClose;
		}

		virtual bool GetCanEnforcedClose() const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			return m_gchWpfUserCtrl->CanEnforcedClose;
		}
#endif
	};

} // end of namespace


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		IMyTaskProgressDialogWrapper::TSharedPtr IMyTaskProgressDialogWrapper::Create(HWND hParent, LPCWSTR pName)
		{
			try
			{
				return IMyTaskProgressDialogWrapper::TSharedPtr(new MyTaskProgressDialogWrapperImpl(hParent, pName));
				//return std::make_shared<MyTaskProgressDialogWrapperImpl>(hParent, pName);
			}
			catch (...)
			{
			}
			return IMyTaskProgressDialogWrapper::TSharedPtr();
		}

	} // end of namespace
} // end of namespace
