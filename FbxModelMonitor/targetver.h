// この MFC サンプル ソース コードでは、MFC Microsoft Office Fluent ユーザー インターフェイス 
// ("Fluent UI") の使用方法を示します。このコードは、MFC C++ ライブラリ ソフトウェアに 
// 同梱されている Microsoft Foundation Class リファレンスおよび関連電子ドキュメントを
// 補完するための参考資料として提供されます。
// Fluent UI を複製、使用、または配布するためのライセンス条項は個別に用意されています。
// Fluent UI ライセンス プログラムの詳細については、Web サイト
// http://msdn.microsoft.com/officeui を参照してください。
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

#pragma once

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

