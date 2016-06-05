#pragma once

// コンパイラのバージョン（ランタイム実装）に依存しない、メッシュ階層構造を抽象化する COM ライクのインターフェイス群を定義する。
// FBX SDK およびそれを使用したローダーのコードと、アプリケーションのコードを分離する目的。
// オブジェクトの生成にはファクトリ関数を用意する。

#include "../SharedUtility.h"

class MySimpleAbiStringWImpl;

// コンパイラのバージョンに依存しない、可変長文字列クラス。
// いわゆる ABI 境界（DLL 境界）を越える目的。
// .NET の System::String や、WinRT の Platform::String の要領で使える。
// ただし文字列の連結などの機能は一切提供しない。
// COM の BSTR および BSTR ラッパークラスは Windows 依存になるので使わない。
class SHAREDUTILITY_API MySimpleAbiStringW final
{
	MySimpleAbiStringWImpl* m_pImpl;
public:
	MySimpleAbiStringW();
	~MySimpleAbiStringW();
	explicit MySimpleAbiStringW(const wchar_t* str);
	MySimpleAbiStringW(const MySimpleAbiStringW& other);
	MySimpleAbiStringW& operator=(const MySimpleAbiStringW& other);

	size_t GetLentgh() const;
	wchar_t* GetBuffer();
	const wchar_t* GetBuffer() const;
};
