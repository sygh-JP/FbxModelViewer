#include "stdafx.h"

// Visual Studio 2012 のプロジェクト ラウンド トリップを使って Visual C++ 2010 コンパイラを使用する場合、
// インテリセンスやコード ハイライトでは _MSC_VER が 2012 を示していても、実際には 2010 バージョンとなるので注意。
// おそらく IDE のバグ。
// ちなみに「プラットフォーム ツールセット」で「Windows7.1SDK」を選択すると、
// VC 2010 コンパイラが使用される模様。


#include "../SharedUtility/PublicInclude/MyFbxLinkLibDesc.hpp"


#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

#pragma region // Direct3D が要求するインポート ライブラリ。//

//#pragma comment(lib, "dxerr.lib") // Windows SDK 8.0 の DirectX API では廃止されている。
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "d3dcompiler.lib") // Windows 8.0 用のストア アプリでは使えない。8.1 では使える。

// GitHub および CodePlex で Effects11, DirectXTK, DirectXTex が公開されている。
// https://github.com/Microsoft/FX11
// https://github.com/Microsoft/DirectXTK
// https://github.com/Microsoft/DirectXTex
// https://fx11.codeplex.com/
// https://directxtk.codeplex.com/
// https://directxtex.codeplex.com/
// しかし、CodePlex からダウンロードした 2016-04-26 版 Effects11 をビルドしてリンクしたところ、
// Windows 7 上でレンダリングが一切なされない現象が発生。
// しかたないのでとりあえず DirectX SDK June 2010 付属サンプルの Effects11 をカスタマイズしたもの (Compact Effects11) に再び戻した。
// もともと deprecated 扱いの Effects11 なので、これ以上こだわっても仕方がない。
// 早めに捨てたほうがよさそう。

// Effects11 のスタティック ライブラリのリンク指定をプロジェクト設定で行なう場合、
// "Effects11_md2012_$(Platform)_$(Configuration).lib"
// "Effects11_md2010_$(Platform)_$(Configuration).lib"
// などとすると便利。
#if (_MSC_VER == 1900)

#if defined(_M_IX86)

#ifdef _DEBUG
#pragma comment(lib, "Desktop_2015\\Win32\\Debug\\Effects11.lib")
#pragma comment(lib, "Desktop_2015\\Win32\\Debug\\DirectXTex.lib")
//#pragma comment(lib, "Desktop_2015\\Win32\\Debug\\DirectXTK.lib")
#else
#pragma comment(lib, "Desktop_2015\\Win32\\Release\\Effects11.lib")
#pragma comment(lib, "Desktop_2015\\Win32\\Release\\DirectXTex.lib")
//#pragma comment(lib, "Desktop_2015\\Win32\\Release\\DirectXTK.lib")
#endif

#elif defined(_M_X64)

#ifdef _DEBUG
#pragma comment(lib, "Desktop_2015\\x64\\Debug\\Effects11.lib")
#pragma comment(lib, "Desktop_2015\\x64\\Debug\\DirectXTex.lib")
//#pragma comment(lib, "Desktop_2015\\x64\\Debug\\DirectXTK.lib")
#else
#pragma comment(lib, "Desktop_2015\\x64\\Release\\Effects11.lib")
#pragma comment(lib, "Desktop_2015\\x64\\Release\\DirectXTex.lib")
//#pragma comment(lib, "Desktop_2015\\x64\\Release\\DirectXTK.lib")
#endif

#else
#error Not supported platform!!
#endif

#elif (_MSC_VER == 1800)


#elif (_MSC_VER == 1700)


#endif

#pragma endregion


#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "WindowsCodecs.lib") // WIC


// VC 2010 コンパイラと Windows SDK 7.x を使わざるをえない状況が発生して、
// Windows SDK 8.0 よりも旧 DirectX SDK June 2010 をどうしても優先したい場合は、
// 例えば Win32 の [構成プロパティ]→[VC++ ディレクトリ] に対して、
// [実行可能ファイル ディレクトリ] に "$(DXSDK_DIR)Utilities\bin\x86;$(ExecutablePath)" を、
// [インクルード ディレクトリ] に "$(DXSDK_DIR)Include;$(IncludePath)" を、
// [ライブラリ ディレクトリ] に "$(DXSDK_DIR)Lib\x86;$(LibraryPath)" を指定しておくこと。
// 
// なお、VS 2012 インストール後は、.user.props ファイルによるグローバル設定が VS 2010 と共有されるため、
// 既定値であるグローバル追加ディレクトリの設定は、必ず Windows SDK > 旧 DirectX SDK の順とする必要がある。
// グローバル設定で旧 DirectX SDK の優先度を上げることは絶対にしないようにすること。


#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT

#pragma region // OpenGL が要求するインポート ライブラリ。//

#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib") // glut.h をインクルードしない場合、明示する必要がある。
//#pragma comment(lib, "glu32.lib") // OpenGL 3.x 以降で廃止される固定機能を使わないのであれば、GLU はもはや不要。

#pragma endregion

#endif


#pragma comment(lib, "MyWpfGraphLibMfc.lib")
