#pragma once

// DOM や XPath を使う場合、XmlLite では不可能。MSXML が必要。
// 今後のサポートを考えると、MSXML 4.0 より MSXML 6.0 を選択すべき。
// Windows Vista / 7 以降、および Windows XP SP3 以降は MSXML 6.0 が標準でインストールされているらしい。
// なお、MSXML 6.0 に含まれる value 識別子は C++/CLI の予約語になっているので、
// 自動リネームを指定する。
// ちなみに x64 では、MSXML2::CLSID_DOMDocument40 などの MSXML 4.0 バージョンの CLSID が使用できない。
// 64bit 版の msxml4.dll は提供されていないため、
// x64 で MSXML 4.0 バージョンの COM コクラスを使用することが不可能だから。
// したがって、msxml6.dll を使うようにして、なおかつ MSXML2::CLSID_DOMDocument60 などの
// MSXML 6.0 バージョンの CLSID を使う必要がある模様。
// #import <msxml?.dll> ではなく、#include <msxml2.h> を使う場合でも状況は同じ。
// CLSID_MXXMLWriter40, CLSID_SAXXMLReader40, CLSID_XMLSchemaCache40 なども同様。

#import <msxml6.dll> named_guids auto_rename

// using namespace は使用しないこと。

// #import で COM DLL からメタデータをインポートすると、各メソッド実行時に戻り値が自動評価されて、
// エラー発生時には _com_error 例外が送出されるようになる。

// 一般的な COM API は SUCCEEDED() マクロあるいは FAILED() マクロで成功／失敗を検出できるが、
// XML DOM 関連のメソッドは一部それができない模様。

// C++/CX で XML を扱う場合、
// Windows.Data.Xml.Dom
// が最適？
// MSXML や XmlLite はストア アプリでは使えない？
// .NET の場合はデスクトップでもストア アプリでも System.Xml がベスト。
