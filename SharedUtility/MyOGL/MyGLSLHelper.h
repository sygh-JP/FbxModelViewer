#pragma once


namespace MyOGL
{
	// GLSL シェーダー プログラムのソース文字列を使ってコンパイルする。
	// なお、GLSL はどのみち #include をサポートしないので、ファイル ベースではなく
	// C/C++ コードなどへのソース文字列リテラルの埋め込みにしたほうが逆に管理・再利用の面で楽になるかもしれない。
	// 外部ファイル化しておくとコンパイル エラー発生箇所の特定が簡単になるし、
	// GLSL ソースを修正したときに実行プログラムへのリンクの手間が省ける。
	extern bool CompileShaderSourceString(GLuint shaderObj, int srcCount, const char* shaderSrcArray[], const int srcLengthArray[]);

	// 単一のソース文字列のみを使う簡易バージョン。
	inline bool CompileShaderSourceString(GLuint shaderObj, const char* shaderSrc)
	{
		const int srcStrLength = static_cast<int>(strlen(shaderSrc));
		return CompileShaderSourceString(shaderObj, 1, &shaderSrc, &srcStrLength);
	}

	//! @brief  GLSL シェーダー プログラムの作成。<br>
	//!
	//! 頂点シェーダー（VS）とフラグメント シェーダー（FS）のみの各ソース文字列を指定するバージョン。<br>
	//! OpenGL 2.x, OpenGL ES 2.0/3.0 では VS + FS しか使えないし、<br>
	//! GS, TCS, TES, および CS が使える上位規格でも大抵は VS + FS の組み合わせがほとんどだが、<br>
	//! あえて意図的に区別する命名にしている。<br>
	extern bool CreateShaderProgramFromSrcVSFS(GLuint programObj,
			const char* vertexShaderSrc,
			const char* fragmentShaderSrc);

	//! @brief  コンパイル済みのシェーダー オブジェクトを指定するバージョン。<br>
	extern bool LinkShaderProgramObjVSFS(GLuint programObj,
			GLuint vertexShaderObj,
			GLuint fragmentShaderObj);

	extern bool CreateMyShaderProgramVSFSFromFile(GLuint programObj,
		const LPCWSTR ppVertexFileNamesArray[], int vertexFileNum,
		const LPCWSTR ppFragmentFileNamesArray[] = nullptr, int fragmentFileNum = 0);

	extern bool CreateMyShaderProgramCSFromFile(GLuint programObj,
		const LPCWSTR ppComputeFileNamesArray[], int computeFileNum);

	//! @brief  GLSL シェーダー プログラムの作成。<br>
	//! 頂点シェーダーとフラグメント シェーダーを指定する。<br>
	//! 
	//! @param[in]  pVertexFileName  頂点シェーダーをカスタマイズする GLSL ソース ファイルへのパス。<br>
	//! @param[in]  pFragmentFileName  フラグメント シェーダーをカスタマイズする GLSL ソース ファイルへのパス。<br>
	//! pFragmentFileName が nullptr の場合、フラグメント シェーダーは固定機能を使う。<br>
	//! ただし頂点シェーダーだけカスタマイズして、フラグメント シェーダーに固定機能を使えるのは、おそらく PC 用の OpenGL 2.x と GLSL 1.2 まで。<br>
	//! GLSL も 1.3 以降は固定機能と連携可能な組み込みの変数の大半が使えなくなる。<br>
	//! このあたりは Direct3D 9 と同じ事情と思われる。<br>
	inline bool CreateMyShaderProgramVSFSFromFile(GLuint programObj, LPCWSTR pVertexFileName, LPCWSTR pFragmentFileName = nullptr)
	{
		if (pFragmentFileName)
		{
			return CreateMyShaderProgramVSFSFromFile(programObj, &pVertexFileName, 1, &pFragmentFileName, 1);
		}
		else
		{
			return CreateMyShaderProgramVSFSFromFile(programObj, &pVertexFileName, 1);
		}
	}

	inline bool CreateMyShaderProgramCSFromFile(GLuint programObj, LPCWSTR pComputeFileName)
	{
		return CreateMyShaderProgramCSFromFile(programObj, &pComputeFileName, 1);
	}

} // end of namespace
