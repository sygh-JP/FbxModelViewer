//! @file
//! @brief  ネイティブ C++ および C++/CLI 両方から参照されるヘッダー ファイル。<br>

#pragma once


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		struct IMyTaskProgressDialogWrapper abstract
		{
		public:
			struct INativeEventListener abstract
			{
			protected:
				virtual ~INativeEventListener() = 0 {} // テンプレート ベースの shared_ptr スマートポインタを使う場合は、実は非仮想デストラクタであってもよい。
			public:
				virtual void OnUpdate(int gradientStopCount) = 0;
			};
		public:
			typedef std::shared_ptr<IMyTaskProgressDialogWrapper> TSharedPtr;
			using TEventMainWorkStarted = std::function<void()>;
		protected:
			virtual ~IMyTaskProgressDialogWrapper() = 0 {} // テンプレート ベースの shared_ptr スマートポインタを使う場合は、実は非仮想デストラクタであってもよい。
		public:
			virtual void SetNativeEventListener(INativeEventListener* pEventListener) = 0;
			virtual void ChangeCulture(CStringW strCultureName) = 0;
			virtual bool ShowModalDialog(/*HWND hWndOwner*/) = 0;
			virtual bool Show() = 0;
			virtual bool Hide() = 0;
			virtual void EnforcedClose() = 0;
			virtual bool SetTopOn(HWND hWnd) = 0;
			virtual void SetIsEnabled(bool isEnabled) = 0;
			virtual void SetVisibility(bool isVisible) = 0;
			virtual bool GetVisibility() const = 0;
			virtual void SetOpacity(double opacity) = 0;
			virtual double GetOpacity() const = 0;
			virtual void SetEventMainWorkStarted(const TEventMainWorkStarted& mainWorkStarted) = 0;
#if 0
			virtual void SetCanEnforcedClose(bool canEnforcedClose) = 0;
			virtual bool GetCanEnforcedClose() const = 0;
#endif
			static AFX_EXT_API IMyTaskProgressDialogWrapper::TSharedPtr Create(HWND hParent, LPCWSTR pName);
		};

	} // end of namespace
} // end of namespace
