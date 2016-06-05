#pragma once

#if 0

// SDKDDKVer.h をインクルードすると、利用できる最も上位の Windows プラットフォームが定義されます。

// 以前の Windows プラットフォーム用にアプリケーションをビルドする場合は、WinSDKVer.h をインクルードし、
// SDKDDKVer.h をインクルードする前に、サポート対象とするプラットフォームを示すように _WIN32_WINNT マクロを設定します。

#include <SDKDDKVer.h>

#else

// 以下のマクロは、最低限必要なプラットフォームを定義します。最低限必要なプラットフォームは、
// アプリケーションを実行するのに必要な最小バージョンの Windows や Internet Explorer などです。
// このマクロは、利用可能なプラットフォームのバージョンで実行できるすべての機能を有効にします。
// プラットフォームのバージョンを指定することもできます。

// 下で指定された定義の前に対象プラットフォームを指定しなければならない場合、以下の定義を変更してください。
// 異なるプラットフォームに対応する値に関する最新情報については、MSDN を参照してください。

#ifndef WINVER                          // 最低限必要なプラットフォームとして Windows Vista が指定されています。
//#define WINVER 0x0600           // これを Windows の他のバージョン向けに適切な値に変更してください。
#define WINVER 0x0601 // Windows 7
#endif

#ifndef _WIN32_WINNT            // 最低限必要なプラットフォームとして Windows Vista が指定されています。
//#define _WIN32_WINNT 0x0600     // これを Windows の他のバージョン向けに適切な値に変更してください。
#define _WIN32_WINNT 0x0601 // Windows 7
#endif

#ifndef _WIN32_WINDOWS          // 最低限必要なプラットフォームとして Windows 98 が指定されています。
#define _WIN32_WINDOWS 0x0410 // これを Windows Me またはそれ以降のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_IE                       // 最低限必要なプラットフォームとして Internet Explorer 7.0 が指定されています。
//#define _WIN32_IE 0x0700        // これを IE の他のバージョン向けに適切な値に変更してください。
#define _WIN32_IE 0x0800
#endif

#endif
