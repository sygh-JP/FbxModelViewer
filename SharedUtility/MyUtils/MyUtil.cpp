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

	// コピーコンストラクタよりも C++11 ムーブ コンストラクタが優先されることを考慮し、あえて引数ではなく戻り値で返す。
	std::string ConvertUtf16toUtf8(const wchar_t* srcText)
	{
		_ASSERTE(srcText != nullptr);
		const int textLen = static_cast<int>(wcslen(srcText));
		if (textLen <= 0)
		{
			return std::string();
		}
		const int reqSize = ::WideCharToMultiByte(CP_UTF8, 0, srcText, textLen, nullptr, 0, nullptr, nullptr);
		if (reqSize > 0)
		{
			std::vector<char> buff(reqSize + 1); // 最後の + 1 は必須らしい。終端 null を含まないサイズが返るらしい。
			//std::vector<char> buff(reqSize);
			::WideCharToMultiByte(CP_UTF8, 0, srcText, textLen, &buff[0], reqSize, nullptr, nullptr);
			return std::move(std::string(&buff[0]));
		}
		else
		{
			// 完全にエラーなので本来は例外を投げるべき？
			return std::string();
		}
	}

	std::wstring ConvertUtf8toUtf16(const char* srcText)
	{
		_ASSERTE(srcText != nullptr);
		const int textLen = static_cast<int>(strlen(srcText));
		if (textLen <= 0)
		{
			return std::wstring();
		}
		const int reqSize = ::MultiByteToWideChar(CP_UTF8, 0, srcText, textLen, nullptr, 0);
		if (reqSize > 0)
		{
			std::vector<wchar_t> buff(reqSize + 1); // 最後の + 1 は必須らしい。終端 null を含まないサイズが返るらしい。
			//std::vector<wchar_t> buff(reqSize);
			::MultiByteToWideChar(CP_UTF8, 0, srcText, textLen, &buff[0], reqSize);
			return std::move(std::wstring(&buff[0]));
		}
		else
		{
			// 完全にエラーなので本来は例外を投げるべき？
			return std::wstring();
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
