// 下記のラップコードを、DirectXTex/DirectXTK 付属の DDSTextureLoader.cpp のコンパイル時に潜り込ませる。
// DDSTextureLoader.cpp には一切変更を加える必要がない。
// 公開されていないグローバル static メソッドのシグネチャを extern なしで前方宣言して使うところがミソ。
// 無名名前空間ではなく static を使っている場合はこういった Hack が使える。
// [C/C++]→[詳細設定]→[必ず使用されるインクルード ファイル] のオプションを使用する方法もあるが、
// .cpp をインクルードするソースファイルを別途用意するほうが楽。
// 
// なお、Windows 7 で DDSTextureLoader を利用する場合は、
// [C/C++]→[プリプロセッサ]→[プリプロセッサの定義] で _WIN32_WINNT=0x0601 を定義しておくのを忘れないように。

#include "stdafx.h"

#include <dxgiformat.h>
#include <cassert>

#include "MyDDSHelper.h"


void GetSurfaceInfo(
	_In_ size_t width,
	_In_ size_t height,
	_In_ DXGI_FORMAT fmt,
	_Out_opt_ size_t* outNumBytes,
	_Out_opt_ size_t* outRowBytes,
	_Out_opt_ size_t* outNumRows);


namespace MyDirectXTex
{

	void GetSurfaceInfo(
		_In_ size_t width,
		_In_ size_t height,
		_In_ DXGI_FORMAT fmt,
		_Out_opt_ size_t* outNumBytes,
		_Out_opt_ size_t* outRowBytes,
		_Out_opt_ size_t* outNumRows)
	{
		::GetSurfaceInfo(width, height, fmt, outNumBytes, outRowBytes, outNumRows);
	}

}

// DirectXTK.lib をリンクするときに、DirectXTK における DDSTextureLoader 実装とのコンフリクトを防ぐ。
#define DirectX MyDirectX

#include "DDSTextureLoader/DDSTextureLoader.cpp"
