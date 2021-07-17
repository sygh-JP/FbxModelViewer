#include "stdafx.h"
#include "MyUtil.h"

#include "DebugNew.h"


namespace MyUtils
{
	template<typename T> void LoadBinaryFromFileImpl(LPCWSTR pFilePath, std::vector<T>& outBuffer)
	{
		//static_assert((sizeof(T) == 1), "Unsupported size!!");

		// あらかじめクリアしておく。
		outBuffer.clear();

		// ファイル サイズの取得には Windows API の GetFileSizeEx() や FindFirstFile() を使う方法もあるが、他 OS への移植性に劣るので使わない。
		// fseek(), ftell() は論外。
		// https://www.jpcert.or.jp/sc-rules/c-fio19-c.html
		// POSIX ベースの stat を使うことにする。
		// _stat32(), _wstat32(), _stat32i64(), _wstat32i64() はタイムスタンプが 2038-01-18 23:59:59 までしか扱えないので論外。
		// _stat64i32(), _wstat64i32() は 0x7FFFFFFF [bytes] (2GB - 1B) までのファイルしか正常に扱えない。
		// 2GB 以上のファイルも正常に扱えるようにするには _stat64(), _wstat64() を使う必要がある。

#if defined(_M_X64) || defined(_M_ARM64)
		// 64bit ビルドの判定は _WIN64 でもよさそう。
		// https://msdn.microsoft.com/en-us/library/b0084kay.aspx
		// ちなみに x86 や ARM では _WIN64 は定義されないが、x64 と ARM64 では _WIN64 と _WIN32 の両方が定義されるので注意。
#elif defined(_M_IX86) || defined(_M_ARM)
#else
#error Not supported platform!!
#endif
		// 構造体と同名の関数が存在するので、C++ でも struct で修飾する必要がある。
		struct _stat64 fileStats = {};
		const auto getFileStatFunc = _wstat64;

		// ファイル サイズが取得できない、もしくはサイズが負だった場合はエラー。
		if (getFileStatFunc(pFilePath, &fileStats) != 0 || fileStats.st_size < 0)
		{
			throw std::exception("Cannot get the file stats for the file!!");
		}

		if (fileStats.st_size % sizeof(T) != 0)
		{
			throw std::exception("The file size is not a multiple of the expected size of element!!");
		}

		const auto fileSizeInBytes = static_cast<uint64_t>(fileStats.st_size);

		// MFC や GDI+ を使う場合は NOMINMAX を定義できず、邪魔というか邪悪な min/max マクロを無効化できない。
		// Windows SDK の <intsafe.h> で定義されている SIZE_T_MAX などを代替として使う方法もあるが、
		// コードレベルでの移植性を高めるために、小技を使ってマクロ展開を回避する。
		// http://d.hatena.ne.jp/yohhoy/20120115/p1

		// size_t で表現できない場合はエラー。
		if (sizeof(size_t) < 8 && (std::numeric_limits<size_t>::max)() < fileSizeInBytes)
		{
			throw std::exception("The file size is over the capacity on this platform!!");
		}

		if (fileStats.st_size == 0)
		{
			return;
		}

		const auto numElementsInFile = static_cast<size_t>(fileStats.st_size / sizeof(T));

		outBuffer.resize(numElementsInFile);

#if 0
		// std::fstream の場合、MSVC のデバッグ ビルドでは、数 MB のファイルを読み込むときに20秒以上の時間がかかる。
		// 従来の CRT ファイル入出力関数には、デバッグ ビルドであってもそこまでの致命的なオーバーヘッドはない。
		std::basic_ifstream<T> ifs(pFilePath, std::ios::in | std::ios::binary);

		if (ifs.fail())
		{
			throw std::exception("Cannot open the file!!");
		}

		ifs.seekg(0, std::fstream::end);
		const auto endPos = ifs.tellg();
		ifs.clear();
		ifs.seekg(0, std::fstream::beg);
		const auto begPos = ifs.tellg();
		const auto fileSize = endPos - begPos; // std::streamoff 型。VC2012 では 32bit 版でも long long になるらしい。
		static_assert(sizeof(fileSize) >= 8, "Size of pos_type must be greater than or equal to 8 bytes!!");
		static_assert(sizeof(fpos_t) >= 8, "Size of fpos_t must be greater than or equal to 8 bytes!!");

		ifs.read(&outBuffer[0], numElementsInFile);
#else
		FILE* pFile = nullptr;
		const auto retCode = _wfopen_s(&pFile, pFilePath, L"rb");
		if (retCode != 0 || pFile == nullptr)
		{
			throw std::exception("Cannot open the file!!");
		}
		fread_s(&outBuffer[0], numElementsInFile * sizeof(T), sizeof(T), numElementsInFile, pFile);
		fclose(pFile);
		pFile = nullptr;
#endif
	}

	template<typename T> bool LoadBinaryFromFileImpl2(LPCWSTR pFilePath, std::vector<T>& outBuffer)
	{
		try
		{
			LoadBinaryFromFileImpl(pFilePath, outBuffer);
			return true;
		}
		catch (const std::exception& ex)
		{
			const CStringW strMsg(ex.what());
			ATLTRACE(L"%s <%s>\n", strMsg.GetString(), pFilePath);
			return false;
		}
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<char>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<int8_t>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<uint8_t>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<int32_t>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<uint32_t>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<float>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}

	// std::wstring, ATL::CStringW, Platform::String などの初期化に使えるよう、ゼロ終端の std::vector<wchar_t> を返す。
	// ただし一時的にヒープが二重に作成されるので、効率はよくない。
	// 空の std::vector に対する std::vector::data() が返す値は未規定。MSVC では nullptr を返す。
	// なお、std::string や std::wstring に変換する際、nullptr は渡せない。
	// https://en.cppreference.com/w/cpp/container/vector/data
	// https://cpprefjp.github.io/reference/vector/vector/data.html
	// コピーコンストラクタよりも C++11 ムーブ コンストラクタが優先されることを考慮し、あえて引数ではなく戻り値で返す。
	// std::move() は書かないでよい。むしろ書くと RVO (Return Value Optimization) が阻害されることがあるらしい。
	// C++11 以降では、通例ローカル変数の型と戻り値の型が同じ場合、
	// NRVO (Named Return Value Optimization) が効かない場合でもコンパイラによって std::move() 呼び出しが自動追加されるらしい。
	// https://en.cppreference.com/w/cpp/language/copy_elision
	// https://en.cppreference.com/w/cpp/language/return
	// std::vector のムーブだとポインタのすげ替えが実行されるので、結果的にムーブ元とムーブ先とでポインタの値は変わらない。
	// しかし、std::string だと必ずしもポインタのすげ替えにはならない模様。
	// ムーブを実行した際、ムーブ先の文字列バッファ容量が十分な場合は、memmove() によるメモリブロックのコピーとなる模様。
	// ムーブ先の文字列バッファ容量が不十分な場合は、ポインタのすげ替えになる模様。
	// VC2015 Update3 でも、gcc 6.3 でも同様の実装になっているらしい。
	// いずれにせよ、ヒープの再割り当ては発生しない模様。
	// なお、C++11 で追加された codecvt は、C++17 で非推奨となってしまった。
	// せっかく char16_t と std::u16string が標準化されたのに……
	// C 関数としては mbstowcs(), wcstombs() などが標準化されているが、
	// これらで UTF-8/UTF-16 間の変換をするにはスレッドのロケール（コードページ）の明示的な事前設定が必要だったりと、使い勝手が悪い。
	// MSVC にはロケールを引数指定できるバージョン _mbstowcs_l(), _wcstombs_l() もあるが、ISO 標準ではない。
	// そもそも C/C++ においてワイド文字が UTF-16 であるとは限らない（規格で規定されていない）という根本的な問題もある。
	// https://en.cppreference.com/w/cpp/string/multibyte/mbstowcs
	// https://en.cppreference.com/w/cpp/string/multibyte/wcstombs
	// クロスプラットフォームな変換処理のためには、ICU (International Components for Unicode) ライブラリの C/C++ 版 ICU4C を使うべき。
	// C++11 の char16_t にも対応している。
	// http://site.icu-project.org/
	// C11/C++11 以降では mbrtoc16(), c16rtomb() を使うのが良いかも。

	std::vector<char> ConvertUtf16toUtf8(const wchar_t* srcText)
	{
		_ASSERTE(srcText != nullptr);
		const int textLen = static_cast<int>(wcslen(srcText));
		if (textLen <= 0)
		{
			return{ 0 };
		}
		const int reqSize = ::WideCharToMultiByte(CP_UTF8, 0, srcText, textLen, nullptr, 0, nullptr, nullptr);
		if (reqSize > 0)
		{
			// 最後の + 1 は必須。終端 NUL を含まないサイズが返る。
			std::vector<char> buff(reqSize + 1);
			// 第4引数に -1 を指定すると、終端 NUL を含むサイズが返るが、終端 NUL をもとにした長さ取得を複数回実行させるのは冗長。
			//std::vector<char> buff(reqSize);
			::WideCharToMultiByte(CP_UTF8, 0, srcText, textLen, &buff[0], reqSize, nullptr, nullptr);
			return buff;
		}
		else
		{
			// 空文字列すなわち第4引数にゼロを指定した場合もゼロが返ってくるが、その可能性は事前に排除している。
			// それ以外のケースでゼロが返ってくるのは完全にエラーなので、本来は例外を投げるべき？
			//return std::vector<char>();
			return{ 0 };
		}
	}

	std::vector<wchar_t> ConvertUtf8toUtf16(const char* srcText)
	{
		_ASSERTE(srcText != nullptr);
		const int textLen = static_cast<int>(strlen(srcText));
		if (textLen <= 0)
		{
			return{ 0 };
		}
		const int reqSize = ::MultiByteToWideChar(CP_UTF8, 0, srcText, textLen, nullptr, 0);
		if (reqSize > 0)
		{
			// 最後の + 1 は必須。終端 NUL を含まないサイズが返る。
			std::vector<wchar_t> buff(reqSize + 1);
			// 第4引数に -1 を指定すると、終端 NUL を含むサイズが返るが、終端 NUL をもとにした長さ取得を複数回実行させるのは冗長。
			//std::vector<wchar_t> buff(reqSize);
			::MultiByteToWideChar(CP_UTF8, 0, srcText, textLen, &buff[0], reqSize);
			return buff;
		}
		else
		{
			// 空文字列すなわち第4引数にゼロを指定した場合もゼロが返ってくるが、その可能性は事前に排除している。
			// それ以外のケースでゼロが返ってくるのは完全にエラーなので、本来は例外を投げるべき？
			//return std::vector<wchar_t>();
			return{ 0 };
		}
	}

	bool GetUtf8FullPath(const wchar_t* inRelFilePath, std::string& outFullPath)
	{
		const int RequiredStringBufferSize = 1024;
		wchar_t inFullPath[RequiredStringBufferSize] = {};
		char outFullPathBuf[RequiredStringBufferSize] = {};

		if (_wfullpath(inFullPath, inRelFilePath, RequiredStringBufferSize) == nullptr)
		{
			return false;
		}

		if (::WideCharToMultiByte(CP_UTF8, 0, inFullPath, -1, outFullPathBuf, RequiredStringBufferSize, nullptr, nullptr) == 0)
		{
			return false;
		}

		outFullPath = outFullPathBuf;
		return true;
	}

} // end of namespace
