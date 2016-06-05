#include "stdafx.h"
#include "MyGLSLHelper.h"
#include "MyGLRAII.hpp"
#include "MyUtil.h"

#include "DebugNew.h"


namespace
{
	// GLSL シェーダー プログラム ファイルを読み込み、コンパイル可能な文字列バッファを作成する。null 終端ではないので注意。
	// OpenCL プログラムにも転用可能。
	bool LoadShaderSourceFile(LPCWSTR pFileName, _Out_ std::vector<char>& outStrBuffer)
	//bool LoadShaderSourceFile(LPCWSTR pFileName, _Out_ std::vector<char>& outStrBuffer, _Out_ int& outStrLength)
	{
		std::vector<char> buf;
		if (!MyUtil::LoadBinaryFromFile(pFileName, buf))
		{
			return false;
		}
		if (buf.size() >= 3 && buf[0] == char(0xEF) && buf[1] == char(0xBB) && buf[2] == char(0xBF))
		{
			// BOM 付き UTF-8 なので、BOM を取り除く。
			// なお、GLSL ソースにマルチバイトのコメントを入れる場合は、ANSI エンコーディングではなく UTF-8 を使ったほうがよい。
			buf.erase(buf.begin(), buf.begin() + 3);
		}
		else if (buf.size() >= 2 && ((buf[0] == char(0xFF) && buf[1] == char(0xFE)) || (buf[0] == char(0xFE) && buf[1] == char(0xFF))))
		{
			// UTF-16LE/BE には非対応。
			ATLTRACE(L"UTF-16 is not supported!!\n");
			return false;
		}
		//outStrLength = static_cast<int>(buf.size());
		//buf.push_back('\0'); // null terminated string.
		outStrBuffer.swap(buf);
		return true;
	}

	bool ReadAndCompileShaderProgramFile(GLuint shaderObj, LPCWSTR pFileName)
	{
		_ASSERTE(shaderObj != 0);
		ATLTRACE(L"Now compiling the shader program file: \"%s\"\n", pFileName);

		std::vector<char> strBuffer;
		if (!LoadShaderSourceFile(pFileName, strBuffer))
		{
			return false;
		}
		const int fileSize = static_cast<int>(strBuffer.size());
		const char* pString = &strBuffer[0];
		// シェーダー オブジェクトにプログラム ソースをセット。
		return MyOGL::CompileShaderSourceString(shaderObj, 1, &pString, &fileSize);
	}

	bool ConnectMultiShaderSourceFiles(std::vector<char>& outTotalBuffer, const LPCWSTR ppSrcFileNamesArray[], int srcFileNum)
	{
		_ASSERTE(ppSrcFileNamesArray != nullptr && srcFileNum > 0);
		// 複数のファイルの内容を順番に連結していく。ただし #include の実装ではない。
		std::vector<char> totalBuffer;
		for (int i = 0; i < srcFileNum; ++i)
		{
			std::vector<char> strTempBuffer;
			ATLTRACE(L"Now loading the shader program file: \"%s\"\n", ppSrcFileNamesArray[i]);
			if (!LoadShaderSourceFile(ppSrcFileNamesArray[i], strTempBuffer))
			{
				return false;
			}
			// CArray::Append(const CArray&) に相当する処理は std::vector::insert() になる。
			// 出力先のイテレータと入力元の範囲イテレータを指定する。
			// 追加ではなく新規割り当てする場合はオーバーロードされたコンストラクタを使う方法もある。
			totalBuffer.insert(totalBuffer.end(), strTempBuffer.begin(), strTempBuffer.end());
			std::stringstream lineNumber;
			// GLSL のコンパイル エラー メッセージを分かりやすくするため、ファイルごとに行番号をリセットし、ファイル番号を設定していく。
			// #line は記述された次の行番号を設定するディレクティブらしい。
			lineNumber << "#line 1 " << (i + 1) << "\n";
			const auto strLineNumber = lineNumber.str();
			// null 終端は入れない。
			totalBuffer.insert(totalBuffer.end(), strLineNumber.begin(), strLineNumber.begin() + strLineNumber.length());
		}
		outTotalBuffer.swap(totalBuffer);
		return true;
	}

	void GetShaderInfoLog(GLuint shaderObj, std::vector<char>& outBuf)
	{
		GLint bufSize = 0;
		glGetShaderiv(shaderObj, GL_INFO_LOG_LENGTH, &bufSize);
		// コンパイルに成功した場合も、サイズ 1 のゼロ終端空文字列が返ってくる。
		if (bufSize > 1)
		{
			std::vector<char> buf(bufSize);
			GLsizei len = 0;
			glGetShaderInfoLog(shaderObj, bufSize, &len, &buf[0]);
			outBuf.swap(buf);
		}
		else
		{
			outBuf.clear();
		}
	}

	void GetProgramInfoLog(GLuint programObj, std::vector<char>& outBuf)
	{
		GLint bufSize = 0;
		glGetProgramiv(programObj, GL_INFO_LOG_LENGTH, &bufSize);
		// リンクに成功した場合も、サイズ 1 のゼロ終端空文字列が返ってくる。
		if (bufSize > 1)
		{
			std::vector<char> buf(bufSize);
			GLsizei len = 0;
			glGetProgramInfoLog(programObj, bufSize, &len, &buf[0]);
			outBuf.swap(buf);
		}
		else
		{
			outBuf.clear();
		}
	}


	// Program Pipeline で使用するための独立したシングル シェーダー プログラムを作成する。
	GLuint CreateSoloShaderProgramFromFile(GLenum shaderType, const LPCWSTR ppSrcFileNamesArray[], int srcFileNum)
	{
		_ASSERTE(
			shaderType == GL_VERTEX_SHADER ||
			shaderType == GL_TESS_CONTROL_SHADER ||
			shaderType == GL_TESS_EVALUATION_SHADER ||
			shaderType == GL_GEOMETRY_SHADER ||
			shaderType == GL_FRAGMENT_SHADER ||
			shaderType == GL_COMPUTE_SHADER);
		std::vector<char> strTotalBuffer;
		ConnectMultiShaderSourceFiles(strTotalBuffer, ppSrcFileNamesArray, srcFileNum);
		const GLsizei fileSize = static_cast<GLsizei>(strTotalBuffer.size());
		const char* pString = &strTotalBuffer[0];
		GLuint outProgramObj = glCreateShaderProgramv(shaderType, fileSize, &pString);
		if (outProgramObj != 0)
		{
			// TODO: コンパイル エラーおよびリンク エラーがないかどうか調べる。
			GLint isCompiled = 0;
			glGetProgramiv(outProgramObj, GL_COMPILE_STATUS, &isCompiled);
			GLint isLinked = 0;
			glGetProgramiv(outProgramObj, GL_LINK_STATUS, &isLinked);

			std::vector<char> buf;
			std::string strMsg;
			GetProgramInfoLog(outProgramObj, buf);
			if (buf.size() > 1)
			{
				strMsg += "Message from GLSL Compiler/Linker:\n";
				strMsg += &buf[0];
				strMsg += '\n';
				ATLTRACE(strMsg.c_str());
				// 出力メッセージがあまりに長いと、ATLTRACE() は何も出力してくれないので注意。おそらく 1024 文字が限度。
				// MFC 環境ではとりあえずメッセージボックスを使う。
				// HACK: 例外をスローして呼び出し側に詳細情報を渡したほうがいいかも。
#ifdef _MFC_VER
				if (strMsg.length() + 1 >= 1024)
				{
					AfxMessageBox(CString(strMsg.c_str()));
				}
#else
				ATLTRACE("Message from GLSL compiler/linker is too long!!\n");
#endif
			}
		}
		return outProgramObj;
	}

	bool ReadAndCompileShaderProgramFile(GLuint shaderObj, const LPCWSTR ppSrcFileNamesArray[], int srcFileNum)
	{
		_ASSERTE(shaderObj != 0);

#if 1
		std::vector<char> strTotalBuffer;
		ConnectMultiShaderSourceFiles(strTotalBuffer, ppSrcFileNamesArray, srcFileNum);
		const GLsizei fileSize = static_cast<GLsizei>(strTotalBuffer.size());
		const char* pString = &strTotalBuffer[0];
		// シェーダー オブジェクトにプログラム ソースをセット。
		return MyOGL::CompileShaderSourceString(shaderObj, 1, &pString, &fileSize);
#else
		// 連結して単一の文字列にすると、2つ目以降のファイルでコンパイル エラーが発生したときに該当行番号の特定が大変になる。
		// ……と思ったが、マルチソースでもソース番号・行番号は GLSL #line ディレクティブでプログラマーが明示的に制御する必要があるらしい。
		// glShaderSource() の strings にマルチソースを渡したとしても、
		// 自動的にソース番号が割り振られることはないし、行番号がリセットされることもない。
		// #line を使えば、ホスト側でシングルソース（シングルバッファ）にまとめてしまってもよさげ。
		std::vector<std::vector<char>> srcStringsArray(srcFileNum);
		std::vector<const char*> srcPointersArray(srcFileNum);
		std::vector<int> srcLengthsArray(srcFileNum);
		for (int i = 0; i < srcFileNum; ++i)
		{
			std::vector<char> strTempBuffer;
			ATLTRACE(L"Now loading the shader program file: \"%s\"\n", ppSrcFileNamesArray[i]);
			if (!LoadShaderSourceFile(ppSrcFileNamesArray[i], strTempBuffer))
			{
				return false;
			}
			auto& targetElem = srcStringsArray[i];
			targetElem.swap(strTempBuffer);
			srcPointersArray[i] = &targetElem[0];
			srcLengthsArray[i] = static_cast<int>(targetElem.size());
		}
		// シェーダー オブジェクトにプログラム ソースをセット。
		return MyOGL::CompileShaderSourceString(shaderObj, srcFileNum, &srcPointersArray[0], &srcLengthsArray[0]);
#endif
	}

	// シェーダー プログラムをリンクする。
	bool LinkAttachedShadersToShaderProgram(GLuint programObj)
	{
		glLinkProgram(programObj);

		GLint isLinked = 0;
		glGetProgramiv(programObj, GL_LINK_STATUS, &isLinked);

		{
			std::vector<char> buf;
			std::string strMsg;
			GetProgramInfoLog(programObj, buf);
			if (buf.size() > 1)
			{
				strMsg += "Message from GLSL Linker:\n";
				strMsg += &buf[0];
				strMsg += '\n';
				ATLTRACE(strMsg.c_str());
#ifdef _MFC_VER
				if (strMsg.length() + 1 >= 1024)
				{
					AfxMessageBox(CString(strMsg.c_str()));
				}
#else
				ATLTRACE("Message from GLSL linker is too long!!\n");
#endif
			}
			if (isLinked == 0)
			{
				ATLTRACE("Failed to link shader programs!!\n");
				return false;
			}
		}
		ATLTRACE("Succeeded to link shader programs.\n");
		return true;
	}

} // end of namespace


namespace MyOGL
{
	// shaderSrcArray の各要素は null 終端文字列である必要はない。
	// その代わり各要素の長さを srcLengthArray で指定する必要がある。
	bool CompileShaderSourceString(GLuint shaderObj, int srcCount, const char* shaderSrcArray[], const int srcLengthArray[])
	{
		_ASSERTE(shaderObj != 0);
		_ASSERTE(shaderSrcArray != nullptr);
		_ASSERTE(srcLengthArray != nullptr);
		_ASSERTE(srcCount > 0);
		{
			// シェーダー オブジェクトにコンパイルすべきプログラム ソースをセット。
			// strings パラメータが null 終端文字列のポインタ配列であれば、
			// lengths パラメータには NULL を指定できるらしい。
			glShaderSource(shaderObj, srcCount, shaderSrcArray, srcLengthArray);
		}

		// シェーダーのコンパイル。
		glCompileShader(shaderObj);
		GLint isCompiled = 0;
		glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &isCompiled);

		{
			std::vector<char> buf;
			std::string strMsg;
			GetShaderInfoLog(shaderObj, buf);
			if (buf.size() > 1)
			{
				strMsg += "Message from GLSL Compiler:\n";
				strMsg += &buf[0];
				strMsg += '\n';
				ATLTRACE(strMsg.c_str());
#ifdef _MFC_VER
				if (strMsg.length() + 1 >= 1024)
				{
					AfxMessageBox(CString(strMsg.c_str()));
				}
#else
				ATLTRACE("Message from GLSL compiler is too long!!\n");
#endif
			}
			if (isCompiled == 0)
			{
				ATLTRACE("Failed to compile the shader!!\n");
				//AfxMessageBox(_T("Failed to compile the shader!!"));
				return false;
			}
		}
		ATLTRACE("Succeeded to compile the shader.\n");
		return true;
	}

	bool LinkShaderProgramObjVSFS(GLuint programObj,
			GLuint vertexShaderObj,
			GLuint fragmentShaderObj)
	{
		_ASSERTE(programObj != 0);
		_ASSERTE(vertexShaderObj != 0);
		_ASSERTE(fragmentShaderObj != 0);

		// 頂点シェーダー オブジェクトをシェーダー プログラムに関連付ける。
		glAttachShader(programObj, vertexShaderObj);

		// フラグメント シェーダー オブジェクトをシェーダー プログラムに関連付ける。
		glAttachShader(programObj, fragmentShaderObj);

		// シェーダー プログラムのリンク。
		if (!LinkAttachedShadersToShaderProgram(programObj))
		{
			return false;
		}
		return true;
	}

	bool CreateShaderProgramFromSrcVSFS(GLuint programObj,
			const char* vertexShaderSrc,
			const char* fragmentShaderSrc)
	{
		_ASSERTE(programObj != 0);

		// 頂点シェーダー オブジェクトへのハンドルを作成・取得する。
		// 再利用しない一時シェーダー オブジェクトなので、プログラムのコンパイル＆リンク後は削除してかまわない。
		auto vertexShader = Factory::CreateVertexShaderPtr();

		// 頂点シェーダーのコンパイル。
		if (!CompileShaderSourceString(vertexShader.get(), vertexShaderSrc))
		{
			return false;
		}

		// フラグメント シェーダー オブジェクトへのハンドルを作成・取得する。
		// 再利用しない一時シェーダー オブジェクトなので、プログラムのコンパイル＆リンク後は削除してかまわない。
		auto fragmentShader = Factory::CreateFragmentShaderPtr();

		// フラグメント シェーダーのコンパイル。
		if (!CompileShaderSourceString(fragmentShader.get(), fragmentShaderSrc))
		{
			return false;
		}

		return LinkShaderProgramObjVSFS(programObj, vertexShader.get(), fragmentShader.get());
	}

} // end of namespace


namespace MyOGL
{
	bool CreateMyShaderProgramVSFSFromFile(GLuint programObj,
		const LPCWSTR ppVertexFileNamesArray[], int vertexFileNum,
		const LPCWSTR ppFragmentFileNamesArray[], int fragmentFileNum)
	{
		_ASSERTE(programObj != 0);
		_ASSERTE(ppVertexFileNamesArray != nullptr && vertexFileNum > 0);

		// 頂点シェーダー オブジェクトへのハンドルを作成・取得する。
		// 一時シェーダー オブジェクトなので、プログラムのコンパイル＆リンク後は削除してかまわない。
		ShaderResourcePtr vertexShader(Factory::CreateVertexShaderPtr());

		// 頂点シェーダーの読み込みとコンパイル。
		if (!ReadAndCompileShaderProgramFile(vertexShader.get(), ppVertexFileNamesArray, vertexFileNum))
		{
			return false;
		}

		// 頂点シェーダー オブジェクトをシェーダー プログラムに関連付ける。
		glAttachShader(programObj, vertexShader.get());

		ShaderResourcePtr fragmentShader; // リンクするまでは死んではいけない。

		if (ppFragmentFileNamesArray && fragmentFileNum > 0)
		{
			// フラグメント シェーダー オブジェクトへのハンドルを作成・取得する。
			fragmentShader = Factory::CreateFragmentShaderPtr();

			// フラグメント シェーダーの読み込みとコンパイル。
			if (!ReadAndCompileShaderProgramFile(fragmentShader.get(), ppFragmentFileNamesArray, fragmentFileNum))
			{
				return false;
			}

			// フラグメント シェーダー オブジェクトをシェーダー プログラムに関連付ける。
			glAttachShader(programObj, fragmentShader.get());

			//glBindFragDataLocation(programObj, 0, "FsOutColor");
			// --> glBindFragDataLocation() は GLSL 1.x で MRT を使うときの API らしい。
			// GLSL 1.x には、Direct3D 9 HLSL の COLOR[n] セマンティクスや、
			// Direct3D 10/11 HLSL の SV_Target[n] セマンティクスに相当するものがないらしい。
			// GLSL 3.3 以降では、フラグメント シェーダーの out パラメータを layout で修飾すると、この API による設定は上書きされるらしい。
			// つまり GLSL 側での location 値設定が優先される。
		}

		// シェーダー プログラムのリンク。
		if (!LinkAttachedShadersToShaderProgram(programObj))
		{
			return false;
		}
		return true;
	}

	bool CreateMyShaderProgramCSFromFile(GLuint programObj,
		const LPCWSTR ppComputeFileNamesArray[], int computeFileNum)
	{
		_ASSERTE(programObj != 0);
		_ASSERTE(ppComputeFileNamesArray != nullptr && computeFileNum > 0);

		ShaderResourcePtr computeShader(Factory::CreateComputeShaderPtr());

		if (!ReadAndCompileShaderProgramFile(computeShader.get(), ppComputeFileNamesArray, computeFileNum))
		{
			return false;
		}

		glAttachShader(programObj, computeShader.get());

		if (!LinkAttachedShadersToShaderProgram(programObj))
		{
			return false;
		}
		return true;
	}

} // end of namespace
