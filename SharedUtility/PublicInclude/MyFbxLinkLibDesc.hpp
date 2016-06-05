#pragma once

#if (_MSC_VER == 1900)
#pragma message("VC++ 2015 compiler is used.")
#define FBX_LIB_VS_DIR_NAME "vs2015"
#elif (_MSC_VER == 1800)
#pragma message("VC++ 2013 compiler is used.")
#define FBX_LIB_VS_DIR_NAME "vs2013"
#elif (_MSC_VER == 1700)
#pragma message("VC++ 2012 compiler is used.")
#define FBX_LIB_VS_DIR_NAME "vs2012"
#endif

#pragma region // FBX SDK が要求するインポート ライブラリ。//
// "C:\Program Files\Autodesk\FBX\FBX SDK\yyyy.v\lib" までをグローバル ディレクトリ設定（.user.props）に追加しておくこと。

#ifdef _M_IX86

#ifdef _DEBUG
#pragma comment(lib, FBX_LIB_VS_DIR_NAME "\\x86\\debug\\libfbxsdk.lib")
#else
#pragma comment(lib, FBX_LIB_VS_DIR_NAME "\\x86\\release\\libfbxsdk.lib")
#endif

#elif defined(_M_X64)

#ifdef _DEBUG
#pragma comment(lib, FBX_LIB_VS_DIR_NAME "\\x64\\debug\\libfbxsdk.lib")
#else
#pragma comment(lib, FBX_LIB_VS_DIR_NAME "\\x64\\release\\libfbxsdk.lib")
#endif

#else
#error Not supported platform!!
#endif

#pragma comment(lib, "wininet.lib")

#pragma endregion
