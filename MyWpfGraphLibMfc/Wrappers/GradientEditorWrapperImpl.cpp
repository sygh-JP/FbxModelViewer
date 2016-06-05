#include "stdafx.h"
#include "../stdafx.h" // インテリセンス用の冗長インクルード。
//#include "../CppCliInterop.hpp"
#include "../CppCliMisc.hpp"
#include "GradientEditorWrapper.hpp"

#include "../../SharedUtility/PublicInclude/DebugNew.h"


namespace
{

	//! @brief  マネージ イベントをネイティブ側に伝えるためのブリッジ用マネージ クラス。非公開。<br>
	private ref class MyGradientEditorManagedEventListenerImpl sealed
		: MyWpfGraphLibrary::MyGradientEditor::IManagedEventListener
	{
	private:
		MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper::INativeEventListener* m_pEventListener;
	public:
		void SetNativeEventListener(MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper::INativeEventListener* pEventListener)
		{ m_pEventListener = pEventListener; }
	public:
		MyGradientEditorManagedEventListenerImpl()
			: m_pEventListener()
		{}

	public:
		//virtual void OnUpdate(cli::array<MyWpfGraphLibrary::MyGradientStopPin^>^ gradientStopPins)
		virtual void OnUpdate(int gradientStopCount)
		{
			if (m_pEventListener)
			{
				m_pEventListener->OnUpdate(gradientStopCount);
			}
		}
	};


	using MyWpfGraphLibWrapper::MiscControls::GradientColorParams;
	using MyWpfGraphLibWrapper::MiscControls::TGradientColorParamsArray;


	//! @brief  WPF コントロールをラップする混合クラス。非公開。<br>
	class MyGradientEditorWrapperImpl
		: public MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper
	{
	private:
		bool m_isWpfCtrlInitialized;
		gcroot<MyWpfGraphLibrary::MyGradientEditorDialog^> m_gchWpfUserCtrl;
		gcroot<MyGradientEditorManagedEventListenerImpl^> m_gchManagedEventListenerImpl;
		gcroot<System::Windows::Interop::WindowInteropHelper^> m_gchWinInteropHelper;

	private:
		void SetPositiveSize(int width, int height)
		{
			m_gchWpfUserCtrl->Width = width > 0 ? width : 1;
			m_gchWpfUserCtrl->Height = height > 0 ? height : 1;
		}

	public:
		MyGradientEditorWrapperImpl(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
			: m_isWpfCtrlInitialized()
		{
			m_gchWpfUserCtrl = gcnew MyWpfGraphLibrary::MyGradientEditorDialog();
			//System::Diagnostics::Debug::Assert(width > 0 && height > 0);
			//this->SetPositiveSize(width, height);

			m_gchWinInteropHelper = gcnew System::Windows::Interop::WindowInteropHelper(m_gchWpfUserCtrl);
			m_gchWinInteropHelper->Owner = CppCliMisc::ConvertHwndToIntPtr(hParent);

			m_gchManagedEventListenerImpl = gcnew MyGradientEditorManagedEventListenerImpl();
			m_gchWpfUserCtrl->EventListener = m_gchManagedEventListenerImpl;

			m_isWpfCtrlInitialized = true;
		}

		~MyGradientEditorWrapperImpl()
		{
			// 完全クローズ。
			this->Close();
			// イベント リスナーの解除。
			this->SetNativeEventListener(nullptr);
		}

		virtual void SetNativeEventListener(MyWpfGraphLibWrapper::MiscControls::IMyGradientEditorWrapper::INativeEventListener* pEventListener) override
		{
			m_gchManagedEventListenerImpl->SetNativeEventListener(pEventListener);
		}

		virtual void ChangeCulture(CStringW strCultureName) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			m_gchWpfUserCtrl->ChangeCulture(CppCliMisc::CreateCultureInfoByName(strCultureName));
		}

		virtual bool ShowModalDialog(/*HWND hWndOwner*/) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			HWND hParent = CppCliMisc::ConvertIntPtrToHwnd(m_gchWinInteropHelper->Owner);
			System::Diagnostics::Debug::Assert(!!::IsWindow(hParent));
			// IsWindow() が false になるのは、
			// コンストラクタで指定した親が nullptr だった場合だけでなく、すでに死んでいた場合も該当する。
			// ちなみに子ウィンドウに親を指定すると、親が死んだタイミングで子ウィンドウも連鎖的に死ぬことになる。
			// なお、WPF ウィンドウをいったん表示したら、その親（Owner プロパティ）は再設定できないらしい。
			// WindowInteropHelper 経由で Owner を設定する場合も同様。
			// MSDN にも、「一般的な Win32 プログラミングの場合と同様に、既に使用されているウィンドウの親は再設定しないでください。」
			// という注意書きがある。
			// つまり親の動的な変更は無理なので、ダイアログのインスタンスを使いまわす場合、
			// 最初に設定した親ウィンドウがずっと生存していることが保証されていないといけない。
			return m_gchWpfUserCtrl->ShowModalDialog();
			// インスタンス再利用のために Window.Closing をキャンセルしているので、
			// Window.ShowDialog() の戻り値も Window.DialogResult プロパティもあてにならない。
			// 独自定義した専用メソッド ShowModalDialog() を利用する。
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

		virtual void Close() override
		{
			if (m_isWpfCtrlInitialized)
			{
				try
				{
					// Window が一度も Show() されていない状態で Close() しても例外は発生しないらしい。
					m_gchWpfUserCtrl->CanEnforcedClose = true;
					m_gchWpfUserCtrl->Close();
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

		virtual void SetGradientColorStops(const TGradientColorParamsArray& inArray) override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			//System::Diagnostics::Debug::Assert(false, L"Not Implemented!!");
			const int size = int(inArray.size());
			auto pinArray = gcnew array<MyWpfGraphLibrary::MyGradientStopPin^>(size);
			for (int i = 0; i < size; ++i)
			{
				pinArray[i] = gcnew MyWpfGraphLibrary::MyGradientStopPin();
				COLORREF color = inArray[i].Color;
				pinArray[i]->SetSurfaceColorAndGradientOffset(
					System::Windows::Media::Color::FromRgb(
					GetRValue(color),
					GetGValue(color),
					GetBValue(color)),
					inArray[i].Offset);
			}
			m_gchWpfUserCtrl->SetGradientColorStopPinsArray(pinArray);
		}

		virtual void GetGradientColorStops(TGradientColorParamsArray& outArray) const override
		{
			System::Diagnostics::Debug::Assert(m_isWpfCtrlInitialized);
			outArray.clear();
			auto pinArray = m_gchWpfUserCtrl->GetGradientColorStopPinsArray();

			//for (auto pin : pinArray) {}
			// --> C++11 の range-based for loop だが、C++/CLI のマネージ型には使えない。
			// マネージのコレクションに対しては、従来からの for each 拡張構文を使う必要がある。
			// ちなみに VC 2012 では System.Xaml の参照が不足している、というエラーメッセージが出るが、
			// 実際に参照設定に追加してコンパイルすると、VC コンパイラ cl.exe がクラッシュする。
			// VC 2013 では、
			// 「error C3312: 型 'cli::array<XXX ^,1> ^' に対して呼び出し可能な 'begin' 関数が見つかりません」
			// 「error C3312: 型 'cli::array<XXX ^,1> ^' に対して呼び出し可能な 'end' 関数が見つかりません」
			// というように、正しいエラーメッセージを出力するように修正された模様。
			// 
			// なお、マネージ ハンドル型と nullptr の比較を行なう場合、同様に
			// 「error C3624: 'System::Windows::Markup::IQueryAmbient': この型を使用するには、アセンブリ 'System.Xaml' への参照が必要です」
			// というエラーメッセージが出るが、こちらは実際に System.Xaml を追加する必要があるらしい。
			// 
			// ちなみに、VC 2008 において、std::map をメンバーに含むようなクラス インスタンスのコンテナに対して
			// for each を使うと、おかしな動作をする（マップが空になる）。
			// また、VC 2008 においては、for each の内部ではインテリセンスが効かなくなる。
			// VC 2010 以降ではこれらの問題が解消されているようだが、
			// 古い環境で for each を使用する場合は注意すること。
			// 下記は関連がありそう。
			// http://d.hatena.ne.jp/faith_and_brave/20080514/1210755507
			// http://connect.microsoft.com/VisualStudio/feedback/details/375614/native-c-for-each-unable-to-enumerate-a-map-with-another-stl-container-as-its-value-type

			for each (auto pin in pinArray)
			{
				System::Diagnostics::Debug::Assert(pin != nullptr);
				auto color = pin->AssociatedGradientStop->Color;
				outArray.push_back(GradientColorParams(
					pin->AssociatedGradientStop->Offset,
					RGB(color.R, color.G, color.B)
					));
			}
		}
	};

} // end of namespace


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		IMyGradientEditorWrapper::TSharedPtr IMyGradientEditorWrapper::Create(HWND hParent, int x, int y, int width, int height, LPCWSTR pName)
		{
			try
			{
				return IMyGradientEditorWrapper::TSharedPtr(new MyGradientEditorWrapperImpl(hParent, x, y, width, height, pName));
			}
			catch (...)
			{
			}
			return IMyGradientEditorWrapper::TSharedPtr();
		}

	} // end of namespace
} // end of namespace
