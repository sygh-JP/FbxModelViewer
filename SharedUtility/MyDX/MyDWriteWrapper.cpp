#include "stdafx.h"
#include "MyDWriteWrapper.hpp"
#include "MyUtil.h"
#include "GdiTextRenderer.h"
#include "MyDesktopHelpers.hpp"

#include "DebugNew.h"


namespace
{
#if 0
	enum GgoGrayBitmap
	{
		GgoGrayBitmap_2 = GGO_GRAY2_BITMAP,
		GgoGrayBitmap_4 = GGO_GRAY4_BITMAP,
		GgoGrayBitmap_8 = GGO_GRAY8_BITMAP,
	};

	// アルファ値の階調。
	enum GgoGrayLevel
	{
		GgoGrayLevel_2 = (2 * 2 + 1),
		GgoGrayLevel_4 = (4 * 4 + 1),
		GgoGrayLevel_8 = (8 * 8 + 1),
	};
#endif

#if 0
	//! @brief  タブ文字 0x09 をホワイトスペース 0x20 何個分とするか。<br>
	const long TabCharWidthAsWhitespace = 4;

	inline long GetWidthAsWhitespace(wchar_t charCode)
	{
		// 不可視の半角／全角スペースや復帰・改行コードの場合はアウトラインの取得およびテクスチャ ベイクを行なわない。
		switch (charCode)
		{
		case L' ':
		case L'\r':
		case L'\n':
			return 1;
		case L'\t':
			return TabCharWidthAsWhitespace;
		case L'　':
			return 2;
		default:
			// ホワイトスペースではない。
			return -1;
		}
	}
#endif

	typedef std::shared_ptr<GdiTextRenderer> TGdiTextRendererPtr;
	//typedef std::shared_ptr<Gdiplus::Graphics> TGdipGraphicsPtr;
	//typedef std::shared_ptr<Gdiplus::Bitmap> TGdipBitmapPtr;

#if 0
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
	{
		UINT num = 0, size = 0;

		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0)
		{
			return -1; // Failure
		}

		Gdiplus::ImageCodecInfo* pImageCodecInfo = static_cast<Gdiplus::ImageCodecInfo*>(malloc(size));
		if (!pImageCodecInfo)
		{
			return -1; // Failure
		}

		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

		for(UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j; // Success
			}
		}

		free(pImageCodecInfo);
		return -1; // Failure
	}

	bool SaveBitmapAsPngFile(LPCWSTR pFileName, HBITMAP hbmp)
	{
		CLSID clsid = {};
		Gdiplus::Bitmap bitmap(hbmp, static_cast<HPALETTE>(::GetStockObject(DEFAULT_PALETTE)));
		if (GetEncoderClsid(L"image/png", &clsid) >= 0)
		{
			bitmap.Save(pFileName, &clsid, nullptr);
			return true;
		}
		return false;
	}
#endif

	class MyDWriteTextRenderingManager final
	{
		Microsoft::WRL::ComPtr<IDWriteFactory> m_pDWriteFactory;
		Microsoft::WRL::ComPtr<IDWriteGdiInterop> m_pGdiInterop;
		//CComPtr<IDWriteFontFace> m_pFontFace;
		//CComPtr<IDWriteFont> m_pFont;
		//CComPtr<IDWriteFontFamily> m_pFontFamily;
		Microsoft::WRL::ComPtr<IDWriteBitmapRenderTarget> m_pBitmapRenderTarget;
		Microsoft::WRL::ComPtr<IDWriteRenderingParams> m_pRenderingParams;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> m_pTextFormat;
		TGdiTextRendererPtr m_pTextRenderer;

	public:
		MyDWriteTextRenderingManager()
		{}

		HRESULT CreateDWriteFactory()
		{
			Microsoft::WRL::ComPtr<IUnknown> pUnknownTemp;
			HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), pUnknownTemp.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("DWriteCreateFactory()"), hr);
				return hr;
			}
			hr = pUnknownTemp.As(&m_pDWriteFactory); // QueryInterface() のラッパー。dynamic_cast 相当。
			_ASSERTE(SUCCEEDED(hr) && m_pDWriteFactory != nullptr);

			Microsoft::WRL::ComPtr<IDWriteGdiInterop> pGdiInteropTemp;
			hr = m_pDWriteFactory->GetGdiInterop(pGdiInteropTemp.GetAddressOf());
			m_pGdiInterop = pGdiInteropTemp;
			if (FAILED(hr) || !m_pGdiInterop)
			{
				DXTRACE_ERR(_T("GetGdiInterop()"), hr);
				return hr;
			}
			Microsoft::WRL::ComPtr<IDWriteRenderingParams> pRenderParamsTemp;
			m_pDWriteFactory->CreateRenderingParams(pRenderParamsTemp.GetAddressOf());
			m_pRenderingParams = pRenderParamsTemp;
			if (FAILED(hr) || !m_pRenderingParams)
			{
				DXTRACE_ERR(_T("CreateRenderingParams()"), hr);
				return hr;
			}
			return hr;
		}

		//HRESULT CreateTextFormat(HDC hdc, HFONT hFont)
		HRESULT CreateTextFormat(HDC hdc, const LOGFONTW& logFont)
		{
			//_ASSERTE(m_pGdiInterop && hdc && hFont);
			_ASSERTE(m_pGdiInterop && hdc);
			HRESULT hr = E_FAIL;
			// HACK: Windows ストア アプリでは HDC というか GDI がすべて使えないので、依存を排除する。
			// Windows ストア アプリで DirectWrite を使ってオフスクリーン描画する場合、GDI ではなく Direct2D を使う必要がある。
			// DXGI 相互運用を使って、Direct3D テクスチャに直接描画してしまい、CPU 側でのリードバックを排除したほうがよい。
#if 0
			// デバイス コンテキストに設定されている現在のフォントをもとに作成。
			CComPtr<IDWriteFontFace> pFontFaceTemp;
			hr = m_pGdiInterop->CreateFontFaceFromHdc(hdc, &pFontFaceTemp);
			if (FAILED(hr) || !pFontFaceTemp)
			{
				DXTRACE_ERR(_T("CreateFontFaceFromHdc()"), hr);
				return hr;
			}
#endif
			//LOGFONTW logFont = {};
			//ATLVERIFY(::GetObjectW(hFont, sizeof(logFont), &logFont));
			Microsoft::WRL::ComPtr<IDWriteFont> pFontTemp;
			hr = m_pGdiInterop->CreateFontFromLOGFONT(&logFont, pFontTemp.GetAddressOf());
			if (FAILED(hr) || !pFontTemp)
			{
				DXTRACE_ERR(_T("CreateFontFromLOGFONT()"), hr);
				return hr;
			}
			Microsoft::WRL::ComPtr<IDWriteFontFamily> pFontFamilyTemp;
			hr = pFontTemp->GetFontFamily(pFontFamilyTemp.GetAddressOf());
			if (FAILED(hr) || !pFontFamilyTemp)
			{
				DXTRACE_ERR(_T("GetFontFamily()"), hr);
				return hr;
			}
			Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> pFamilyNames;
			hr = pFontFamilyTemp->GetFamilyNames(pFamilyNames.GetAddressOf());
			if (FAILED(hr) || !pFamilyNames)
			{
				DXTRACE_ERR(_T("GetFamilyNames()"), hr);
				return hr;
			}
			UINT32 familyNameLength = 0;
			hr = pFamilyNames->GetStringLength(0, &familyNameLength);
			if (FAILED(hr) || familyNameLength == 0)
			{
				DXTRACE_ERR(_T("GetStringLength()"), hr);
				return hr;
			}
			std::vector<wchar_t> strBuffer(familyNameLength + 1);
			hr = pFamilyNames->GetString(0, &strBuffer[0], familyNameLength + 1);
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("GetString()"), hr);
				return hr;
			}
#if false
			const auto fontSize = fabs(static_cast<float>(::MulDiv(logFont.lfHeight, 96, ::GetDeviceCaps(hdc, LOGPIXELSY))));
#else
			// IDWriteFactory::CreateTextFormat() が受け取るのは DIP 単位のフォントサイズ。
			// LOGFONT の高さを論理ピクセルサイズとして渡すことにする。
			const auto fontSize = static_cast<float>(abs(logFont.lfHeight));
#endif
			Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormatTemp;
			hr = m_pDWriteFactory->CreateTextFormat(&strBuffer[0], nullptr,
				pFontTemp->GetWeight(), pFontTemp->GetStyle(), pFontTemp->GetStretch(),
				fontSize, L"", pTextFormatTemp.GetAddressOf());
			m_pTextFormat = pTextFormatTemp;
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("CreateTextFormat()"), hr);
				return hr;
			}
			// デフォルトのトリミング設定は DWRITE_TRIMMING_GRANULARITY_NONE となっている模様。
			// IDWriteTextLayout を使う場合、IDWriteTextLayout::SetTrimming() を呼ばないと意味がない模様。
			//DWRITE_TRIMMING trimOption = {};
			//CComPtr<IDWriteInlineObject> pInlineObjectTemp;
			//hr = m_pTextFormat->GetTrimming(&trimOption, &pInlineObjectTemp);
			//trimOption.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
			//hr = m_pTextFormat->SetTrimming(&trimOption, pInlineObjectTemp);
			return hr;
		}

		HRESULT CreateTextBitmap(HDC hdc, const std::vector<wchar_t>& strBuffer, UINT32 bmpWidth, UINT32 bmpHeight, _Out_ std::vector<uint8_t>& textureBitmapBuffer, _Out_ MyMath::TCharCodeUVMap& codeUVMap)
		{
			// TODO: テクスチャ画像作成処理を外部ツール化する場合、C++ & GDI & DirectWrite ではなく、WPF でやってしまったほうがよい。
			// なお、WPF も 4.0 以降は結局内部で DirectWrite を採用しているので、Windows のフォント描画クオリティの限界に直面する。
			// 真にプラットフォーム非依存のきれいなフォント テクスチャが欲しい場合、Photoshop や Illustrator と
			// JavaScript (ExtendScript) による自動化された事前作成をするしかないかも（JavaScript であればアクション機能よりも細かく、ほとんどすべての機能を制御して自動化できるはず）。
			// Windows 版 Photoshop / Illustrator は COM コンポーネントで構築されているらしく、JavaScript は実質 JScript になる模様。
			_ASSERTE(m_pGdiInterop && m_pTextFormat && hdc && !strBuffer.empty());
			HRESULT hr = E_FAIL;
			// DirectWrite を使って実際に DIB バッファに文字列を描画する（ピクセルを操作する）には、
			// レンダリング バックエンドとして GDI もしくは Direct2D を使う必要がある。
			// ただし Direct2D による描画結果を CPU 側のシステム メモリーにマッピングする場合、
			// 直接は不可能。Staging リソースを経由した転送処理が必要となる。
			// D2D 1.1 では CPU 側で参照する際に ID2D1Bitmap1::Map() が使えるが、
			// D2D1_BITMAP_OPTIONS_CPU_READ を指定すると D2D1_BITMAP_OPTIONS_TARGET が指定できないため、利用不可能。
			// これは、D3D10_CPU_ACCESS_FLAG, D3D11_CPU_ACCESS_FLAG 同様に、
			// IDXGISurface::Map() や ID3D10Texture2D::Map(), ID3D11DeviceContext::Map() による CPU 側での読み書きを可能とすると、
			// そのビットマップ（テクスチャ）はレンダーターゲットとして指定できなくなるのと同じ。
			// http://msdn.microsoft.com/en-us/library/windows/desktop/hh446984.aspx
			// http://msdn.microsoft.com/ja-jp/library/bb204908.aspx
			// http://msdn.microsoft.com/ja-jp/library/ee416074.aspx
			// 今回は静的な初回書き込みのみで、また CPU 側で完結したほうが楽なので、GDI を使う。
			// HUD 用の短いテキストは、テクスチャ画像を実行時に作成せず、
			// DirectWrite + GDI, WPF, Photoshop アクションなどによる画像ファイル生成用のツールを作っておいたほうが良い。
			// 日本語の長いテキストなど、多数の文字種（サロゲート ペアも含む）が使われる文章を柔軟に描画するために、
			// どうしても動的なテクスチャ書き換えを行なう必要がある場合、
			// OpenGL を使わず、Direct3D のみを使うのであれば、
			// Direct2D を使った DXGI サーフェイスへの直接書き込みを行なったほうがよい。
			// OpenGL と連携しないのであれば、CPU 側メモリーへのマッピング（書き戻し）も必要ない。
			// D3D 10.1 + D2D 1.0 では ID3D10Texture2D, IDXGISurface と ID2D1Factory::CreateDxgiSurfaceRenderTarget() を使う。
			// D3D 11.1 + D2D 1.1 では ID3D11Texture2D, IDXGISurface と ID2D1DeviceContext::CreateBitmapFromDxgiSurface() を使う。
			// 文字列描画処理自体は GDI よりも Direct2D をバックエンドに使ったほうが圧倒的に高速だと思われるが、
			// Windows 環境における OpenGL での柔軟な日本語長文テキスト描画は、
			// Direct2D での描画結果を CPU メモリーに転送して OpenGL テクスチャに変換するよりも、
			// 都度 DirectWrite + GDI を使って CPU 側で更新したテクスチャを使うようにしたほうがよいかも。
			// もちろんテクスチャの更新を CPU で毎フレーム行なうとパフォーマンスがガタ落ちなので、更新するタイミングなどは検討する必要がある。
			Microsoft::WRL::ComPtr<IDWriteBitmapRenderTarget> pBitmapRenderTargetTemp;
			hr = m_pGdiInterop->CreateBitmapRenderTarget(hdc, bmpWidth, bmpHeight, pBitmapRenderTargetTemp.GetAddressOf());
			m_pBitmapRenderTarget = pBitmapRenderTargetTemp;
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("CreateBitmapRenderTarget()"), hr);
				return hr;
			}
			HDC hMemDC = m_pBitmapRenderTarget->GetMemoryDC();
			//::SetBoundsRect(hMemDC, nullptr, DCB_ENABLE | DCB_RESET);
			//::SetDCBrushColor(hMemDC, RGB(0, 0, 0x80));
			//::SelectObject(hMemDC, GetStockObject(DC_BRUSH));
			//::SetBkMode(hMemDC, TRANSPARENT);
			{
				//TGdipGraphicsPtr g(Gdiplus::Graphics::FromHDC(hMemDC));
				//g->Clear(0);
			}
			float maxFontHeight = 0;
			const float iniX = 0, iniY = 0;
			float posX = iniX, posY = iniY;
			m_pTextRenderer = TGdiTextRendererPtr(new GdiTextRenderer(m_pBitmapRenderTarget.Get(), m_pRenderingParams.Get()));
			// 1文字ずつバッファに書き込んでいく。
			for (size_t i = 0; i < strBuffer.size(); ++i)
			{
				Microsoft::WRL::ComPtr<IDWriteTextLayout> pTextLayoutTemp;
				const wchar_t index = strBuffer[i];
				const wchar_t tempString[2] = { index }; // サロゲートペアは考慮しない。
				hr = m_pDWriteFactory->CreateTextLayout(tempString, 1, m_pTextFormat.Get(),
					//static_cast<float>(bmpWidth), static_cast<float>(bmpHeight),
					0, 0,
					pTextLayoutTemp.GetAddressOf());
				if (FAILED(hr))
				{
					DXTRACE_ERR(_T("CreateTextLayout()"), hr);
					continue;
				}
				DWRITE_TEXT_METRICS textMetrics = {};
				hr = pTextLayoutTemp->GetMetrics(&textMetrics);
				const float paddingX = 2, paddingY = 1;
				//const float fontWidth = textMetrics.widthIncludingTrailingWhitespace;
				// --> これだと OpenGL では良くても Direct3D では最近傍補間の品質が劣る。
				// 等倍表示する際は、Width, Height および Feed は丸めたほうがよさげ。
				// 三角形ラスタライズ ルールの違いが影響している？
				const float fontWidth = std::ceil(textMetrics.widthIncludingTrailingWhitespace);
				const float fontHeight = std::ceil(textMetrics.height);
				maxFontHeight = (std::max)(maxFontHeight, fontHeight);
				const float feedX = fontWidth + paddingX;
				const float feedY = maxFontHeight + paddingY;
				// DWRITE_TEXT_METRICS が扱うのは DIP なので、デバイスピクセル単位との比較には換算が必要。
				const int logPixelsX = ::GetDeviceCaps(hdc, LOGPIXELSX);
				const int logPixelsY = ::GetDeviceCaps(hdc, LOGPIXELSY);
				const float devPixelsPerLogX = logPixelsX / 96.0f;
				const float devPixelsPerLogY = logPixelsY / 96.0f;
				if ((posX + feedX) * devPixelsPerLogX > bmpWidth)
				{
					posX = iniX;
					posY += feedY;
				}
				// 1文字分のグリフ ビットマップ データを書き込む。
				// ただしアルファ チャンネルは一切描き込まれず、常に完全透過になってしまう模様。
				hr = pTextLayoutTemp->Draw(nullptr, m_pTextRenderer.get(), posX, posY);
				// 1文字分のフォント境界ボックス矩形の UV。
				_ASSERTE(codeUVMap.size() > index);
				// TODO: UV はデバイスピクセル単位を使う。
				codeUVMap[index] = MyMath::RectF(
					posX * devPixelsPerLogX,
					posY * devPixelsPerLogY,
					fontWidth * devPixelsPerLogX,
					fontHeight * devPixelsPerLogY);
				posX += feedX;
				if (FAILED(hr))
				{
					DXTRACE_ERR(_T("Draw()"), hr);
					continue;
				}
			}
			if (true)
			{
				// EXE のあるフォルダーに出力される。
				CPath pathFontAlphaMapFile;
				ATLVERIFY(MyDesktopHelpers::GetModuleDirPath(pathFontAlphaMapFile));
				pathFontAlphaMapFile += _T("dwrite_test.png");
				HBITMAP hBmp = static_cast<HBITMAP>(::GetCurrentObject(hMemDC, OBJ_BITMAP));
				//BITMAPINFO bmpInfo = {};
				//const int dibSize = ::GetDIBits(hMemDC, hBmp, 0, bmpHeight, nullptr, &bmpInfo, DIB_RGB_COLORS);
				//ATLTRACE("DibSize = %d\n", dibSize);
#if 0
				HDC hTempDC = ::CreateCompatibleDC(hMemDC);
				HBITMAP hTempBmp = ::CreateCompatibleBitmap(hTempDC, bmpWidth, bmpHeight);
				::SelectObject(hTempDC, hTempBmp);
				::SelectObject(hTempDC, nullptr);
				::DeleteObject(hTempBmp);
				::DeleteDC(hTempDC);
#endif
				//SaveBitmapAsPngFile(pathFontAlphaMapFile, hBmp);
#if 1
				// MSDN によると、GetDIBits() の hbmp パラメータが示すビットマップが
				// デバイス コンテキストで選択されている場合、この関数を呼び出してはいけないらしい。
				// DIB ビットマップをいちいち GDI 関数を使って取り出すのは面倒だが、CImage を使うと楽。
				CImage image;
				image.Attach(hBmp, CImage::DIBOR_TOPDOWN);
				//image.SetHasAlphaChannel(true);
				const auto* pDibBuffer = static_cast<const MyMath::ColorBgra*>(image.GetBits());
				textureBitmapBuffer.resize(bmpWidth * bmpHeight);
				// DirectWrite による描画は ClearType なので、8bpp グレースケールに変換する。パディングなし。
				// Direct2D 経由でサーフェイスに描画する場合も ClearType を有効にできるが、
				// Windows ストア アプリの C++ プロジェクト テンプレートでは D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE が使われている。
				// ClearType でなくグレースケール アンチエイリアスのほうが推奨されているらしい。
				for (UINT32 i = 0; i < bmpWidth * bmpHeight; ++i)
				{
					const auto colorVal = pDibBuffer[i];
					textureBitmapBuffer[i] = MyMath::ToGrayscaleUI8(colorVal.R, colorVal.G, colorVal.B);
				}
				// 直接アルファ マップとしては使えないが、参考までにファイル保存しておく。
				hr = image.Save(pathFontAlphaMapFile, Gdiplus::ImageFormatPNG);
				image.Detach();
#endif
			}
			return hr;
		}
	};
} // end of namespace


namespace MyDWriteWrapper
{
	bool CreateFontAlphaTextureBufferFromDC(HDC hdc, const LOGFONTW& logFont, long monospaceFontHeight, UINT texWidth, UINT texHeight, _Out_ std::vector<uint8_t>& textureBuf, _Out_ MyMath::TCharCodeUVMap& codeUVMap)
	{
		_ASSERTE((texWidth % 4) == 0); // パディングなし。
		_ASSERTE(MyMath::IsModulo2(texWidth) && MyMath::IsModulo2(texHeight)); // 2 のべき乗サイズであったほうがよい。

		// ASCII のみ対応。
		const UINT firstCode = 0x20, lastCode = 0x7e; // isprint(), iswprint() で判定可能。

		codeUVMap.resize(128); // 0x00～0x7f の ASCII のみ。
		_ASSERTE(lastCode < codeUVMap.size());
		std::fill(codeUVMap.begin(), codeUVMap.end(), MyMath::RectF()); // 念のため、すべて null 矩形にしておく。

		// DirectWrite によるフォント グリフの取得。
		MyDWriteTextRenderingManager dwriteManager;
		if (FAILED(dwriteManager.CreateDWriteFactory()))
		{
			return false;
		}
		if (FAILED(dwriteManager.CreateTextFormat(hdc, logFont)))
		{
			return false;
		}

		std::vector<wchar_t> strBuffer;
		for (UINT i = firstCode; i <= lastCode; ++i)
		{
			strBuffer.push_back(static_cast<wchar_t>(i));
		}
		return SUCCEEDED(dwriteManager.CreateTextBitmap(hdc, strBuffer, texWidth, texHeight, textureBuf, codeUVMap));
#if 0
		const long monospaceFontWidth = monospaceFontHeight / 2;
		const long maxFontNumX = texWidth / monospaceFontWidth;
		const long maxFontNumY = texHeight / monospaceFontHeight;
		const UINT codeDiff = lastCode - firstCode;

		if (maxFontNumX * maxFontNumY < codeDiff)
		{
			return false;
		}
#endif

		// GDI はいろいろ問題があるので却下。もう二度と使わない。
#if 0
		textureBuf.resize(texWidth * texHeight);

		for (UINT code = firstCode, offsetX = 0, offsetY = 0; code <= lastCode; ++code)
		{
			// フォント グリフ ビットマップ取得。
			TEXTMETRICW tm = {};
			::GetTextMetricsW(hdc, &tm);
			GLYPHMETRICS gm = {};
			const MAT2 mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
			const DWORD bufSize = ::GetGlyphOutlineW(hdc, code, GgoGrayBitmap_8, &gm, 0, nullptr, &mat);
			if (bufSize != GDI_ERROR && bufSize > 0)
			{
				std::vector<BYTE> outlineBuf(bufSize);
				::GetGlyphOutlineW(hdc, code, GgoGrayBitmap_8, &gm, bufSize, &outlineBuf[0], &mat);

				const SIZE fontSize = { gm.gmCellIncX, tm.tmHeight };

				// フォント情報をテクスチャ作成用の一時バッファに書き込んでいく。

				// フォント グリフ ビットマップの幅と高さ。
				const long fontBmpW = gm.gmBlackBoxX + (4 - (gm.gmBlackBoxX % 4)) % 4;
				const long fontBmpH = gm.gmBlackBoxY;

				const UINT feedAmt = max(fontSize.cx, fontBmpW);
				if (offsetX + feedAmt >= texWidth)
				//if (offsetX + fontSize.cx >= texWidth)
				{
					offsetX = 0;
					offsetY += monospaceFontHeight;
					if (offsetY + monospaceFontHeight >= texHeight)
					{
						ATLTRACE("Failed to map all fonts to a texture!!\n");
						return false;
					}
				}

				// 書き出し位置（左上）。
				const POINT localOffsets =
				{
					gm.gmptGlyphOrigin.x + offsetX,
					tm.tmAscent - gm.gmptGlyphOrigin.y + offsetY,
				};
				ATLTRACE(L"0x%02X(%c): FontSize(%ld, %ld), Offset(%u, %u), Bmp(%d, %d), GM(%u, %u)\n", code, code,
					fontSize.cx, fontSize.cy, offsetX, offsetY, fontBmpW, fontBmpH, gm.gmBlackBoxX, gm.gmBlackBoxY);
				// フォント境界ボックス矩形の UV。
#if 0
				const RectF rectuv(
					static_cast<float>(localOffsets.x) / texWidth,
					static_cast<float>(localOffsets.y) / texHeight,
					static_cast<float>(localOffsets.x + fontSize.cx) / texWidth,
					static_cast<float>(localOffsets.y + fontSize.cy) / texHeight);
#elif 0
				const RectF rectuv(
					static_cast<float>(offsetX) / (texWidth - 0),
					static_cast<float>(offsetY) / (texHeight - 0),
					static_cast<float>(offsetX + feedAmt) / (texWidth - 0),
					//static_cast<float>(offsetX + monospaceFontHeight / 2) / (texWidth - 0),
					static_cast<float>(offsetY + monospaceFontHeight / 1) / (texHeight - 0));
#else
				const RectF rectuv(
					static_cast<float>(offsetX),
					static_cast<float>(offsetY),
					static_cast<float>(offsetX + feedAmt),
					static_cast<float>(offsetY + monospaceFontHeight));
#endif
				// 1文字分のグリフ ビットマップ データを書き込む。
				for (long y = localOffsets.y; y < localOffsets.y + fontBmpH; ++y)
				{
					for (long x = localOffsets.x; x < localOffsets.x + fontBmpW; ++x)
					{
						const int alpha =
							(255 * outlineBuf[x - localOffsets.x + fontBmpW * (y - localOffsets.y)])
							/ (GgoGrayLevel_8 - 1);
						const size_t pos = texWidth * y + x;
						_ASSERTE(pos < textureBuf.size());
						textureBuf[pos] = MyUtils::Clamp(alpha, 0, 255);
					}
				}
				offsetX += feedAmt;
				//offsetX += fontSize.cx;
				//offsetX += fontBmpW;
				codeUVMap[code] = rectuv;
			}
			else
			{
				continue;
				// グリフが取得できないのであれば、UV には null 矩形を使う。
				// 送りは必要ない。
			}
		}
		return true;
#endif
	}

} // end of namespace
