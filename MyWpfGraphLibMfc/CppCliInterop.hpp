#pragma once
// C++/CLI プロジェクト（/clr オプション有効）でのみ使用されるシンボルやディレクティブを定義します。

#ifdef __cplusplus_cli

// NOTE: MFC/WPF 相互運用の注意点 1:
// C++/CLI（/clr）でコンパイルする（CLR のサポートを追加する）と、リンク時に、
// 「warning LNK4248: 未解決の typeref トークン (...) ('_TREEITEM') です。イメージを実行できません。」
// 「warning LNK4248: 未解決の typeref トークン (...) ('_IMAGELIST') です。イメージを実行できません。」
// という警告が発生する。
// Windows SDK の CommCtrl.h で前方宣言とポインタのシノニムしか定義されていないせいで、
// MSIL の制約に引っかかるために起こる。
// 回避するためには、HTREEITEM や HIMAGELIST を参照している部分を unmanaged でコンパイルするか、
// 空のダミー構造体を代わりに定義する。
// 実際に Windows API 内部で使用されている構造体とは異なる型を定義するわけだが、
// Windows API では前方宣言とポインタしか公開されていないので問題ない。

struct _TREEITEM {};
struct _IMAGELIST {};

// なお、WPF 相互運用するためには、MFC プロジェクト側で少なくとも下記アセンブリ群を参照設定に追加する必要がある。
// ・System
// ・WindowsBase
// ・PresentationCore
// ・PresentationFramework


// NOTE: MFC/WPF 相互運用の注意点 2:
// MFC 拡張 DLL のプロジェクトで、混合コードのために gcroot などを使う目的で、
// stdafx.h 内で <vcclr.h> をインクルードすると、
// プロジェクト設定で /clr コンパイル オプションを指定していても、
// dllmain.cpp をコンパイル中に C1190 のエラーが発生する。
// これは、dllmain.cpp には DllMain() の定義があるため、ファイル レベルで常に
// 「共通言語ラインタイム サポートを使用しない」設定になっていることが原因。
// dllmain.cpp に対して無理に /clr を使うと、コンパイル時に
// 「warning C4747: マネージ 'DllMain' を呼び出します: マネージ コードは、
// DLL エントリポイントおよび DLL エントリポイントから到達した呼び出しを含むローダー ロック下では実行できません」
// という警告が発生し、DLL をロードできなくなる。
// したがって、stdafx.h で <vcclr.h> をインクルードしないようにするか、
// __cplusplus_cli 定義の有無でインクルードを制御する必要がある。
// なお、なんらかの条件で、VS 2012 では C++/CLI インテリセンスが正しく動作しなくなるバグがある模様。
// 特にネイティブ型まわりが全滅する。
// バッチ ビルドを実行すると現象が発生しやすい模様。
// どうやら、stdafx.h と異なるディレクトリにソース ファイルを置いて、先頭に
// #include "stdafx.h"
// を記述しただけの場合に発生する模様。
// #include "../stdafx.h"
// などのように、インテリセンス用に冗長なインクルードを追加で記述しておくとよさげ。
// stdafx.h はプロジェクト ファイル（.vcxproj）と同一階層にあるので、ファイル名だけ指定してもコンパイルは通るし、
// ネイティブ C++ オンリーだとインテリセンスもきちんと機能するが、/clr によって C++/CLI が入るとおかしな現象が発生する。
// おそらく Visual Studio の C++/CLI 関連機能は重要度が低く、出荷前にあまりテストされていないことが原因と思われる。
// ちなみにコード エディターのコンテキスト メニュー コマンド [ドキュメント "XXX" を開く] も、
// stdafx.h と異なるディレクトリにあるソース ファイルにおける
// #include "stdafx.h"
// に対しては正しく機能しない。


// NOTE: MFC/WPF 相互運用の注意点 3:
// System::Windows::Interop::MSG 値クラスと Win32 の MSG 構造体がバッティングするので、
// Win32/MFC 相互運用プロジェクトでは実質 using ディレクティブは使わないほうがいい。
// ブロック内で限定的に using ディレクティブを使用するか、using 宣言にとどめること。


// NOTE: MFC/WPF 相互運用の注意点 4:
// WPF Toolkit, Extended WPF Toolkit における XamlGeneratedNamespace::GeneratedInternalTypeHelper の多重インポートにより、
// C4945 の警告が出るので、とりあえず「C/C++ --> 詳細 --> 指定の警告を無効にする」に指定する。


// NOTE: MFC/WPF 相互運用の注意点 5:
// それなりに規模の大きい MFC プロジェクトなどで、共通言語ランタイムのサポート /clr を追加すると、
// 起動時に EETypeLoadException 例外が発生して実行できない場合、/GF（同一文字列の削除）を追加する。
// VS 2008 IDE からは、「構成プロパティ」→「C/C++」→「コード生成」→「文字列プール」を「はい」にする。
// 特にデバッグ ビルドで最適化を行なわない場合はこのオプションが OFF になっていることが多いので注意。
// 特定の最適化オプションを有効にすると自動で ON になることもあるらしい。


// NOTE: MFC/WPF 相互運用の注意点 6:
// C++/CLI 混合コード（ネイティブ・マネージ混合コード）からは、64bit ネイティブ（x64）の MFC 拡張 DLL 内部には
// デバッガでステップインできない模様。内部にブレークポイントも置けない。
// 例えば｛/clr を有効にした x64 MFC EXE プロジェクト｝＋｛/clr を無効にした x64 MFC 拡張 DLL プロジェクト｝などが該当する。
// x64 MFC 拡張 DLL を /clr でコンパイルすると、混合コードからでもステップインできるようになる。
// （ただし /clr でビルドされた DLL を、同じソリューション内の別プロジェクトから
// リンカ オプション「リンク ライブラリの依存関係」で自動リンクさせようとしても
// うまくいかないようなので注意。明示的にインポート ライブラリをリンカ指定する必要がある）
// また、混合コードにリンクされている 64bit ネイティブの MFC 拡張 DLL 内部で
// ASSERT（あるいは ATLASSERT や _ASSERTE など）が失敗したとき、「再試行」を実行すると、
// プロセスが例外コード 0x80000003 でクラッシュする現象が発生する。
// MFC EXE プロジェクト プロパティの、「構成プロパティ ==> デバッグ ==> デバッガのタイプ」を、
// 「自動」ではなく「ネイティブのみ」にすると、64bit ネイティブ コードにステップインできるようになる。
// ASSERT 失敗時のクラッシュも発生しない。
// ただし、その場合混合コードにはステップインできなくなるので注意。
// なお、32bit（Win32, x86）では、デバッガのタイプが「自動」設定のままで普通にステップイン可能。
// ちなみにデバッガのタイプ設定は、ユーザーごと（開発者ごと）のプロジェクト オプション ファイル（.user ファイル）に
// 保存され、VC++ プロジェクト ファイルには保存されないので注意。


// NOTE: MFC/WPF 相互運用の注意点 7:
// VS 2008 SP1 の AFX_GLOBAL_DATA::GetNonClientMetrics() はバグっているらしく、
// CLR サポートを有効にする（/clr オプションを有効にする）と、Windows Vista 以降では
// スタック オーバーフロー エラー（STATUS_STACK_BUFFER_OVERRUN）が発生する。
// 代わりに SystemParametersInfo() Win32 API を直接呼び出して対処すること。
// ちなみにこのバグっている API は、MFC AppWizard が生成するコード CPropertiesWnd::SetPropListFont() で使われている。
// VS 2010 以降ではバグが解消されているらしい。


// NOTE: MFC/WPF 相互運用の注意点 8:
// .NET アセンブリを C++ アプリケーションから /clr を使ってロードする場合、
// Win32 (32bit) の C++ プロジェクト構成に対しては .NET 側の [プラットフォーム ターゲット] を [x86] に指定しておく。[Any CPU] は不可。
// x64 (64bit) の C++ プロジェクト構成に対しては .NET 側の [プラットフォーム ターゲット] を [Any CPU] に指定しておく。[x64] ではない。


#include <vcclr.h>
#using <mscorlib.dll>

//using namespace System;
//using namespace System::Windows::Interop; // Win32 API と相互運用する場合、using, using namespace は厳禁。
//using System::Windows::Interop::HwndSource;
//using System::Windows::Interop::HwndSourceParameters;

#endif
