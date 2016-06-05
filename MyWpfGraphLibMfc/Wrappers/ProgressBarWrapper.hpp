//! @file
//! @brief  ネイティブ C++ および C++/CLI 両方から参照されるヘッダー ファイル。<br>

#pragma once


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		struct IMyProgressBarWrapper abstract
		{
		public:
			typedef std::shared_ptr<IMyProgressBarWrapper> TSharedPtr;
		protected:
			virtual ~IMyProgressBarWrapper() = 0 {} // テンプレート ベースの shared_ptr スマートポインタを使う場合は、実は非仮想デストラクタであってもよい。
		public:
			virtual bool GetIsKeyboardFocusWithin() const = 0;
			virtual bool DoTranslateAccelerator(_Inout_ MSG* pMsg) = 0;
			virtual void SetFocusToControl() = 0;
			virtual bool SetSize(int width, int height) = 0;

			virtual bool GetVisibility() const = 0;
			virtual void SetVisibility(bool isVisible) = 0;

			virtual double GetValue() const = 0;
			virtual void SetValue(double value) = 0;

			// サブスレッドからも実行できるメソッド。
			virtual void InvokeSetValue(double value) = 0;

			// サブスレッドからも実行できるメソッド。
			virtual void Refresh() = 0;

			virtual double GetMinimum() const = 0;
			virtual void SetMinimum(double value) = 0;
			virtual double GetMaximum() const = 0;
			virtual void SetMaximum(double value) = 0;

			// Win32 でいう Marquee に相当。進行速度の制御はできないらしい？
			virtual bool GetIsIndeterminate() const = 0;
			virtual void SetIsIndeterminate(bool isIndeterminate) = 0;

			static AFX_EXT_API IMyProgressBarWrapper::TSharedPtr Create(HWND hParent, int x, int y, int width, int height, LPCWSTR pName);
		};

	} // end of namespace
} // end of namespace
