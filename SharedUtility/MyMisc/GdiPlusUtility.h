#pragma once


namespace Misc
{

	//! @brief  GDI+ の初期化と終了を管理するクラス。<br>
	//! 
	//! コンストラクタで初期化、デストラクタで終了を行なうので、<br>
	//! 適切なクラスのコンポジション メンバーとするだけでよい。<br>
	class GdiPlusInitializer final
	{
	private:
		ULONG_PTR m_gdipToken;
	public:
		GdiPlusInitializer()
			: m_gdipToken()
		{
			Gdiplus::GdiplusStartupInput gdipSI;
			// とりあえず必ず初期化が成功するという前提。
			Gdiplus::GdiplusStartup(&m_gdipToken, &gdipSI, nullptr);
			// HACK: 初期化に失敗したら例外を投げるようにしたほうがよい。
		}

		virtual ~GdiPlusInitializer()
		{
			Gdiplus::GdiplusShutdown(m_gdipToken);
		}
	};

}
