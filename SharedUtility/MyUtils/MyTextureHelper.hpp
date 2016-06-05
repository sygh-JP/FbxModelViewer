#pragma once

#include "MyMath.hpp"


namespace MyTextureHelper
{
	class TextureDataPack final
	{
		// DIB ピクセルの並びも規定・保証したければ、WICPixelFormatGUID を使えば良いのだが……非 MS プラットフォームへの移植性が問題になる。
		// 独自の enum を設けるなりすべき。ちなみに現状では圧縮バッファには非対応。
		// なお、Direct3D 10 以降はパレット テクスチャ（インデックス画像）は存在しないので、
		// グレースケール以外の 256 色画像などがどうしても必要であれば Texture1D などをパレット代わりにしてシェーダーで対応する。
	public:
		uint32_t TextureWidth;
		uint32_t TextureHeight;
		uint32_t TextureColorDepthInBits;
		bool IsForAlpha;
		std::vector<uint8_t> TextureDib;
	public:
		TextureDataPack()
			: TextureWidth()
			, TextureHeight()
			, TextureColorDepthInBits()
			, IsForAlpha()
		{}

	public:
		uint32_t GetStrideInBytes() const
		{
			return MyMath::CalcStrideInBytes(this->TextureWidth, this->TextureColorDepthInBits);
		}
	};

	typedef std::shared_ptr<TextureDataPack> TTextureDataPackPtr;


	class FontTextureDataPack final
	{
	public:
		TextureDataPack TextureData;
		MyMath::TCharCodeUVMap CodeUVMap;
		long FontHeight;
		bool UsesMonospaceFont;
	public:
		FontTextureDataPack()
			: FontHeight()
			, UsesMonospaceFont()
		{}
	};

	typedef std::shared_ptr<FontTextureDataPack> TFontTextureDataPackPtr;

	// テクスチャ OFF 設定用などに使うダミー ホワイト テクスチャのサイズ。とりあえず2のべき乗サイズにしておく。1x1 でもいける模様。
#if 0
	const UINT DUMMY_WHITE_TEX_SIZE = 4;
#elif 0
	const UINT DUMMY_WHITE_TEX_SIZE = 2;
#else
	const UINT DUMMY_WHITE_TEX_SIZE = 1;
#endif

	// テクスチャ サイズが最大階調数（グラデーション ストップ数）になる。256 階調あれば通常の LDR と同じシェーディングも可能。
	// 128 階調だとしてもサンプリングするときにバイリニア補間などのサンプラーを使えば、
	// リニアなグラデーションに関しては大差はない。
#if 0
	const UINT TOON_SHADING_REF_TEX_SIZE = 128;
#else
	const UINT TOON_SHADING_REF_TEX_SIZE = 256;
#endif

	inline float CalcBilinearToonShadingGradientRefTexCoordV(int toonMaterialIndex)
	{
		return (toonMaterialIndex + 0.5f) / MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;
	}

} // end of namespace
