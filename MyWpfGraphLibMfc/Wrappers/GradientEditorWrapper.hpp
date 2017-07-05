//! @file
//! @brief  ネイティブ C++ および C++/CLI 両方から参照されるヘッダー ファイル。<br>

#pragma once


namespace MyWpfGraphLibWrapper
{
	namespace MiscControls
	{
		class GradientColorParams final
		{
		public:
			double Offset; // 0.0~1.0 の範囲。
			COLORREF Color;
		public:
			GradientColorParams()
				: Offset()
				, Color(RGB(0xFF, 0xFF, 0xFF))
			{}
			GradientColorParams(double offset, COLORREF color)
				: Offset(offset)
				, Color(color)
			{}
		};

		class GradientOpacityParams final
		{
		public:
			double Offset; // 0.0~1.0 の範囲。
			double Opacity;
		public:
			GradientOpacityParams()
				: Offset()
				, Opacity(1)
			{}
			GradientOpacityParams(double offset, double opacity)
				: Offset(offset)
				, Opacity(opacity)
			{}
		};

		typedef std::vector<GradientColorParams> TGradientColorParamsArray;
		typedef std::vector<GradientOpacityParams> TGradientOpacityParamsArray;


		struct IMyGradientEditorWrapper abstract
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
			typedef std::shared_ptr<IMyGradientEditorWrapper> TSharedPtr;
		protected:
			virtual ~IMyGradientEditorWrapper() = 0 {} // テンプレート ベースの shared_ptr スマートポインタを使う場合は、実は非仮想デストラクタであってもよい。
		public:
			virtual void SetNativeEventListener(INativeEventListener* pEventListener) = 0;
			virtual void ChangeCulture(CStringW strCultureName) = 0;
			virtual bool ShowModalDialog(/*HWND hWndOwner*/) = 0;
			virtual bool Show() = 0;
			virtual bool Hide() = 0;
			virtual void Close() = 0;
			virtual bool SetTopOn(HWND hWnd) = 0;
			virtual void SetIsEnabled(bool isEnabled) = 0;
			virtual void SetVisibility(bool isVisible) = 0;
			virtual bool GetVisibility() const = 0;
			virtual void SetOpacity(double opacity) = 0;
			virtual double GetOpacity() const = 0;
#if 0
			virtual void SetCanEnforcedClose(bool canEnforcedClose) = 0;
			virtual bool GetCanEnforcedClose() const = 0;
#endif
			virtual void SetGradientColorStops(const TGradientColorParamsArray& inArray) = 0;
			virtual void GetGradientColorStops(TGradientColorParamsArray& outArray) const = 0;

			static AFX_EXT_API IMyGradientEditorWrapper::TSharedPtr Create(HWND hParent, int x, int y, int width, int height, LPCWSTR pName);
		};

	} // end of namespace
} // end of namespace
