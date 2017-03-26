#pragma once

// Windows ストア アプリでは GetTickCount() やマルチメディア タイマー timeGetTime() は使えないらしい。
// GetTickCount64() と高分解能パフォーマンス カウンター QueryPerformanceCounter() は使用可能。
#include <mmsystem.h>

#ifdef _DEBUG
#define _ATL_DEBUG
#define _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_QI
#endif

#include <comdef.h>
#include <atlbase.h>
#include <tchar.h>
#include <atlcoll.h>
#include <atlpath.h>
#include <atlimage.h> // HACK: ATL::CImage は GDI+ に依存しており、WinRT アプリでは使えないので、WIC などを代わりに使うようにする。
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <ppltasks.h> // Parallel Patterns Library (PPL), Concurrency Runtime

//using Microsoft::WRL::ComPtr;

//#define FBXSDK_NEW_API // K～, KFbx～ で始まる古いシンボルなどを使えなくする。2014.1 では完全に廃止されたらしい。
#define FBXSDK_SHARED // FBX SDK の DLL バージョンを使用する。
#include <fbxsdk.h> // Autodesk FBX SDK.
// 古いスタティック リンク バージョンの FBX SDK では、プロジェクト プリプロセッサ シンボルとして、
// あるいは stdafx.h で <fbxsdk.h> をインクルードする前に #define で定義するシンボルとして、
// KFBX_PLUGIN;KFBX_FBXSDK;KFBX_NODLL を定義しておく必要があった。
// K は Autodesk に買収された Kaydara の頭文字らしい。
// SDK バージョン 2014 の頃までは、Kaydara 由来のシンボル名の廃止・変更や大幅な API 仕様変更が頻繁に行なわれていた。


// DirectX Graphics
//#include <dxgi.h>
//#include <dxgi1_2.h> // DXGI 1.2: Windows 8, Windows 7 Platform Update
#include <dxgi1_3.h> // DXGI 1.3: Windows 8.1
//#include <dxgi1_4.h> // DXGI 1.4: Windows 10

// Direct3D 11
//#include <d3d11.h>
//#include <d3d11_1.h> // Direct3D 11.1: Windows 8, Windows 7 Platform Update
#include <d3d11_2.h> // Direct3D 11.2: Windows 8.1
// --> Windows 8.1 では D3D 11.2 が使えるようになる。ただし無印の Windows 8 にはバックポートされない。Windows 7 でも使えない。
//#include <d3d11_3.h> // Direct3D 11.3: Windows 10

//#include <d3dx11.h>
// 個別に <d3dx11async.h> や <d3dx11tex.h> をインクルードしても、結局 <d3dx11.h> をインクルードすることになる模様。
#include <d3dx11effect.h> // DX SDK June 2010, C++ Samples の Effects11 を改変したもの。

// Direct2D
//#include <d2d1_1.h> // Direct2D 1.1: Windows 8, Windows 7 Platform Update
#include <d2d1_2.h> // Direct2D 1.2: Windows 8.1

// DirectWrite
//#include <dwrite_1.h> // DirectWrite 1.1: Windows 8, Windows 7 Platform Update
#include <dwrite_2.h> // DirectWrite 1.2: Windows 8.1

// Windows Animation Manager (WAM)
#include <UIAnimation.h>
// WAM は Windows ストア アプリ開発でも使用可能らしい。
// また、Windows 7 Platform Update で更新されて Windows 8 用コンポーネントがバックポートされているらしい。
// Windows 8.1 でさらに更新されるらしい？
// http://msdn.microsoft.com/ja-jp/library/windows/apps/hh452772.aspx#windows_animation
// Xbox One では Xbox OS と Windows 8 OS の2つが共存されるらしく、
// Windows 8 用に開発された DirectX ストア アプリであればそのまま動作するらしい。
// D2D や DWrite だけでなく、WAM や WIC も使えるということ？
// http://www.inside-games.jp/article/2013/07/23/68805.html

// Windows Imaging Component (WIC)
#include <wincodec.h>
#include <wincodecsdk.h>

//#include <xnamath.h>
#include <DirectXMath.h> // Windows SDK 8.0 or later
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

// DirectXTK 付属の DirectXMath ラッパー。
// D3DX ライクな演算子オーバーロードの定義された派生クラスなどが用意されてある。
#include <SimpleMath.h>

// 以前のバージョンの DirectXTex は、Windows Vista/7 で利用する場合には CreateFile2() API まわりの関係で
// _WIN32_WINNT=0x0600/_WIN32_WINNT=0x0601 を明示的に定義する必要があったらしいが、現在（2013-08-13 版）はデフォルトで定義されている。
// ただし DirectXTex スタティック ライブラリのビルドに含まれない DDSTextureLoader などの補助クラスをコンパイルするときは注意が必要。
#include <DirectXTex.h>

// OpenGL も使用して、別ウィンドウに同時に同内容をレンダリングする。
// 他プラットフォームへの移植の検証用。
// 基本的に Windows プラットフォームでは OpenGL はパフォーマンス的にも開発効率的にも不利で、Direct3D に比べてほとんどメリットがないが、
// モバイルも含めたクロスプラットフォームなアプリケーションを作る場合は
// OpenGL であればコードをある程度共有できる。

//#define MY_FBX_ENABLES_OPENGL_SUPPORT

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
#include <GL/glew.h>
#include <GL/wglew.h>
//#include <GL/glut.h>
//#include <GL/glu.h>
#endif

#include <clocale>
#include <cstdint>
// VC 2010 コンパイラ＆ツールセットでは、Windows SDK の intsafe.h に関連して C4005 警告が出力される。
// VC 2012 コンパイラ＆ツールセットでは解決されているらしい。
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <random>
//#include <future>
// std::future, std::async など。C++11 で標準化された PPL のようなものだが、PPL のほうが慣れているのでそちらを使う。

//#include <boost/shared_ptr.hpp>
// VC 2008 では boost::shared_ptr だとインテリセンスが効かなくなることがある。TR1 を使う場合は OK。
// using とか using namespace を下手に使った場合も、やはりインテリセンスが効かなくなる。
// VC 2010 以降は解消されている？
#include <boost/format.hpp>
#include <boost/utility.hpp>
