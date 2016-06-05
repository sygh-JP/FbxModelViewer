#pragma once

#include "MyTextureHelper.hpp"


namespace MyDWriteWrapper
{
	//! @brief  デバイス コンテキストに設定されているフォントのアルファ テクスチャ用バッファを作成する。<br>
	//! 現時点では ASCII のみ。<br>
	//! 内部では DirectWrite と GDI の相互運用を利用しているが、最終的にただの DIB を作成するので Direct3D と OpenGL 両方で使用できる。<br>
	//! そのせいで逆に WinRT では利用できない。<br>
	//! なお、Android や iOS では当然別の API を利用する必要があるが、<br>
	//! モバイル向けの場合こういったデータはコンテンツ（アセット）としてオフライン ツールで作成しておくほうがよい。<br>
	extern bool CreateFontAlphaTextureBufferFromDC(HDC hdc, const LOGFONTW& logFont, long monospaceFontHeight, UINT texWidth, UINT texHeight, _Out_ std::vector<uint8_t>& textureBuf, _Out_ MyMath::TCharCodeUVMap& codeUVMap);
} // end of namespace
