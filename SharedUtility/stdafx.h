// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

// GDI+ や MFC のヘッダーで WinDef.h, minwindef.h の min/max マクロを使っているので、NOMINMAX は定義不可能。
// 標準 C++ の std::min(), std::max() に置き換えようにも、あいまいさの解決ができない箇所が存在するので、
// MFC のヘッダーが修正されないかぎり無理。
// 現状、ユーザーコードでは (std::min)(x, y) や (std::max)(x, y) などのように、
// プリプロセッサによるマクロ展開を無効にするテクニックを使うとよい。
// min(x, y) や max(x, y) の形にならなければよいだけなので、
// std::min<T>(x, y) や std::max<T>(x, y) のように、テンプレート型引数を明示的に指定する方法でも可。
// BOOST_PREVENT_MACRO_SUBSTITUTION のようなオブジェクト形式マクロを挟んで、関数形式マクロによる置換を阻害する方法もあるが、
// 可読性が損なわれる。
// GDI+ のヘッダーに関しては、Windows 10 SDK version 2104 で修正されている。
// https://developercommunity.visualstudio.com/t/GdiplusTypesh-does-not-compile-with-NOM/727770
#if 0
#define NOMINMAX
#include <algorithm>
using std::min;
using std::max;
#endif

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Windows ヘッダーから使用されていない部分を除外します。
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 一部の CString コンストラクタは明示的です。

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>



// TODO: プログラムに必要な追加ヘッダーをここで参照してください。

#include "PublicInclude/ForPCHeader.hpp"

// HRESULT translation for Direct3D10 and other APIs
//#include <dxerr.h>
#include "dxerr.h"
// dxerr.h, dxerr.lib は Windows SDK 8.0 では廃止されている。
// 代替のコードが MSDN ブログで公開されている。
// http://blogs.msdn.com/b/chuckw/archive/2012/04/24/where-s-dxerr-lib.aspx

#include "PublicInclude/SuppressWarning.hpp"
#include "PublicInclude/FatalWarning.hpp"
//#include "PublicInclude/ImportMsXml.hpp"
