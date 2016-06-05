#pragma once

// FBX にも Direct3D にも OpenGL にも依存しないユーティリティを記述する。
// ただし CRT、Boost、WinAPI および MFC/ATL には依存する。

// NOTE: 文字列クラスは移植性よりも速度・メモリ効率や機能性が必要な部分や、MFC/ATL との境界面には、MFC/ATL の CString 系列を使う。
// なお、Windows ストア アプリでは ATL の CStringW, CPathW は使えるが、CStringA, CPathA が使えない。
// また、ANSI マルチバイト文字列が廃止されている。
// 明示的に ASCII 文字列もしくは UTF-8 文字列を扱う場合、std::string での置き換えを行なう必要がある。
// 特に必要がなければ、他の環境への移植性も考慮してストレージ用途にはできるかぎり std::wstring を使っておくのがよさげ。
// ちなみにストア アプリでも OutputDebugStringA() は使えるが、リリース ビルドで直接使ってしまうと認証キットが NG 判定するので注意。

#include "MyUtilTypes.hpp"


namespace MyUtil
{
	struct StdFileDeleter
	{
		void operator()(FILE* fp) const
		{
			if (fp)
			{
				fclose(fp);
			}
		}
	};

	using TStdFileUniquePtr = std::unique_ptr<FILE, StdFileDeleter>;


	// char の符号は処理系依存なため、オーバーロードを作成できることに注意。
	extern bool LoadBinaryFromFile(LPCWSTR pFileName, std::vector<char>& buffer);
	extern bool LoadBinaryFromFile(LPCWSTR pFileName, std::vector<int8_t>& buffer);
	extern bool LoadBinaryFromFile(LPCWSTR pFileName, std::vector<uint8_t>& buffer);

	// Windows API の TCHAR 用に MyUtil::tstring, tformat を定義。
	// std 名前空間を汚染しないように、std::tstring とはしない。
	typedef std::basic_string<TCHAR> tstring;
	typedef boost::basic_format<TCHAR> tformat;

#pragma region // プリミティブ型のシノニム。C# や MSIL の組込型に似せてある。要は signed, unsigned をいちいち付けたくないだけ。//
	// C++11 対応環境では、stdint.h, cstdint ヘッダーで定義されている型を使うべき。//
#if 0
	typedef signed char Int8;
	typedef unsigned char UInt8;
	typedef signed char SByte;
	typedef unsigned char Byte;

	typedef signed short Int16;
	typedef unsigned short UInt16;
	typedef signed short Short;
	typedef unsigned short UShort;

	typedef signed int Int32;
	typedef unsigned int UInt32;
	typedef signed int Int;
	typedef unsigned int UInt;

	typedef signed long Long32;
	typedef unsigned long ULong32;

	typedef signed long long Int64;
	typedef unsigned long long UInt64;
#endif

#if 0
	typedef signed char sbyte;
	typedef unsigned char byte;

	typedef unsigned int uint;
	typedef unsigned short ushort;
	typedef unsigned long ulong32;
	typedef unsigned long long ulong64;
#endif

#pragma endregion

} // end of namespace


// std::string, std::wstring を返す。
// <boost/format.hpp> のインクルードが必要。
// Boost.Format() は Legacy C の printf() 系や MFC/ATL の CString::Format() と比べて型安全でありながら、
// かつ C++ の stream 系よりも記述しやすいのが利点。
// また、CStringA が使えない Windows ストア アプリでの文字列フォーマットにも役立つ。
// ただし、MSVC 拡張である %S 書式による ANSI - Unicode 相互変換は使えないので注意。
#define STRINGA_FORMAT(asf, pr)  boost::io::str(boost::format(asf) pr)
#define STRINGW_FORMAT(wsf, pr)  boost::io::str(boost::wformat(wsf) pr)
#define STRINGT_FORMAT(tsf, pr)  boost::io::str(MyUtil::tformat(tsf) pr)


// シンボルの文字列リテラル化。
#ifndef SYMBOL2STRING
#if defined UNICODE || defined _UNICODE
#define SYMBOL2STRING(s) L#s
#else
#define SYMBOL2STRING(s) #s
#endif
#endif

#ifndef SYMBOL2STRINGW
#define SYMBOL2STRINGW(s) L#s
#endif

#ifndef SYMBOL2STRINGA
#define SYMBOL2STRINGA(s) #s
#endif


// .NET の System.Char のメソッドに似せてある。
namespace MyCharHelpers
{
	inline bool IsHighSurrogate(wchar_t c)
	{
		return (0xD800 <= c && c <= 0xDBFF);
	}

	inline bool IsLowSurrogate(wchar_t c)
	{
		return (0xDC00 <= c && c <= 0xDFFF);
	}

	inline bool IsSurrogate(wchar_t c)
	{
		return IsHighSurrogate(c) || IsLowSurrogate(c);
	}

	inline bool IsSurrogatePair(wchar_t x, wchar_t y)
	{
		return IsHighSurrogate(x) && IsLowSurrogate(y);
	}


	class UnicodePair
	{
	public:
		wchar_t X, Y;
	public:
		UnicodePair()
			: X(), Y()
		{
		}
		explicit UnicodePair(wchar_t x, wchar_t y = 0)
			: X(x), Y(y)
		{
		}
	public:
		// 同値判定用の演算子オーバーロード。
		// .NET の System.Object.Equals() に相当。
		// unordered_set/unordered_map ではハッシュ値とともに使われる。
		bool operator==(const UnicodePair& rval) const
		{
			return this->X == rval.X && this->Y == rval.Y;
		}
		bool operator!=(const UnicodePair& rval) const
		{
			return !(*this == rval);
		}
		bool operator==(wchar_t rval) const
		{
			return this->Y == 0 && this->X == rval;
		}
		bool operator!=(wchar_t rval) const
		{
			return !(*this == rval);
		}
	public:
		// .NET の System.Object.GetHashCode() に相当。
		size_t GetHashCode() const
		{
			return this->X ^ this->Y;
		}

		// ハッシュ値取得用の関数オブジェクト。
		struct HashFunctor
		{
			size_t operator ()(const UnicodePair& v) const { return v.GetHashCode(); }
		};
	};

#if 0
	// std::function<decltype(T)> と組み合わせて使う。
	// std::unordered_map のハッシュ関数として定義する場合、クラスの static メンバーにしてはダメで、
	// グローバル関数として定義してやらないといけないらしい？
	inline size_t GetUnicodePairHashCode(const UnicodePair& key)
	{
		return key.GetHashCode();
	}
#endif
} // end of namespace

#if 0
namespace std
{
	// 独自型を std::unordered_map のキーとして使うために、
	// 独自型のハッシュ計算関数として std::hash<T> の特殊化を定義する方法がある。
	// std 名前空間を汚染したくない場合、
	// std::unordered_map テンプレート第3引数に
	// size_t operator ()(const TKey&) const;
	// を定義した関数オブジェクトの型を渡す方法もある。
	// どちらかというと関数オブジェクトを使うほうが独立性が高く、適用範囲や影響を限定できるが、
	// map と unordered_map を切り替えてパフォーマンス テストするときはやや面倒になる。
	template<> struct hash<MyCharHelpers::UnicodePair>
	{
		std::size_t operator()(const MyCharHelpers::UnicodePair& key) const
		{
			return key.GetHashCode();
		}
	};

	// なお、独自型の比較関数として std::equal_to<T> の特殊化を定義する方法がある。
	// std 名前空間を汚染したくない場合、
	// std::unordered_map テンプレート第4引数に
	// bool operator ()(const TKey&, const TKey&) const;
	// を定義した関数オブジェクトの型を渡す方法もある。
	// あるいは、独自型のメンバーとして
	// bool operator ==(const TKey&) const;
	// を定義するだけでもよい。
} // end of namespace
#endif

namespace MyUtil
{
	// メソッドの戻り値で文字列クラスへの const 参照（const ポインタではない）を返したりする必要があるとき、
	// null 値の代替として空文字列の定数オブジェクトを用意しておく。
	// メソッド ブロック内の static const ローカル変数でもいいが、共通グローバル定数オブジェクトを作っておいたほうがよい。
	// なお、グローバル定数オブジェクトを定義して、それを使ってクラスのメンバー変数を初期化したりする場合、
	// そのクラスのインスタンスをグローバル変数にしてはいけない。
	// グローバル変数の初期化順序の問題に引っかかる。
	// もしどうしてもグローバル変数にする場合、グローバル定数オブジェクトではなく、
	// 静的ローカル変数への参照を返すグローバル メソッドを定義して、初期化時にそれを呼び出すようにする。

	const std::string EmptyStdStringA;
	const std::wstring EmptyStdStringW;


	extern std::string ConvertUtf16toUtf8(const wchar_t* srcText);
	extern std::wstring ConvertUtf8toUtf16(const char* srcText);

	inline std::string SafeConvertUtf16toUtf8(const wchar_t* srcText)
	{ return srcText ? MyUtil::ConvertUtf16toUtf8(srcText) : ""; }

	inline std::wstring SafeConvertUtf8toUtf16(const char* srcText)
	{ return srcText ? MyUtil::ConvertUtf8toUtf16(srcText) : L""; }


	//! @brief  Windows Unicode (UTF-16) のパスに対応する UTF-8 エンコード MBCS の絶対ファイルパスを取得する。<br>
	//! 
	//! Windows ANSI Code Page (日本語では Windows Shift-JIS, CP932) のパスに対応する <br>
	//! UTF-8 エンコード MBCS の絶対ファイルパスを取得したい場合、呼び出し側でいったん UTF-16 のワイド文字列に <br>
	//! 変換しておくこと。<br>
	extern bool GetUtf8FullPath(const wchar_t* inRelFilePath, std::string& outFullPath);


	template<typename T> inline T SquareVal(const T& x)
	{ return x * x; }

	template<typename T> inline T CubeVal(const T& x)
	{ return x * x * x; }

	template<typename T> inline T ToPowerOf4(const T& x)
	{ return x * x * x * x; }


	template<typename T> inline void Swap(T& a, T& b)
	{
		T c = a;
		a = b;
		b = c;
	}

#if 0
	//! @brief  GDI+ や MFC アプリとコードを共有する場合の、std::min() の代替。NaN 非対応。<br>
	template<typename T> inline const T& MinVal(const T& a, const T& b)
	{
		return (a <= b) ? a : b;
	}

	//! @brief  GDI+ や MFC アプリとコードを共有する場合の、std::max() の代替。NaN 非対応。<br>
	template<typename T> inline const T& MaxVal(const T& a, const T& b)
	{
		return (a >= b) ? a : b;
	}
#endif

	template<typename T> inline const T& Clamp(const T& a, const T& minVal, const T& maxVal)
	{
		if (a < minVal) return minVal;
		if (a > maxVal) return maxVal;
		return a;
	}

	template<typename T> inline void LogicalNot(T& n)
	{
		n = !n;
	}

	template<typename T> inline void SafeDelete(T*& p)
	{
		// C++ の仕様では NULL ポインタに delete を実行しても何も起こらないということになっている。
		// なので NULL チェックは行なわない。
		delete p;
		p = nullptr;
	}

	template<typename T> inline void SafeFree(T*& ptr)
	{
		// C/C++ の仕様では NULL ポインタに free() を実行しても何も起こらないということになっている。
		// なので NULL チェックは行なわない。
		free(ptr);
		ptr = nullptr;
	}

	template<typename T> inline void SafeRelease(T*& ptr)
	{
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
	}

	inline void SafeFClose(FILE*& fp)
	{
		if (fp)
		{
			fclose(fp);
			fp = nullptr;
		}
	}


	template<typename T> T* StaticPointerCast(void* pIntermediate)
	{ return static_cast<T*>(pIntermediate); }

	template<typename T> const T* StaticPointerCast(const void* pIntermediate)
	{ return static_cast<const T*>(pIntermediate); }


	class AltFinally final
	{
		using TFinalFunc = std::function<void()>;
		TFinalFunc m_finalFunc;
	public:
		explicit AltFinally(const TFinalFunc& finalFunc)
			: m_finalFunc(finalFunc)
		{}
		~AltFinally()
		{
			if (m_finalFunc)
			{
				m_finalFunc();
			}
		}
	};

} // end of namespace


namespace MyAlgHelpers
{
	// コンテナ／コレクションの全要素を検査するアサーションを書く場合、
	// C++ であれば STL アルゴリズムとラムダ式、
	// C# であれば LINQ メソッド構文とラムダ式を使えばインラインで書けるので有用。
	// ジャグ配列の検査などで効果的。

	template<typename T> bool CheckAllElemSizesInJaggedArrayAreSame(const std::vector<std::shared_ptr<std::vector<T>>>& inJagged, size_t targetSize)
	{
		typedef typename decltype(*inJagged.begin()) TValue;
		return std::find_if(inJagged.begin(), inJagged.end(),
			[targetSize] (const TValue& pSubContainer)
		{ return !pSubContainer || (pSubContainer->size() != targetSize); })
		== inJagged.end();
	}
}
