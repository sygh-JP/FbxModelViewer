#pragma once

// MFC アプリケーション ウィザードは、デバッグ ビルドにおいて new キーワードを DEBUG_NEW に置換することで
// トラッキング（デバッグ用のメモリーアロケータ）が有効になる [A] のコードを埋め込むが、これは本来は規約違反。
// 言語のキーワード（予約語）を #define の左辺値にして置換することは NG。
// 本来は [B] を定義して、MFC アプリケーション プログラマーに対して new ではなく NEW を使うことを推奨するべきだった。
// http://msdn.microsoft.com/ja-jp/library/cc440188(v=vs.71).aspx
// https://web.archive.org/web/20140301090452/http://msdn.microsoft.com:80/ja-jp/library/cc440188(v=vs.71).aspx


///////////////////////////////////////////////////////////////////////////////
#pragma region // [A]
#ifdef _MFC_VER

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#endif
#pragma endregion

///////////////////////////////////////////////////////////////////////////////
#pragma region // [B]
#ifndef _MFC_VER

// VC2015 Update3 では、<vcruntime_new_debug.h> にて、デバッグ情報を受け取る
// operator new のオーバーロードが定義されている。
// MFC では、<afx.h> にて DEBUG_NEW が定義されている。
// 非 MFC の場合、DEBUG_NEW を自前で定義する。
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#endif

// MSVC の CRT では、_CRTDBG_MAP_ALLOC を定義して <crtdbg.h> をインクルードすると、
// malloc, free などをマクロ定義のデバッグ レイヤーに置き換えるようになる。
// 自前で定義する必要はない。

#ifdef _DEBUG
//#define DEBUG_MALLOC(size) _malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define DEBUG_CALLOC(count, size) _calloc_dbg(count, size, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define DEBUG_REALLOC(memory, newSize) _realloc_dbg(memory, newSize, _NORMAL_BLOCK, __FILE__, __LINE__)

#define NEW DEBUG_NEW
//#define MALLOC DEBUG_MALLOC
//#define CALLOC DEBUG_CALLOC
//#define REALLOC DEBUG_REALLOC
#else
#define NEW new
//#define MALLOC malloc
//#define CALLOC calloc
//#define REALLOC realloc
#endif
#pragma endregion
