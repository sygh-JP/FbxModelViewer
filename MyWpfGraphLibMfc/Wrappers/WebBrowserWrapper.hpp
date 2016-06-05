//! @file
//! @brief  ネイティブ C++ および C++/CLI 両方から参照されるヘッダー ファイル。<br>

#pragma once


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		// UNDONE: JavaScript イベントなどをネイティブ側で受け取る必要がある場合は、イベント リスナーを実装する。
		// UNDONE: Web ブラウザー（IE）のキーボード ショートカットやコンテキスト メニューを無効にする必要がある場合は、適宜ラッパーメソッドを実装する。
		// ちなみに HTML 側の body 要素をいじって無効にする方法もある。
		// e.g. <body oncontextmenu="return false;">


		struct IMyWebBrowserWrapper abstract
		{
		public:
			typedef std::shared_ptr<IMyWebBrowserWrapper> TSharedPtr;
		protected:
			virtual ~IMyWebBrowserWrapper() = 0 {} // テンプレート ベースの shared_ptr スマートポインタを使う場合は、実は非仮想デストラクタであってもよい。
		public:
			virtual bool GetIsKeyboardFocusWithin() const = 0;
			virtual bool DoTranslateAccelerator(_Inout_ MSG* pMsg) = 0;
			virtual void SetFocusToControl() = 0;
			virtual void ChangeCulture(CStringW strCultureName) = 0;
			virtual bool OnSize(UINT sizingType, int width, int height) = 0;

			virtual bool SetSourceUri(CStringW strUri) = 0;
			virtual bool GetVisibility() const = 0;
			virtual void SetVisibility(bool isVisible) = 0;

			static AFX_EXT_API IMyWebBrowserWrapper::TSharedPtr Create(HWND hParent, int x, int y, int width, int height, LPCWSTR pName);
		};

	} // end of namespace
} // end of namespace
