#include "stdafx.h"
#include "MyGLSLHelper.h"
#include "MyOGL.h"
#include "MyStopwatch.hpp"
#include "CustomTrace.hpp"
#include "MyAppCommonConsts.hpp"
using namespace MyAppCommonConsts;

#include "DebugNew.h"

// TODO: ハードウェア能力に応じて、MSAA の有無を動的に切り替える。
// Intel 系では OpenGL も Direct3D も MSAA がサポートされていないことが多い。
// TODO: OpenGL 4.4 をサポートしない場合はリソースの作成を中止し、画面の塗りつぶしだけを行なうようにする。
// 4.3 以前ではバッファ レイアウト系の細かい制御ができないので、Direct3D 11 定数バッファ用 POD 型を共有できない。
#define ENABLES_MY_GL_MSAA

namespace MyOGL
{
	const GLenum CubemapAllFaceTypes[] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	};

	// 圧縮テクスチャを作成する。
	// http://dench.flatlib.jp/opengl/texturefileformat
	// http://msdn.microsoft.com/ja-jp/library/ee417780.aspx
	bool CreateCompressedTexture(GLenum targetDimension, GLenum texFormat, GLsizei imageWidth, GLsizei imageHeight, GLsizei imageDepth, GLint numMipmaps, const uint8_t* pCompressedImageData, size_t compressedImageDataSizeInBytes, TextureResourcePtr& outTexture)
	{
		// DXT2, DXT4 は拡張シンボルがなく、OpenGL では非対応らしい。

		int blockSize = 0;
		switch (texFormat)
		{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: // BC1
			blockSize = 8;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: // BC2
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: // BC3
			blockSize = 16;
			break;
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: // BC6H
		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: // BC6H
		case GL_COMPRESSED_RGBA_BPTC_UNORM: // BC7
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM: // BC7
		case GL_COMPRESSED_RGB8_ETC2:
		case GL_COMPRESSED_RGBA8_ETC2_EAC:
		default:
			// HACK: とりあえず非対応。
			// 非圧縮の DDS にも非対応。
			// 圧縮・非圧縮はフォーマットを見て分岐するようにして、外部に公開するメソッドは両方受け取れるようにしておいたほうがよい。
			return false;
		}

		_ASSERTE(
			targetDimension == GL_TEXTURE_1D || targetDimension == GL_TEXTURE_2D || targetDimension == GL_TEXTURE_3D ||
			targetDimension == GL_TEXTURE_CUBE_MAP ||
			targetDimension == GL_TEXTURE_1D_ARRAY || targetDimension == GL_TEXTURE_2D_ARRAY);
		if (targetDimension != GL_TEXTURE_1D && targetDimension != GL_TEXTURE_2D && targetDimension == GL_TEXTURE_3D &&
			targetDimension != GL_TEXTURE_CUBE_MAP)
		{
			// HACK: とりあえず非対応。
			return false;
		}
		if (imageWidth < 1 || imageHeight < 1 || imageDepth < 1)
		{
			_ASSERTE(false);
			return false;
		}
		// HACK: テクスチャ サイズ上限のチェック。ハードウェア能力にも依存する。Direct3D 9/10/11 と同じ仕様とは限らない。

		outTexture = Factory::CreateOneTexturePtr();
		_ASSERTE(outTexture.get() != 0);
		if (outTexture.get() == 0)
		{
			return false;
		}

		// キューブマップ配列にはとりあえず非対応。
		const int arraySize = (targetDimension == GL_TEXTURE_CUBE_MAP) ? 6 : 1;

		int offset = 0;

		glBindTexture(targetDimension, outTexture.get());
		glTexParameteri(targetDimension, GL_TEXTURE_MAX_LEVEL, numMipmaps - 1);
		// DDS フォーマットは、
		// ( ( ( ( ( width * height ) * volume_depth ) * mip ) * cube_face ) * array )
		// の順で格納されているのでキューブマップの取り出しが簡単。
		// array は D3D10 で拡張されたテクスチャ配列に相当するが、対応しているツールが少ないらしい。
		// DirectX は 10.1 以降でキューブマップ配列に対応。OpenGL は 4.0 以降で標準化されたらしい。
		// HACK: KTX はミップマップの扱いが逆になるので注意。
		for (int a = 0; a < arraySize; ++a)
		{
			GLsizei mipWidth = imageWidth, mipHeight = imageHeight, mipDepth = imageDepth;

			for (int m = 0; m < numMipmaps; ++m)
			{
				// HACK: CubeMap のみ検証。1D, 2D, 3D は正しい処理なのかどうか検証・テストが必要。

				// DDS 専用の分解処理。
				const GLsizei mipSizeInBytes = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * ((mipDepth + 3) / 4) * blockSize;
				_ASSERTE(size_t(offset + mipSizeInBytes) <= compressedImageDataSizeInBytes); // でないとオーバーランする。
				if (targetDimension == GL_TEXTURE_CUBE_MAP)
				{
					// キューブマップの各面は常に 2D となる。
					glCompressedTexImage2D(CubemapAllFaceTypes[a], m, texFormat, mipWidth, mipHeight, 0, mipSizeInBytes, pCompressedImageData + offset);
				}
				else if (targetDimension == GL_TEXTURE_1D)
				{
					glCompressedTexImage1D(GL_TEXTURE_1D, m, texFormat, mipWidth, 0, mipSizeInBytes, pCompressedImageData + offset);
				}
				else if (targetDimension == GL_TEXTURE_2D)
				{
					glCompressedTexImage2D(GL_TEXTURE_2D, m, texFormat, mipWidth, mipHeight, 0, mipSizeInBytes, pCompressedImageData + offset);
				}
				else if (targetDimension == GL_TEXTURE_3D)
				{
					glCompressedTexImage3D(GL_TEXTURE_3D, m, texFormat, mipWidth, mipHeight, mipDepth, 0, mipSizeInBytes, pCompressedImageData + offset);
				}

				mipWidth = (std::max)((mipWidth / 2), 1);
				mipHeight = (std::max)((mipHeight / 2), 1);
				mipDepth = (std::max)((mipDepth / 2), 1);

				offset += mipSizeInBytes;
			}
		}
		glBindTexture(targetDimension, 0);
		return true;
	}

	static bool CreateDummyCubeTexture(TextureResourcePtr& outTexture)
	{
		outTexture = Factory::CreateOneTexturePtr();
		_ASSERTE(outTexture.get() != 0);
		if (outTexture.get() == 0)
		{
			return false;
		}

		const GLsizei imageWidth = MyTextureHelper::DUMMY_WHITE_TEX_SIZE;
		const GLsizei imageHeight = MyTextureHelper::DUMMY_WHITE_TEX_SIZE;
		const GLenum texFormat = GL_RGBA;
		const GLenum targetDimension = GL_TEXTURE_CUBE_MAP;
		const int arraySize = 6;

		glBindTexture(targetDimension, outTexture.get());
		glTexParameteri(targetDimension, GL_TEXTURE_MAX_LEVEL, 0);
		// デフォルトで白色。
		const std::vector<uint32_t> dummyBuffer(imageWidth * imageHeight, 0xFFFFFFFF);
		for (int a = 0; a < arraySize; ++a)
		{
			glTexImage2D(CubemapAllFaceTypes[a], 0, texFormat, imageWidth, imageHeight, 0, texFormat, GL_UNSIGNED_BYTE,
				&dummyBuffer[0]);
		}

		glBindTexture(targetDimension, 0);
		return true;
	}


	// 最後の引数 userParam は、GLEW 1.10 までは GLvoid* だったが、正しくは const void* でなければならない。GLEW 1.11 では修正されている。
	// GLvoid* を受け取るのは AMD 固有の拡張版のみらしい。
	static void CALLBACK OutputGLDebugMessage(
		GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		// HACK: 各定数は連続が保証されることが規格で決まっているのかどうかよく分からないので、ベタに switch-case のほうがいいかも。

		static const char* const ppSourceStrings[] =
		{
			"OpenGL API",
			"Window System",
			"Shader Compiler",
			"Third Party",
			"Application",
			"Other",
		};
		const int sourceNo = (GL_DEBUG_SOURCE_API_ARB <= source && source <= GL_DEBUG_SOURCE_OTHER_ARB)
			? (source - GL_DEBUG_SOURCE_API_ARB)
			: (GL_DEBUG_SOURCE_OTHER_ARB - GL_DEBUG_SOURCE_API_ARB);

		static const char* const ppTypeStrings[] =
		{
			"Error",
			"Deprecated behavior",
			"Undefined behavior",
			"Portability",
			"Performance",
			"Other",
		};
		const int typeNo = (GL_DEBUG_TYPE_ERROR_ARB <= type && type <= GL_DEBUG_TYPE_OTHER_ARB)
			? (type - GL_DEBUG_TYPE_ERROR_ARB)
			: (GL_DEBUG_TYPE_OTHER_ARB - GL_DEBUG_TYPE_ERROR_ARB);

		static const char* const ppSeverityStrings[] =
		{
			"High",
			"Medium",
			"Low",
		};
		const int severityNo = (GL_DEBUG_SEVERITY_HIGH_ARB <= type && type <= GL_DEBUG_SEVERITY_LOW_ARB)
			? (type - GL_DEBUG_SEVERITY_HIGH_ARB)
			: (GL_DEBUG_SEVERITY_LOW_ARB - GL_DEBUG_SEVERITY_HIGH_ARB);

		ATLTRACE("Source=\"%s\", Type=\"%s\", ID=%d, Severity=%s, Message=\"%s\"\n",
			ppSourceStrings[sourceNo], ppTypeStrings[typeNo], id, ppSeverityStrings[severityNo], message);
	}

	// OpenGL Shader Storage Buffer Object を作成する。
	// DirectX の RWStructuredBuffer に相当する。
	// ちなみに追加バッファ／消費バッファに直接相当するものはなさげ。
	// アトミック カウンターを使って自前でなんとかしろ、ということらしい。
	// ID3D11DeviceContext::CopyStructureCount() の説明を読む限り、
	// 追加バッファには要素数管理のための隠しカウンターが存在するらしい。
	// http://msdn.microsoft.com/ja-jp/library/ee419575.aspx
	static bool CreateStorageBuffer(uint32_t elementSize, uint32_t elementCount, _In_opt_ const void* pInitData, BufferResourcePtr& outStorageBuffer)
	{
		outStorageBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(outStorageBuffer.get() != 0);
		if (outStorageBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, outStorageBuffer.get());
		// glBufferData() や glBufferStorage() の data に NULL を与えてもエラーにはならないが、glTexImage2D() のように未初期化のゴミデータが入ったままになるらしい。
		// バッファのクリア用として glClearBufferData(), glClearBufferSubData() の他に glClearBufferiv() などがあるようだが、どれが何なのかよく分からない。
		// OpenGL API は命名のひどさ（開発者の自己満足を投影したような意味不明な用語や略称を含む）や EXT/ARB エクステンション方式の冗長さもあいまってカオスすぎる。
		// Direct3D のようにオブジェクト指向で階層構造化されているわけでもないので、各関数の動作を関連付けにくい。

		//glBufferData(GL_SHADER_STORAGE_BUFFER, elementSize * elementCount, pInitData, GL_DYNAMIC_DRAW);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, elementSize * elementCount, pInitData, 0);
		// CPU による読み書きは許可しない。
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		return true;
	}

	// OpenGL Uniform Buffer Object を作成する。
	// DirectX の Constant Buffer (cbuffer) に相当する。
	template<typename T> static bool CreateUniformBuffer(_In_opt_ const T* pInitData, BufferResourcePtr& outUniformBuffer)
	{
		outUniformBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(outUniformBuffer.get() != 0);
		if (outUniformBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, outUniformBuffer.get());
		//glBufferData(GL_UNIFORM_BUFFER, sizeof(T), pInitData, GL_DYNAMIC_DRAW);
		glBufferStorage(GL_UNIFORM_BUFFER, sizeof(T), pInitData, GL_DYNAMIC_STORAGE_BIT);
		// Uniform Block の内容の書き換えは、
		// [1] glBufferData() + GL_DYNAMIC_DRAW + glBufferSubData()
		// [2] glBufferData() + GL_DYNAMIC_DRAW + glMapBuffer() + GL_WRITE_ONLY + glUnmapBuffer()
		// [3] glBufferData() + GL_DYNAMIC_DRAW + glMapBufferRange() + (GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT) + glUnmapBuffer()
		// [4] glBufferStorage() + GL_DYNAMIC_STORAGE_BIT + glBufferSubData()
		// [5] glBufferStorage() + (GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT) + glMapBufferRange() + memcpy()
		// のどれで行なうのがベストなのか？
		// [1] は Direct3D 10/11 の UpdateSubresource() に近そう。OpenGL 2.x / ES 2.0 時代からある方法。
		// 頂点バッファのフレーム内書き換えに使ったことがあるが、
		// デスクトップの NVIDIA Quadro では問題なかったものの、モバイル環境（Android の OpenGL ES 2.0）で glFinish() が必要になることがあった。
		// [2] はたぶん D3D9 の Lock/Unlock、D3D10/11 の Map/Unmap 相当だが、OpenGL 1.5 時代からある機能らしい。ただしパフォーマンスに問題がありそう。
		// [3] は [2] 同様にパフォーマンスに問題がありそう。
		// [4] と [1] の違いは不明。
		// [5] は Immutable Buffer という方法らしく、OpenGL 4.4 で使えるようになった機能らしい。
		// http://shikihuiku.wordpress.com/2014/01/28/opengl%E3%81%AEimmutable-data-sotre%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6/
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return true;
	}

	template<typename T> static bool UpdateUniformBuffer(_In_opt_ const T* pSrcData, const BufferResourcePtr& uniformBuffer)
	{
		_ASSERTE(uniformBuffer.get() != 0);
		if (uniformBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer.get());
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), pSrcData);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return true;
	}


	static int GetMaxMSAASampleCount()
	{
		int maxSamples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
		return maxSamples;
	}

	static int GetMaxUniformBlockSize()
	{
		int maxUniformBlockSize = 0;
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
		return maxUniformBlockSize;
	}

	static int GetMaxSubroutineCount()
	{
		int maxSubroutineCount = 0;
		glGetIntegerv(GL_MAX_SUBROUTINES, &maxSubroutineCount);
		return maxSubroutineCount;
	}

	static BOOL SetSwapInterval(int interval)
	{
		//if (glewGetExtension("WGL_EXT_swap_control"))
		if (WGLEW_EXT_swap_control)
		{
			// DWM 環境では必ず垂直同期されるらしいので、意味がない？
			// wglSwapIntervalEXT(0) 呼び出しの有無で glClearColor() - SwapBuffers() - glFinish() の処理速度は変わるので、意味がないことはないはず。
			// interval の既定値は 1 らしい。
			// https://www.opengl.org/registry/specs/EXT/wgl_swap_control.txt

			return wglSwapIntervalEXT(interval);
		}
		else
		{
			ATLTRACE("OpenGL VSync control is not supported!!\n");
			return false;
		}
	}

	inline static void ActivateTextureSlot(uint32_t slot)
	{
		// GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, ... は連番であることが保証されているらしい。
		// 変な定義済みシンボルを毎回指定するよりは、glBindSampler() 同様にオフセット（インデックス）数値を指定できたほうが分かりやすい。
		// それはそうと glActiveTexture() の設計はどうみても失敗作以外の何者でもないと思われる。
		// OpenGL でマルチテクスチャを使おうとすると、まずこの API のダメ仕様にイラッとさせられる。
		// OpenGL API 全体に蔓延する絶望的なまでの分かりづらさは、
		// どうも OpenGL 黎明期にデバイス ドライバーの設計をそのまま API に落とし込んだという
		// まさにハードウェア屋の都合に基づいた設計になっていることが原因らしい。
		// http://yu-zora.blogspot.jp/2011/02/opengl2-opengl.html
		// Direct3D はメジャーバージョンアップのたびに互換性をバッサリ切り捨てて
		// 最新のトレンドを追及してきた C++ 向けオブジェクト指向ベースの Side-by-Side COM API なので、
		// 用語に慣れさえすれば非常に分かりやすくなっている仕様だが、
		// OpenGL のほうは（3.x で古い機能が一部廃止されたとはいえ）
		// いつまでたっても古い API との互換性が捨てられないので設計思想そのものが古いまま。まさに生きた化石。
		// この状態でどんどん新機能を追加していってもキメラになるだけだと思うが……
		// togl に関わっている Valve 社の開発者 Rich Geldreich もブチ切れているらしい。
		// 特にコンテキストや GLSL シェーダーまわりへの不満は非常に賛同できる。
		// 原文へのコメントも興味深い。
		// http://cpplover.blogspot.jp/2014/05/opengl.html
		// http://richg42.blogspot.jp/2014/05/things-that-drive-me-nuts-about-opengl.html
		// 元 id Software の伝説のプログラマー、かの John Carmack も OpenGL より Direct3D (Direct3D 11) が優れているという旨の発言をしている。
		// http://www.bit-tech.net/news/gaming/2011/03/11/carmack-directx-better-opengl/1
		// SIGGRAPH 2014 にて、ようやく API の作り直し (glNext) が発表された。
		// 共通バイトコード（中間言語）規格や、マルチスレッド レンダリングといった、
		// Direct3D が何年も昔からサポートしてきた機能をようやく盛り込むことになった。
		// http://jp.techcrunch.com/2014/08/12/20140811khronos-group-starts-working-on-the-next-generation-of-its-opengl-3d-specs/
		// 実際に Vulkan 1.0 として2016年2月に正式リリースされた時点では、
		// 肝心の SPIR-V に対応した公式オフラインシェーダーコンパイラー glslangValidator の完成度の低さにガッカリしたが、
		// その後着々と改良が重ねられ、GLSL だけでなく HLSL にも対応するようになっている。
		// OpenGL 4.6 にも SPIR-V のサポートがバックポートされた。
		// Vulkan 1.1 は Direct3D 12 が先行していたマルチ GPU 対応も正式にサポートするようになっている。
		// ただし Vulkan は Direct3D 12 以上にローレベルで、D3D や GL と比べて CPU 側コードの実装量が格段に増えるため、
		// アプリケーション層から直接利用するのには向いていない。
		// Direct3D 11 に近い上位ラッパーがないとつらい。

		glActiveTexture(GL_TEXTURE0 + slot);
	}

	static bool CheckBackBufferBestMSAAProperties(HDC dc)
	{
		GLint msaaSampleBuffers = 0, msaaSamples = 0;
		glGetIntegerv(GL_SAMPLE_BUFFERS, &msaaSampleBuffers);
		glGetIntegerv(GL_SAMPLES, &msaaSamples);
		ATLTRACE("OpenGL MSAA Sample Buffers = %d, Samples = %d\n", msaaSampleBuffers, msaaSamples);
		if (msaaSampleBuffers >= 1 && msaaSamples >= 2)
		{
			ATLTRACE("OpenGL MSAA may be enabled on system control panel.\n");
			// システム設定による MSAA が使えるらしい。
			// ただし Windows においてバック バッファに対する MSAA を常にアプリケーション側で制御する場合、
			// wglChoosePixelFormatARB() を使う必要がある。
			// 面倒なので MSAA バッファを使う際には最初からレンダーターゲット テクスチャを使って、バック バッファは非 MSAA にしたほうがよい。
		}
		if (!WGLEW_ARB_pixel_format || !WGLEW_ARB_multisample)
		{
			ATLTRACE("OpenGL MSAA not supported!!\n");
			return false;
		}

		const int msaaMaxSampleCount = GetMaxMSAASampleCount();
		ATLTRACE("OpenGL Max MSAA Sample Count = %d\n", msaaMaxSampleCount);

		int attributes[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			// バック バッファが GDI 互換フォーマットでない場合、OpenGL MSAA 能力はないと判断されるらしい。
			// GL_MAX_SAMPLES はフォーマットに関係ないみたいだが、
			// Direct3D では MSAA 能力を DXGI フォーマットごとに判定できるようになっていて、
			// おそらくバック バッファを非 GDI 互換で作成した上で MSAA を使うこともできる。が、
			// バック バッファは非 MSAA の GDI 互換フォーマットとして、
			// MSAA レンダーターゲット テクスチャを別に用意しておいたほうがよい。
			// なお、Direct3D では浮動小数バッファに対する MSAA は Feature Level 10.x 以上でないと保証されない。
			// OpenGL では 3.2 以降で標準化されているのか？（ES の場合は 3.0 以降で標準化されているのか？）
			//WGL_COLOR_BITS_ARB, 16 * 3, // NG.
			//WGL_ALPHA_BITS_ARB, 16, // NG.
			WGL_COLOR_BITS_ARB, 24,
			WGL_ALPHA_BITS_ARB, 8,
			// バック バッファに深度ステンシルは不要。
			WGL_DEPTH_BITS_ARB, 0,
			WGL_STENCIL_BITS_ARB, 0,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_SAMPLE_BUFFERS_ARB, 1,
			WGL_SAMPLES_ARB, 0, // MSAA サンプル数を変更してチェック。
			0, 0 // sentinel
		};

		_ASSERTE(attributes[19 - 1] == WGL_SAMPLES_ARB);
		for (int i = msaaMaxSampleCount; i >= 1; i /= 2)
		{
			attributes[19] = i;
			int pixelFormat = 0;
			UINT numFormats = 0;
			const BOOL retVal = wglChoosePixelFormatARB(dc, attributes, nullptr, 1, &pixelFormat, &numFormats);
			if (retVal && numFormats >= 1)
			{
				ATLTRACE("OpenGL Best MSAA Sample Count = %d\n", i);
				return true;
			}
		}
		ATLTRACE("No OpenGL MSAA available for the format.\n");
		return false;
	}


	bool MyOGLManager::Create(UINT width, UINT height, HWND hWnd)
	{
		// WGL 用の GDI サーフェイスの準備。ピクセル フォーマットを明示的に指定する。
		PIXELFORMATDESCRIPTOR pfd = {};

		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		//pfd.cColorBits = 16;
		//pfd.cColorBits = 32;
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
		//pfd.cDepthBits = 16;
		//pfd.cDepthBits = 24;
		//pfd.cStencilBits = 8;
		// FP16 ターシャリ バッファ（FBO）にレンダリングしたものを転送するだけなので、バック バッファはカラーだけでよい。
		// MSAA も使わない。
		// ちなみに PFD_SUPPORT_GDI というフラグも存在するが、これは PFD_DOUBLEBUFFER とは排他的らしい。
		// GDI の SwapBuffers() は実質的に OpenGL (WGL) 専用？
		// https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/ns-wingdi-tagpixelformatdescriptor
		// https://docs.microsoft.com/ja-jp/windows/desktop/api/Wingdi/nf-wingdi-swapbuffers

		// マルチ GPU 環境の場合、Direct3D 10 以降ではデバイス作成時に
		// DXGI アダプターを列挙して好きなものを使うことができる仕組みが用意されているが、
		// OpenGL ではバージョン 4.4 時点でもベンダー依存の拡張 API しか用意されていない模様。
		// 同一ベンダーのマルチ GPU 環境に関しては Windows XP + Direct3D 9 にもアダプター列挙機能があったが、
		// DXGI とは違って複数の異なるベンダーのドライバーは共存できなかったはず。
		// http://wlog.flatlib.jp/item/1326
		// http://www.xbitlabs.com/news/graphics/display/20090612123450_ATI_to_Boost_Efficiency_of_Multi_GPU_Operation_in_OpenGL.html
		// WGL_NV_gpu_affinity よりも、WGL_AMD_gpu_association のほうが API セットとしては分かりやすそう。
		// また、WGL_NV_gpu_affinity は Quadro 専用で、GeForce では使えないらしい。
		// WGL_AMD_gpu_association のほうは FirePro でも Radeon でも使えるらしい。

		m_hWnd = hWnd;
		_ASSERTE(m_hWnd != nullptr);
		m_hDC = ::GetDC(m_hWnd);
		_ASSERTE(m_hDC != nullptr);
		const int pixelFormat = ::ChoosePixelFormat(m_hDC, &pfd);
		// MSAA バック バッファを作成する場合、wglChoosePixelFormatARB() を使う必要がある。
		ATLTRACE(__FILE__"(%d): PixelFormat = %d\n", __LINE__, pixelFormat);
		::SetPixelFormat(m_hDC, pixelFormat, &pfd);
#if 0
		// 何も考えずレンダリング コンテキストを作成する方法。
		m_hGLRC = wglCreateContext(m_hDC);
		_ASSERTE(m_hGLRC != nullptr);
		wglMakeCurrent(m_hDC, m_hGLRC);
#else
		{
			auto dummyGLContext = wglCreateContext(m_hDC);
			_ASSERTE(dummyGLContext != nullptr);
			wglMakeCurrent(m_hDC, dummyGLContext);

			::SetLastError(0);
			// GLEW の初期化前なので、拡張関数のエントリーポイントはダミーコンテキストを作成し、カレントに設定した上で明示的に取り出す必要がある。
			auto wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
			_ASSERTE(wglCreateContextAttribsARB != nullptr);
			_ASSERTE(::GetLastError() == 0);

			const int majorVersion = 4;
			const int minorVersion = 4;
			const bool usesCoreProfile = false; // HACK: Core にすると NVIDIA 環境では glDebugMessageCallback() が取得できない？
			const int attributes[] =
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
				WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,
#ifdef _DEBUG
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#else
				WGL_CONTEXT_FLAGS_ARB, 0,
#endif
				WGL_CONTEXT_PROFILE_MASK_ARB, usesCoreProfile ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
				0, 0 // sentinel
			};
			// OpenGL 3.0 以降の機能や、対応する GLSL を使用するため、
			// OpenGL 3.0 コア プロファイルの前方互換コンテキストを作成する場合、wglCreateContextAttribsARB() と
			// WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB を使う必要があるらしい。
			// なお、NVIDIA の場合は従来の wglCreateContext() で作成したコンテキストでも OpenGL 3.x 以降の機能が
			// そのまま使用できるらしい（デフォルトで H/W やドライバーがサポートする最新のフル コンテキストになるらしい）。
			// http://wlog.flatlib.jp/?itemid=1382
			// 内部的には互換コンテキスト WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB のビットが立っていると思われる。
			// 
			// OpenGL 4.0 の GL_ARB_debug_output がサポートされている場合、
			// WGL_CONTEXT_FLAGS_ARB に WGL_CONTEXT_DEBUG_BIT_ARB フラグを追加すると、デバッグ コンテキストを作成できるらしい。
			// エラー発生時にコールバックされる関数は glDebugMessageCallbackARB() で登録する。
			// OpenGL API 関数を呼び出した後、glGetError() がエラーになる状況では必ずコールバックされるらしい。
			// ただ、Win32 API の GetLastError()/SetLastError() とは違い、
			// glSetError() がないのはどうかしてると思う。
			// また、OpenGL の API は Win32 API とは違って成功したときでもエラーをリセットしないので、最後に発生したエラーが残ったままになる。
			// デバッグ レイヤーがなかった時代はエラー発生箇所の特定が相当苦痛だったと思われる。
			m_hGLRC = wglCreateContextAttribsARB(m_hDC, nullptr, attributes);
			_ASSERTE(m_hGLRC != nullptr);
			wglMakeCurrent(m_hDC, m_hGLRC);
			wglDeleteContext(dummyGLContext);
			dummyGLContext = nullptr;
		}
#endif

		ATLTRACE(__FILE__"(%d): Now initializing GLEW...\n", __LINE__);
		const GLenum ret = glewInit();
		if (ret != GLEW_OK)
		{
			// glGetString(), glewGetString(), glewGetErrorString() はいずれも const GLubyte* を返すので、
			// std::string コンストラクタと相性が悪い。
			// パフォーマンスはやや劣るが、std::stringstream で文字列化するのが安全で楽。

			//const std::string strGlewErrMsg(reinterpret_cast<const char*>(glewGetErrorString(ret)));
			//ATLTRACE("[GLEW Error]: %s\n", strGlewErrMsg.c_str());
			std::stringstream glinfo;
			glinfo << "[GLEW Error]: " << glewGetErrorString(ret) << std::endl;
			ATLTRACE(glinfo.str().c_str());
			return false;
		}

		// OpenGL バージョンをチェックする。
		// 標準化されたジオメトリ シェーダーを使う場合は 3.2 が必須。
		// 標準化されたテッセレーション シェーダーを使う場合は 4.0 が必須。
		// 標準化されたコンピュート シェーダーを使う場合は 4.3 が必須。
		// GLEW は 1.9 で OpenGL 4.3 に、1.10 で 4.4 に、1.11 で 4.5 に対応している。
		const GLboolean isOpenGL_4_0 = glewIsSupported("GL_VERSION_4_0");
		ATLTRACE("OpenGL 4.0 support: %s\n", isOpenGL_4_0 ? "Yes" : "No");
		const GLboolean isOpenGL_4_3 = glewIsSupported("GL_VERSION_4_3");
		ATLTRACE("OpenGL 4.3 support: %s\n", isOpenGL_4_3 ? "Yes" : "No");
		const GLboolean isOpenGL_4_4 = glewIsSupported("GL_VERSION_4_4");
		ATLTRACE("OpenGL 4.4 support: %s\n", isOpenGL_4_4 ? "Yes" : "No");
		const GLboolean isOpenGL_4_5 = glewIsSupported("GL_VERSION_4_5");
		ATLTRACE("OpenGL 4.5 support: %s\n", isOpenGL_4_5 ? "Yes" : "No");

		std::stringstream glinfo;
		glinfo << "----- OpenGL information -----" << std::endl;
		glinfo << "Vendor   : " << glGetString(GL_VENDOR) << std::endl;
		glinfo << "Renderer : " << glGetString(GL_RENDERER) << std::endl;
		glinfo << "Version  : " << glGetString(GL_VERSION) << std::endl;
		glinfo << "GLSL     : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
		glinfo << "GLEW     : " << glewGetString(GLEW_VERSION) << std::endl;
		ATLTRACE(glinfo.str().c_str());

		// MSAA 能力をチェックする。
		CheckBackBufferBestMSAAProperties(m_hDC);

		// その他の能力をチェックする。
		{
			const int maxUniformBlockSize = GetMaxUniformBlockSize();
			ATLTRACE("OpenGL Max Uniform Block Size = %d\n", maxUniformBlockSize);
			const int maxSubroutineCount = GetMaxSubroutineCount();
			ATLTRACE("OpenGL Max Subroutine Count = %d\n", maxSubroutineCount);
		}

		// Direct3D からの移植を容易にする互換機能が欲しいので、OpenGL 4.4 は必須。
		if (!isOpenGL_4_4)
		{
			AfxMessageBox(_T("Your computer does not support OpenGL 4.4!!"));
			return false;
		}

#ifdef _DEBUG
		glDebugMessageCallback(OutputGLDebugMessage, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
#endif

		// 垂直同期（VSync）を OFF にする。
#if 1
		SetSwapInterval(0);
#endif
		// Direct3D と同時描画している際、垂直同期を切るとフレームレートが安定しない現象が発生する？
		// 性能評価のためには VSync を切る必要がある。
		// なお、Direct3D 10/11 には速度性能などを測定するときに使える ID3D10Query/ID3D11Query が存在するが、
		// OpenGL には拡張として GL_EXT_timer_query がある。
		// http://wlog.flatlib.jp/item/1387

		if (!this->ResizeScreen(width, height))
		{
			return false;
		}

		if (!this->CreateMyDummyWhiteTexture())
		{
			return false;
		}

		if (!CreateDummyCubeTexture(m_dummyCubeTexture))
		{
			return false;
		}

		if (!this->CreateMyToonShadingRefTexture())
		{
			return false;
		}

		if (!this->CreateMyFontTexture())
		{
			return false;
		}

		if (!m_fontRects.CreateEx())
		{
			return false;
		}

		{
			CPathW pathShaderDir(m_pathMediaDir);
			//pathShaderDir += L"..\\FbxModelMonitor\\MyGLSL";
			//pathShaderDir += L"..\\..\\FbxModelMonitor\\MyGLSL";

			// シェーダー プログラムの作成。
			{
				CPathW pathVertFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathVertFiles[0] += L"MyGlslVersion.glsl";
				pathVertFiles[1] += L"MyUniformBlocks.glsl";
				pathVertFiles[2] += L"Skinning.vert";
				LPCWSTR vertFileNamesArray[] =
				{
					pathVertFiles[0].m_strPath.GetString(),
					pathVertFiles[1].m_strPath.GetString(),
					pathVertFiles[2].m_strPath.GetString(),
				};

				CPathW pathFragFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathFragFiles[0] += L"MyGlslVersion.glsl";
				pathFragFiles[1] += L"MyUniformBlocks.glsl";
				pathFragFiles[2] += L"DiffuseSpecularFunc.glsl";
				pathFragFiles[3] += L"BlinnPhong.frag";
				LPCWSTR fragFileNamesArray[] =
				{
					pathFragFiles[0].m_strPath.GetString(),
					pathFragFiles[1].m_strPath.GetString(),
					pathFragFiles[2].m_strPath.GetString(),
					pathFragFiles[3].m_strPath.GetString(),
				};

				m_programObjSkinningPhong = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramVSFSFromFile(m_programObjSkinningPhong.get(),
					vertFileNamesArray, ARRAYSIZE(vertFileNamesArray),
					fragFileNamesArray, ARRAYSIZE(fragFileNamesArray)))
				{
					return false;
				}

				m_locationsSkinningPhong.FindLocationsInProgram(m_programObjSkinningPhong.get());
			}

			{
				CPathW pathVertFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathVertFiles[0] += L"MyGlslVersion.glsl";
				pathVertFiles[1] += L"MyUniformBlocks.glsl";
				pathVertFiles[2] += L"ShadingLessPC.vert";
				LPCWSTR vertFileNamesArray[] =
				{
					pathVertFiles[0].m_strPath.GetString(),
					pathVertFiles[1].m_strPath.GetString(),
					pathVertFiles[2].m_strPath.GetString(),
				};

				m_programObjShadingLess = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramVSFSFromFile(m_programObjShadingLess.get(),
					vertFileNamesArray, ARRAYSIZE(vertFileNamesArray)))
				{
					return false;
				}

				m_locationsShadingLess.FindLocationsInProgram(m_programObjShadingLess.get());
			}

			{
				CPathW pathVertFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathVertFiles[0] += L"MyGlslVersion.glsl";
				pathVertFiles[1] += L"MyUniformBlocks.glsl";
				pathVertFiles[2] += L"ShadingLessPCT.vert";
				LPCWSTR vertFileNamesArray[] =
				{
					pathVertFiles[0].m_strPath.GetString(),
					pathVertFiles[1].m_strPath.GetString(),
					pathVertFiles[2].m_strPath.GetString(),
				};

				CPathW pathFragFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathFragFiles[0] += L"MyGlslVersion.glsl";
				pathFragFiles[1] += L"MyUniformBlocks.glsl";
				pathFragFiles[2] += L"FetchAlphaMap.frag";
				LPCWSTR fragFileNamesArray[] =
				{
					pathFragFiles[0].m_strPath.GetString(),
					pathFragFiles[1].m_strPath.GetString(),
					pathFragFiles[2].m_strPath.GetString(),
				};

				m_programObjFontSprite = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramVSFSFromFile(m_programObjFontSprite.get(),
					vertFileNamesArray, ARRAYSIZE(vertFileNamesArray),
					fragFileNamesArray, ARRAYSIZE(fragFileNamesArray)))
				{
					return false;
				}

				m_locationsFontSprite.FindLocationsInProgram(m_programObjFontSprite.get());
			}

			{
				CPathW pathVertFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathVertFiles[0] += L"MyGlslVersion.glsl";
				pathVertFiles[1] += L"MyUniformBlocks.glsl";
				pathVertFiles[2] += L"TransformLessPCT.vert";
				LPCWSTR vertFileNamesArray[] =
				{
					pathVertFiles[0].m_strPath.GetString(),
					pathVertFiles[1].m_strPath.GetString(),
					pathVertFiles[2].m_strPath.GetString(),
				};

				CPathW pathFragFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathFragFiles[0] += L"MyGlslVersion.glsl";
				pathFragFiles[1] += L"MyUniformBlocks.glsl";
				pathFragFiles[2] += L"MSAATransport.frag";
				LPCWSTR fragFileNamesArray[] =
				{
					pathFragFiles[0].m_strPath.GetString(),
					pathFragFiles[1].m_strPath.GetString(),
					pathFragFiles[2].m_strPath.GetString(),
				};

				m_programObjMSAATransport = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramVSFSFromFile(m_programObjMSAATransport.get(),
					vertFileNamesArray, ARRAYSIZE(vertFileNamesArray),
					fragFileNamesArray, ARRAYSIZE(fragFileNamesArray)))
				{
					return false;
				}

				m_locationsMSAATransport.FindLocationsInProgram(m_programObjMSAATransport.get());
			}

			{
				CPathW pathVertFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathVertFiles[0] += L"MyGlslVersion.glsl";
				pathVertFiles[1] += L"MyUniformBlocks.glsl";
				pathVertFiles[2] += L"WaveSim.vert";
				LPCWSTR vertFileNamesArray[] =
				{
					pathVertFiles[0].m_strPath.GetString(),
					pathVertFiles[1].m_strPath.GetString(),
					pathVertFiles[2].m_strPath.GetString(),
				};

				CPathW pathFragFiles[] =
				{
					pathShaderDir,
					pathShaderDir,
					pathShaderDir,
				};
				pathFragFiles[0] += L"MyGlslVersion.glsl";
				pathFragFiles[1] += L"MyUniformBlocks.glsl";
				pathFragFiles[2] += L"WaveSim.frag";
				LPCWSTR fragFileNamesArray[] =
				{
					pathFragFiles[0].m_strPath.GetString(),
					pathFragFiles[1].m_strPath.GetString(),
					pathFragFiles[2].m_strPath.GetString(),
				};

				m_programObjDisplayWaveSim = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramVSFSFromFile(m_programObjDisplayWaveSim.get(),
					vertFileNamesArray, ARRAYSIZE(vertFileNamesArray),
					fragFileNamesArray, ARRAYSIZE(fragFileNamesArray)))
				{
					return false;
				}

				m_locationsDisplayWaveSim.FindLocationsInProgram(m_programObjDisplayWaveSim.get());
			}

			{
				CPathW pathComputeFile(pathShaderDir);
				pathComputeFile += L"SimpleComputingTest.glsl";

				m_programObjSimpleComputingTest = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramCSFromFile(m_programObjSimpleComputingTest.get(), pathComputeFile))
				{
					return false;
				}

				m_locationsSimpleComputingTest.FindLocationsInProgram(m_programObjSimpleComputingTest.get());

				GLenum lastErrorCode = 0;
#if 0
				// Uniform Block のインデックスを名前から取得する場合、glGetUniformBlockIndex() を使う。
				// glGetProgramResourceIndex() と GL_UNIFORM_BLOCK でもいける？
				// Shader Storage Block の場合は glGetProgramResourceIndex() と GL_SHADER_STORAGE_BLOCK を使う。
				// image2D の場合は glGetProgramResourceIndex() と GL_UNIFORM を使うらしい。
				auto indexStorageInputBuffer0F4 = glGetProgramResourceIndex(m_programObjSimpleComputingTest.get(), GL_SHADER_STORAGE_BLOCK, "StorageInputBuffer0F4");
				lastErrorCode = glGetError();
				_ASSERTE(lastErrorCode == GL_NO_ERROR && indexStorageInputBuffer0F4 != GL_INVALID_INDEX);
				const GLuint bindingStorageInputBuffer0F4 = 0;
				glShaderStorageBlockBinding(m_programObjSimpleComputingTest.get(), indexStorageInputBuffer0F4, bindingStorageInputBuffer0F4);
				// glShaderStorageBlockBinding() でマニュアル設定したバインディング値と同じ値を、glBindBufferBase() に渡すことになる。
				// プログラム リソース インデックスとバインディング値は別物なので注意。
				// OpenGL 4.3 以降では、GLSL 側の buffer/image2D などの layout の binding パラメータでも指定できるはず（≒オートマティック）。
				// GLSL 側でハード コーディングした値を glBindBufferBase() に指定すればよい。
				// in 変数や uniform ブロックの layout の location パラメータと同様。
				// HLSL では b#, t#, u# などのレジスター番号（スロット番号）をハード コーディングする方法のみだが、
				// GLSL では以前の uniform や in 同様に間接テーブル参照方式も可能な仕様にしたらしい（疎結合にして柔軟性を持たせるため？）。
				// 正直ごちゃごちゃして分かりにくくなるだけの余計な機能だと思うが……

				auto indexStorageOutputBuffer0F4 = glGetProgramResourceIndex(m_programObjSimpleComputingTest.get(), GL_SHADER_STORAGE_BLOCK, "StorageOutputBuffer0F4");
				lastErrorCode = glGetError();
				_ASSERTE(lastErrorCode == GL_NO_ERROR && indexStorageOutputBuffer0F4 != GL_INVALID_INDEX);
				const GLuint bindingStorageOutputBuffer0F4 = 1;
				glShaderStorageBlockBinding(m_programObjSimpleComputingTest.get(), indexStorageOutputBuffer0F4, bindingStorageOutputBuffer0F4);
#endif
				// インデックスの確認。関数パラメータが異常だったりすると GL_INVALID_ENUM エラーが生成されるらしい。
				// OpenGL はエラーコードもパラメータ定数も戻り値定数もすべて enum でなくマクロによって節操なく定義されているので、どれがどれだかまったく分からなくなる。
				// image2D に関しては sampler2D などのように uniform 修飾するので、glBindBufferBase() でなく glUniform1i() を使うらしい？
				// glBindBufferBase() を使うこともできる？
				// やはり OpenGL/GLSL は仕様や文法が分かりにくい。むしろ Direct3D のように古い互換性を捨ててすべて統一的に glBindBufferBase() を使うようにすればいいのに……
				const auto indexUniWaveSimInputImage = glGetProgramResourceIndex(m_programObjSimpleComputingTest.get(), GL_UNIFORM, "UniWaveSimInputImage");
				lastErrorCode = glGetError();
				_ASSERTE(lastErrorCode == GL_NO_ERROR && indexUniWaveSimInputImage != GL_INVALID_INDEX);
				const auto indexUniWaveSimOutputImage = glGetProgramResourceIndex(m_programObjSimpleComputingTest.get(), GL_UNIFORM, "UniWaveSimOutputImage");
				lastErrorCode = glGetError();
				_ASSERTE(lastErrorCode == GL_NO_ERROR && indexUniWaveSimOutputImage != GL_INVALID_INDEX);
			}

			{
				CPathW pathComputeFile(pathShaderDir);
				pathComputeFile += L"UpdateRandomTable.glsl";

				m_programObjUpdateRandomTable = Factory::CreateProgramPtr();

				if (!CreateMyShaderProgramCSFromFile(m_programObjUpdateRandomTable.get(), pathComputeFile))
				{
					return false;
				}

				// SSBO のロケーションを glGetUniformLocation() で取得するようなことはできないらしい。
				// glGetProgramResourceIndex() と glShaderStorageBlockBinding() を使って実行時に与えるバインディング値か、
				// layout:binding で指定した固定値を glBindBufferBase() で指定するらしい。
				// なお、glGetProgramResourceiv() を使えば、GLSL 側で指定した値を取得できる。

				GLenum lastErrorCode = 0;

				// インデックスの確認。
				const auto indexSBufferRandomNumTable = glGetProgramResourceIndex(m_programObjUpdateRandomTable.get(), GL_SHADER_STORAGE_BLOCK, "SBufferRandomNumTable");
				lastErrorCode = glGetError();
				_ASSERTE(lastErrorCode == GL_NO_ERROR && indexSBufferRandomNumTable != GL_INVALID_INDEX);

				const GLenum inProps[] = { GL_BUFFER_BINDING };
				GLsizei actualOutputLength = 0;
				GLint outParams[ARRAYSIZE(inProps)] = {};

				glGetProgramResourceiv(m_programObjUpdateRandomTable.get(), GL_SHADER_STORAGE_BLOCK, indexSBufferRandomNumTable,
					ARRAYSIZE(inProps), inProps, ARRAYSIZE(outParams), &actualOutputLength, outParams);
				ATLTRACE("buffer binding = %d\n", outParams[0]);
#if 0
				const GLenum inProps[] = { GL_ARRAY_SIZE, GL_TYPE, GL_ARRAY_STRIDE };
				GLsizei actualOutputLength = 0;
				GLint outParams[ARRAYSIZE(inProps)] = {};

				const auto index = glGetProgramResourceIndex(m_programObjUpdateRandomTable.get(), GL_BUFFER_VARIABLE, "foo.ary");
				glGetProgramResourceiv(m_programObjUpdateRandomTable.get(), GL_BUFFER_VARIABLE, index,
					ARRAYSIZE(inProps), inProps, ARRAYSIZE(outParams), &actualOutputLength, outParams);
				ATLTRACE("buffer index = %d, actualOutputLength = %d, outParams = { %d, 0x%x, %d }\n", index, actualOutputLength, outParams[0], outParams[1], outParams[2]);
#endif
			}
		}

		// VAO の作成とステートの登録。OpenGL 3.0 以降の標準機能。
		// OpenGL 4.3 では頂点属性の登録時にもはや頂点バッファのバインドやプログラム オブジェクトの有効化は不要。完全に独立管理できる。
		{
			m_inputLayoutPC = Factory::CreateOneVertexArrayPtr();
			VertexStreamInputLayoutHelperForPC::SaveVertexStreamInputLayoutStateToVao(m_inputLayoutPC);

			m_inputLayoutPCT = Factory::CreateOneVertexArrayPtr();
			VertexStreamInputLayoutHelperForPCT::SaveVertexStreamInputLayoutStateToVao(m_inputLayoutPCT);

			m_inputLayoutPCNT = Factory::CreateOneVertexArrayPtr();
			VertexStreamInputLayoutHelperForPCNT::SaveVertexStreamInputLayoutStateToVao(m_inputLayoutPCNT);

			m_inputLayoutPNTIW = Factory::CreateOneVertexArrayPtr();
			VertexStreamInputLayoutHelperForPNTIW::SaveVertexStreamInputLayoutStateToVao(m_inputLayoutPNTIW);
		}

		// UBO の作成。OpenGL 3.1 以降の標準機能。
		{
			if (!CreateUniformBuffer(&CBufferBoneMatrixPalettePack(), m_cbufferBoneMatrixPalette))
			{
				return false;
			}
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_cbufferBoneMatrixPalette.get());
			if (!CreateUniformBuffer(&CBufferViewParamsPack(), m_cbufferViewParams))
			{
				return false;
			}
			glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_cbufferViewParams.get());
			if (!CreateUniformBuffer(&CBufferMeshPartAttributePack(), m_cbufferMeshPartAttribute))
			{
				return false;
			}
			glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_cbufferMeshPartAttribute.get());
			if (!CreateUniformBuffer(&CBufferLightParamsPack(), m_cbufferLightParams))
			{
				return false;
			}
			glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_cbufferLightParams.get());
		}

		// サンプラーの作成。OpenGL 3.3 以降の標準機能。
		{
			// OpenGL では境界色のデフォルトは黒らしいので注意。
			// トゥーン シェーディング グラデーション参照テクスチャのフィルタはクランプ モード。
			// デフォルトは GL_REPEAT が設定されている（D3D でいう Wrap モード）。
			// GL_CLAMP と GL_CLAMP_TO_EDGE と GL_CLAMP_TO_BORDER は何が違う？
			// D3D でいう Clamp モードに相当するのが GL_CLAMP で、
			// D3D でいう Border モードに相当するのが GL_CLAMP_TO_BORDER らしい。
			// 他にも、D3D でいう Mirror モードに相当する GL_MIRRORED_REPEAT もある。
			// GL_CLAMP_TO_EDGE は GL_CLAMP と微妙に異なる。バイリニア補間するときにその違いが効いてくる模様。
			// GL_CLAMP_TO_EDGE に相当する D3D アドレッシング モードは存在しない？
			// http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20080822
			// http://open.gl/textures
			// ミップマップを含まないテクスチャに対して GL_NEAREST_MIPMAP_NEAREST や GL_LINEAR_MIPMAP_LINEAR を使うと、
			// Sampler Object が正常に動かない模様。
			// GL_NEAREST や GL_LINEAR を使う必要がある。
			// Direct3D だとミップマップなしでも D3D11_FILTER_MIN_MAG_MIP_POINT や D3D11_FILTER_MIN_MAG_MIP_LINEAR で OK なのに……
			// てことはミップマップ補間（トライリニア補間）する場合は専用のサンプラーオブジェクトを
			// 作らないとダメってことらしい。あんまりうまみがない……
			// と思っていたが、テクスチャ作成直後に GL_TEXTURE_MAX_LEVEL を 0 に設定すればミップマップなしのテクスチャでも
			// GL_NEAREST_MIPMAP_NEAREST や GL_LINEAR_MIPMAP_LINEAR が使えそう。
			// http://dench.flatlib.jp/opengl/glsl_hlsl
			// https://sites.google.com/site/monshonosuana/opengl/opengl_004

			m_samplerPointWrap = Factory::CreateOneSamplerPtr();
			_ASSERTE(m_samplerPointWrap);
			glSamplerParameteri(m_samplerPointWrap.get(), GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glSamplerParameteri(m_samplerPointWrap.get(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glSamplerParameteri(m_samplerPointWrap.get(), GL_TEXTURE_WRAP_S, GL_REPEAT);
			glSamplerParameteri(m_samplerPointWrap.get(), GL_TEXTURE_WRAP_T, GL_REPEAT);
			glSamplerParameteri(m_samplerPointWrap.get(), GL_TEXTURE_WRAP_R, GL_REPEAT);
			glSamplerParameterfv(m_samplerPointWrap.get(), GL_TEXTURE_BORDER_COLOR, &MyMath::COLOR4F_WHITE.x);

			m_samplerLinearWrap = Factory::CreateOneSamplerPtr();
			_ASSERTE(m_samplerLinearWrap);
			glSamplerParameteri(m_samplerLinearWrap.get(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glSamplerParameteri(m_samplerLinearWrap.get(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(m_samplerLinearWrap.get(), GL_TEXTURE_WRAP_S, GL_REPEAT);
			glSamplerParameteri(m_samplerLinearWrap.get(), GL_TEXTURE_WRAP_T, GL_REPEAT);
			glSamplerParameteri(m_samplerLinearWrap.get(), GL_TEXTURE_WRAP_R, GL_REPEAT);
			glSamplerParameterfv(m_samplerLinearWrap.get(), GL_TEXTURE_BORDER_COLOR, &MyMath::COLOR4F_WHITE.x);

			m_samplerLinearClamp = Factory::CreateOneSamplerPtr();
			_ASSERTE(m_samplerLinearClamp);
			glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_WRAP_S, GL_CLAMP);
			glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_WRAP_T, GL_CLAMP);
			glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_WRAP_R, GL_CLAMP);
			glSamplerParameterfv(m_samplerLinearClamp.get(), GL_TEXTURE_BORDER_COLOR, &MyMath::COLOR4F_WHITE.x);
			//glSamplerParameterf(m_samplerLinearClamp.get(), GL_TEXTURE_MIN_LOD, -FLT_MAX);
			//glSamplerParameterf(m_samplerLinearClamp.get(), GL_TEXTURE_MAX_LOD, +FLT_MAX);
			//glSamplerParameterf(m_samplerLinearClamp.get(), GL_TEXTURE_LOD_BIAS, 0.0f);
			//glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_COMPARE_MODE, GL_NONE);
			//glSamplerParameteri(m_samplerLinearClamp.get(), GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			//glSamplerParameterf(m_samplerLinearClamp.get(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

			// 描画時には glBindSampler() でバインドする。第1引数（unit パラメータ）には、
			// GL_TEXTURE0 のユニットにバインドしたい場合は 0 を、
			// GL_TEXTURE1 のユニットにバインドしたい場合は 1 を指定する。
		}

		// 座標軸用頂点バッファの作成。
		if (!this->CreateCoordAxisLineVertexBufferPC(m_coordAxisLineVertexBufferPC))
		{
			return false;
		}

		if (!this->CreateOneSquareVertexBufferPCT(m_oneSquareVertexBufferPCT))
		{
			return false;
		}

		if (!this->CreateWaveFrontPlaneVertexBufferPCNT(m_waveFrontPlaneVertexBufferPCNT))
		{
			return false;
		}

		if (!this->CreateOneQuadIndexBuffer(m_oneQuadIndexBuffer))
		{
			return false;
		}

#if 0
		{
			const uint32_t vertexCount = 4U;
			const MyMath::MyVertexPCT pVerticesArray[vertexCount] =
			{
				// { Pos, Color, Tex }
				{ MyMath::Vector3F(-1, +1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(0, 0) },
				{ MyMath::Vector3F(+1, +1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(1, 0) },
				{ MyMath::Vector3F(-1, -1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(0, 1) },
				{ MyMath::Vector3F(+1, -1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(1, 1) },
			};

			m_squareMeshPCT.CreateMesh(pVerticesArray, vertexCount, MyMath::OneQuadIndicesArray021, MyMath::OneQuadIndexCount);
		}
#endif

		// とりあえず Direct3D 版と OpenGL 版との比較をするため、同じ乱数シードを与える。
		std::srand(0);
		std::vector<MyMath::Vector4UI> initialRandomNumbers(RANDOM_NUM_TABLE_DATA_COUNT);
		std::for_each(initialRandomNumbers.begin(), initialRandomNumbers.end(),
			[](MyMath::Vector4UI& x) { x = MyMath::Xorshift128Random::CreateInitialNumber(std::rand()); });

		// コンピュート シェーダー用 SSBO の作成。
		if (!CreateStorageBuffer(sizeof(MyMath::Vector4UI), RANDOM_NUM_TABLE_DATA_COUNT, &initialRandomNumbers[0], m_ssboRandomNumTableBuffer))
		{
			return false;
		}

		// コンピュート シェーダーと他のシェーダーで相互運用するための、
		// RW テクスチャの作成。
		// OpenGL の場合、Direct3D とは違って特に Unordered Access のためのフラグを作成時に立てる必要はないらしい。
		// 普通のテクスチャとまったく同じ要領で作成できる。
		// ただし使用時にバインドするための関数は glBindTexture() でなく glBindImageTexture() になるので注意。
		{
			m_waveSimWorkTextures[0] = Factory::CreateOneTexturePtr();
			m_waveSimWorkTextures[1] = Factory::CreateOneTexturePtr();

			const GLsizei width = COMPUTING_TEMP_WORK_SIZE;
			const GLsizei height = COMPUTING_TEMP_WORK_SIZE;

			// ID3D11Device::CreateTexture2D() は pInitialData に NULL を指定すると、ゼロで初期化してくれるらしい。
			// 一方で glTexImage2D() は pixels に NULL を指定した場合、未定義データになるらしい。
			// ゼロクリアされている可能性もあるし、ゴミデータが入ったままの可能性もある。
			// OpenGL 4.4 では GL_ARB_clear_texture として glClearTexImage(), glClearTexSubImage() が追加されている。
			// Direct3D 11 には
			// ID3D11DeviceContext::ClearRenderTargetView(),
			// ID3D11DeviceContext::ClearDepthStencilView(),
			// ID3D11DeviceContext::CopyResource(),
			// ID3D11DeviceContext::CopySubresourceRegion(),
			// は存在するが、テクスチャの領域クリア機能に直接相当するものはなさそう。
			// Direct3D 11.1 では
			// ID3D11DeviceContext1::ClearView() が追加されている。
			const std::vector<MyMath::Vector4F> initData(width * height, MyMath::ZERO_VECTOR4F);
			for (int i = 0; i < 2; ++i)
			{
				glBindTexture(GL_TEXTURE_2D, m_waveSimWorkTextures[i].get());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#if 0
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, &initData[0]);
			}

			glBindTexture(GL_TEXTURE_2D, 0);
		}
#if 0
		if (!CreateStorageBuffer(sizeof(MyMath::Vector4F), ComputingTempDataCount, nullptr, m_ssboWaveSimWorkBuffers[0]))
		{
			return false;
		}
		if (!CreateStorageBuffer(sizeof(MyMath::Vector4F), ComputingTempDataCount, nullptr, m_ssboWaveSimWorkBuffers[1]))
		{
			return false;
		}
#endif

		this->InitializeCameraSettings();

		// GLSL 変数への値セットは glUseProgram() 呼び出し後でないと意味がない。

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
#ifdef USE_LEFT_HAND_COORD_SYS
		glCullFace(GL_FRONT);
#else
		glCullFace(GL_BACK);
#endif
		//glEnable(GL_LIGHTING); // 固定機能はもはや使わないので意味がない。

		m_isInitialized = true;

		return true;
	}

	bool MyOGLManager::CreateMyFontTexture()
	{
		_ASSERTE(m_pHudFontTexData != nullptr);
		_ASSERTE(m_hudFontAlphaTexture == nullptr);

		if (!MyOGL::CreateFontAlphaTexture(m_pHudFontTexData->TextureData, m_hudFontAlphaTexture))
		{
			return false;
		}

		return true;
	}

	void MyOGLManager::Destroy()
	{
		this->ClearMainTexTable();
		this->ClearMainMeshArray();

		// HACK: シェーダープログラム、バッファなど、すべてのリソースを破棄してから OpenGL コンテキストを解放するべき。

		if (m_hGLRC)
		{
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(m_hGLRC);
			m_hGLRC = nullptr;
		}

		if (m_hDC && m_hWnd)
		{
			_ASSERTE(::IsWindow(m_hWnd));
			::ReleaseDC(m_hWnd, m_hDC);
			m_hDC = nullptr;
			m_hWnd = nullptr;
		}

		m_isInitialized = false;
	}

	//! @brief  座標軸ライン用のライティング済み・未トランスフォーム頂点バッファを作成する。<br>
	bool MyOGLManager::CreateCoordAxisLineVertexBufferPC(BufferResourcePtr& outVertexBuffer)
	{
		_ASSERTE(outVertexBuffer.get() == 0);

		const uint32_t vertexCount = 6U;
		const float axisLength = 300;
		const MyVertexTypes::MyVertexPC pVerticesArray[vertexCount] =
		{
			// { Pos, Color }
			{ MyMath::Vector3F(0, 0, 0),          MyMath::Vector4F(1, 0, 0, 1) },
			{ MyMath::Vector3F(axisLength, 0, 0), MyMath::Vector4F(1, 0, 0, 1) },
			{ MyMath::Vector3F(0, 0, 0),          MyMath::Vector4F(0, 1, 0, 1) },
			{ MyMath::Vector3F(0, axisLength, 0), MyMath::Vector4F(0, 1, 0, 1) },
			{ MyMath::Vector3F(0, 0, 0),          MyMath::Vector4F(0, 0, 1, 1) },
			{ MyMath::Vector3F(0, 0, axisLength), MyMath::Vector4F(0, 0, 1, 1) },
		};

		outVertexBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(outVertexBuffer.get() != 0);
		if (outVertexBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, outVertexBuffer.get());
		glBufferData(GL_ARRAY_BUFFER, sizeof(pVerticesArray), pVerticesArray, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return true;
	}

	bool MyOGLManager::CreateOneSquareVertexBufferPCT(BufferResourcePtr& outVertexBuffer)
	{
		_ASSERTE(outVertexBuffer.get() == 0);

		const uint32_t vertexCount = 4U;
		const MyVertexTypes::MyVertexPCT pVerticesArray[vertexCount] =
		{
			// { Pos, Color, Tex }
			{ MyMath::Vector3F(-1, +1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+1, +1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(1, 0) },
			{ MyMath::Vector3F(-1, -1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(0, 1) },
			{ MyMath::Vector3F(+1, -1, 0), MyMath::Vector4F(1.0f, 1.0f, 1.0f, 1), MyMath::Vector2F(1, 1) },
		};

		outVertexBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(outVertexBuffer.get() != 0);
		if (outVertexBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, outVertexBuffer.get());
		glBufferData(GL_ARRAY_BUFFER, sizeof(pVerticesArray), pVerticesArray, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return true;
	}

	bool MyOGLManager::CreateWaveFrontPlaneVertexBufferPCNT(BufferResourcePtr& outVertexBuffer)
	{
		_ASSERTE(outVertexBuffer.get() == 0);

		const uint32_t vertexCount = 4U;
		const float planeLength = 1;
		const float planeHeight = 0;
		// HACK: 水面の法線はシミュレーションで分割単位（グリッド）ごとに計算するので、メッシュ頂点法線は不要？
		// 海面の盛り上がりなどの大きな変形でテッセレーションを活用する場合も、GPU 側で計算したほうが良さげ。
		const MyVertexTypes::MyVertexPCNT pVerticesArray[vertexCount] =
		{
			// { Pos, Color, Normal, Tex }
			{ MyMath::Vector3F(-planeLength, planeHeight, -planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+planeLength, planeHeight, -planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(1, 0) },
			{ MyMath::Vector3F(-planeLength, planeHeight, +planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(0, 1) },
			{ MyMath::Vector3F(+planeLength, planeHeight, +planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(1, 1) },
			// LT, RT, LB, RB.（左手系の定義順）
		};

		outVertexBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(outVertexBuffer.get() != 0);
		if (outVertexBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, outVertexBuffer.get());
		glBufferData(GL_ARRAY_BUFFER, sizeof(pVerticesArray), pVerticesArray, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return true;
	}

	//! @brief  四辺形用のインデックス バッファを作成する。<br>
	bool MyOGLManager::CreateOneQuadIndexBuffer(BufferResourcePtr& outIndexBuffer)
	{
		_ASSERTE(outIndexBuffer.get() == 0);

		const uint32_t indexCount = MyMath::OneQuadIndexCount;
		const uint16_t* pIndicesArray =
#ifdef USE_LEFT_HAND_COORD_SYS
			MyMath::OneQuadIndicesArray012;
#else
			MyMath::OneQuadIndicesArray021;
#endif

		outIndexBuffer = Factory::CreateOneBufferPtr();
		_ASSERTE(outIndexBuffer.get() != 0);
		if (outIndexBuffer.get() == 0)
		{
			return false;
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outIndexBuffer.get());
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*pIndicesArray) * indexCount, pIndicesArray, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		return true;
	}


	bool MyOGLManager::CreateMyDummyWhiteTexture()
	{
		m_dummyWhiteTexture = Factory::CreateOneTexturePtr();
		if (m_dummyWhiteTexture.get() == 0)
		{
			return false;
		}

		const uint32_t texW = MyTextureHelper::DUMMY_WHITE_TEX_SIZE;
		const uint32_t texH = MyTextureHelper::DUMMY_WHITE_TEX_SIZE;

		glBindTexture(GL_TEXTURE_2D, m_dummyWhiteTexture.get());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#if 0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif

		// デフォルトで白色。
		const std::vector<uint32_t> textureBuf(texW * texH, 0xFFFFFFFF);

		const int inoutPixelFormat = GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, inoutPixelFormat,
			texW,
			texH,
			0, inoutPixelFormat, GL_UNSIGNED_BYTE, &textureBuf[0]);
		const auto lastError = glGetError();

		glBindTexture(GL_TEXTURE_2D, 0);

		return lastError == GL_NO_ERROR;
	}

	bool MyOGLManager::CreateMyToonShadingRefTexture()
	{
		_ASSERTE(m_pToonShadingDiffuseCoefRefTexData != nullptr);

		m_toonShadingRefTexture = Factory::CreateOneTexturePtr();
		if (m_toonShadingRefTexture.get() == 0)
		{
			return false;
		}

		const uint32_t texW = MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;
		const uint32_t texH = MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;

		// glTexParameteri(), glTexParameteriv(), glTexParameterf(), glTexParameterfv() は設定用途（第2引数）に応じて使い分けが必要。 
		// Web 上のサンプルでは、glTexParameteri() の代わりに glTexParameterf() を間違って使っているものがあるが、
		// それでも普通に動いているらしいのはなぜだ……
		// どうも glTexParameterf() の第3引数に float 値を渡したとしても第2引数の種類に応じて int に変換され、
		// glTexParameteri() を呼び出した場合と等価になるため、
		// float で表現できる範囲であればなんとか動いているらしい。だが間違っている。
		// ～f() のほうは GL_TEXTURE_MIN_LOD, GL_TEXTURE_MAX_LOD などを指定するときに使うもの。
		// これはそもそも OpenGL API の設計がまずい気がする。
		// Direct3D はオブジェクトのプロパティの設定に構造体を使う方式なので、
		// 新しくパラメータを追加するときに構造体の仕様も変わってしまうという欠点があるが、
		// OpenGL と違って型安全で、また設定可能なプロパティを把握しやすいという利点がある。
		// とは言っても列挙型に C++11 の enum class は使われておらず、完全にタイプセーフとは言えないので、
		// D3D10 用定数から D3D11 用定数に変更し忘れていてもコンパイル エラーにならないという不満があるが……
		// C++11 対応コンパイラーであれば C# 並みの型安全コードを記述できるが、ライブラリ側のサポートも欠かせない。

		glBindTexture(GL_TEXTURE_2D, m_toonShadingRefTexture.get());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#if 0
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &MyMath::COLOR4F_WHITE.x);
#endif

		const int inoutPixelFormat = GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, inoutPixelFormat,
			texW,
			texH,
			0, inoutPixelFormat, GL_UNSIGNED_BYTE, &m_pToonShadingDiffuseCoefRefTexData->TextureDib[0]);
		const auto lastError = glGetError();

		glBindTexture(GL_TEXTURE_2D, 0);

		// テクスチャの初回割り当て（メモリー確保を伴う）には glTexImage2D() を使うが、
		// 以降のテクスチャの実行時書き換えは glTexSubImage2D() を使う。
		// Direct3D と違い、最適化のための Usage/CPUAccessFlags は存在せず、すべての OpenGL テクスチャは動的書き換え可能らしい。

		return lastError == GL_NO_ERROR;
	}

	void MyOGLManager::UpdateCBuffer(const CBufferBoneMatrixPalettePack* pSrcData) const
	{
		_ASSERTE(m_cbufferBoneMatrixPalette);
		UpdateUniformBuffer(pSrcData, m_cbufferBoneMatrixPalette);
	}

	void MyOGLManager::UpdateCBuffer(const CBufferViewParamsPack* pSrcData) const
	{
		_ASSERTE(m_cbufferViewParams);
		UpdateUniformBuffer(pSrcData, m_cbufferViewParams);
	}

	void MyOGLManager::UpdateCBuffer(const CBufferMeshPartAttributePack* pSrcData) const
	{
		_ASSERTE(m_cbufferMeshPartAttribute);
		UpdateUniformBuffer(pSrcData, m_cbufferMeshPartAttribute);
	}

	void MyOGLManager::UpdateCBuffer(const CBufferLightParamsPack* pSrcData) const
	{
		_ASSERTE(m_cbufferLightParams);
		UpdateUniformBuffer(pSrcData, m_cbufferLightParams);
	}

	void MyOGLManager::InitializeCameraSettings()
	{
		m_myCameraSettings.Initialize();
	}

	static void ApplyTestLightColor(CBufferLightParamsPack& lightParam)
	{
		//const float ambientIntensity = 0.5f;
		//const float ambientIntensity = 0.2f;
		const float ambientIntensity = 32.0f / 255.0f;
		lightParam.AmbientLight = MyMath::Vector4F(ambientIntensity, ambientIntensity, ambientIntensity, 1);
		lightParam.LightColor = MyMath::Vector4F(1, 1, 1, 1);
	}

	void MyOGLManager::UpdateEffectMatrices()
	{
		CBufferViewParamsPack viewParam;
		m_myCameraSettings.GetMatrixWorldViewProj(
			&viewParam.UniWorldMatrix, &viewParam.UniViewMatrix, &viewParam.UniProjectionMatrix,
			&viewParam.UniWorldViewMatrix, &viewParam.UniViewProjMatrix, &viewParam.UniWorldViewProjMatrix);
		viewParam.UniEyePosition = m_myCameraSettings.m_vCameraEye;
		viewParam.UniScreenSize.x = m_myCameraSettings.m_screenWidth;
		viewParam.UniScreenSize.y = m_myCameraSettings.m_screenHeight;
		this->UpdateCBuffer(&viewParam);

		CBufferLightParamsPack lightParam;
		m_myCommonSettings.GetRotatedLightDir(&lightParam.LightDir);
		const float lightRadius = 10000;
		MyMath::MultiplyVector3(lightParam.LightPos, -lightRadius, lightParam.LightDir);

		ApplyTestLightColor(lightParam);
		this->UpdateCBuffer(&lightParam);
	}

	namespace
	{

		void DrawMeshSubset(MyDeviceMeshPack* pMesh, const ProgramResourcePtr& programObj, size_t attribId,
			bool isToonShading, bool drawsToonInk, bool sproutsFur, MyMath::MyReflexType reflexType)
		{
			// 実行する Shader Subroutine のインデックス（関数ポインタ）を選択する。
			// この選択状態はプログラム オブジェクトのステートではなく、OpenGL コンテキストのステートらしいので、
			// glUseProgram() などを呼び出すたびに選択処理が必要らしい。
			// インデックス番号に関しては、layout:index を使って GLSL シェーダー側で指定することもできるらしい。
			// シェーダーあたりのサブルーチンの種類の最大数は、少なくとも 256 が保証されるらしい。
			// Shader Subroutine の場合、glGetUniformLocation() ではロケーションを取得できないので注意。
			// layout:location を使って GLSL シェーダー側で指定することもできるらしい。
			// 毎フレーム名前検索するのもバカらしいので、シェーダーで指定した値に合わせるようにしたほうが良いかも。
			// http://www.opengl.org/wiki/Shader_Subroutine
			static const char* const ppMainLightingShaderInstanceNames[] =
			{
				"SubCalcMainLightingPhotoReal",
				"SubCalcMainLightingToon",
			};
			const auto indexSubCalcMainLighting = glGetSubroutineIndex(programObj.get(), GL_FRAGMENT_SHADER,
				isToonShading ? ppMainLightingShaderInstanceNames[1] : ppMainLightingShaderInstanceNames[0]);
			_ASSERTE(indexSubCalcMainLighting != GL_INVALID_INDEX);
#ifdef _DEBUG
			const auto locSubCalcMainLighting = glGetSubroutineUniformLocation(programObj.get(), GL_FRAGMENT_SHADER, "SubCalcMainLighting");
			_ASSERTE(locSubCalcMainLighting != GL_INVALID_INDEX);
#endif

			static const char* const ppColorPickerInstanceNames[] =
			{
				"SubGetEnvMapColorDummy",
				"SubGetEnvMapColorReflect",
				"SubGetEnvMapColorRefract",
				"SubGetEnvMapColorFresnel",
			};
			_ASSERTE(0 <= reflexType && reflexType < ARRAYSIZE(ppColorPickerInstanceNames));
			const auto indexSubGetEnvMapColor = glGetSubroutineIndex(programObj.get(), GL_FRAGMENT_SHADER, ppColorPickerInstanceNames[reflexType]);
			_ASSERTE(indexSubGetEnvMapColor != GL_INVALID_INDEX);
#ifdef _DEBUG
			const auto locSubGetEnvMapColor = glGetSubroutineUniformLocation(programObj.get(), GL_FRAGMENT_SHADER, "SubGetEnvMapColor");
			_ASSERTE(locSubGetEnvMapColor != GL_INVALID_INDEX);
#endif
			// 複数種のサブルーチンが含まれる場合は、
			// Direct3D の ID3D11DeviceContext::PSSetShader() などのように、
			// ID3D11ClassInstance へのポインタ配列を渡す要領でインデックス配列を渡せばよいらしい。
			_ASSERTE(locSubCalcMainLighting == 0);
			_ASSERTE(locSubGetEnvMapColor == 1);
			const GLuint subroutineIndices[] =
			{
				indexSubCalcMainLighting,
				indexSubGetEnvMapColor,
			};
			glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, ARRAYSIZE(subroutineIndices), subroutineIndices);

			pMesh->DrawSubset(attribId);
		}
	}

	void MyOGLManager::DrawMeshArray(const ProgramResourcePtr& programObj)
	{
		// マテリアルを持たないメッシュ用のデフォルト ディフューズ色。
		//static const MyMath::Vector4F defaultGrayDiffuseColor(0.5f, 0.5f, 0.5f, 1);
		static const MyMath::Vector4F defaultGrayDiffuseColor(1, 1, 1, 0.5f);

		_ASSERTE(m_pModelMeshInfoArray != nullptr);
		_ASSERTE(m_mainMeshArray.size() == m_pModelMeshInfoArray->size());

		for (size_t i = 0; i < m_mainMeshArray.size(); ++i)
		{
			// モーフ ターゲット（トゥイーニング ターゲット）のメッシュは自身の直接の描画をスキップする。
			// HACK: 現状、モーフ ターゲットとなる条件はメッシュ名のプレフィックスで判断しているが、このプレフィックスを固定名でなく GUI で指定できるようにする。
			// FBX シェイプ アニメーションの機能をインポートできるようにする？

			const auto* pModelMeshInfo = (*m_pModelMeshInfoArray)[i].get();
			//if (strncmp(pModelMeshInfo->GetMeshName(), MyCommon::MorphTargetMeshPrefixName, MyCommon::MorphTargetMeshPrefixNameLen) == 0)
			if (MyCommon::CheckHasMeshMorphTargetPrefixName(pModelMeshInfo->GetMeshNameW().c_str()))
			{
				continue;
			}
#if 1
			// いったん配列のすべての要素を単位行列化する。
			// デュアル クォータニオン配列も単位化する。
			this->IdentifyAllSkinningBonePalette();
#endif

			const auto& pAnimMixerArray = (*m_pAnimMixerArrayArray)[i];
			_ASSERTE(pAnimMixerArray);
			pModelMeshInfo->GetCurrentFrameBoneQuatsArray(
				m_boneQuatPalette.BoneQuats, *pAnimMixerArray);
			m_boneQuatPalette.ConvertToMatrices(m_boneMatrixPalette.BoneMatrices);

			// GLSL 用には転置しない。

			const auto& currentLocations = m_locationsSkinningPhong;
			const auto& currentUniforms = currentLocations.CommonUniforms;

			const int mainTexSlotIndex = 0;
			ActivateTextureSlot(mainTexSlotIndex);
			currentUniforms.UpdateUniMainTextureSlot(mainTexSlotIndex);

			// シェーダー側にボーン行列パレットを転送。
			this->UpdateCBuffer(&m_boneMatrixPalette);

			auto* pMesh = m_mainMeshArray[i].get();
			const size_t attrSize = pMesh->GetAttributeRangeArray().size();
			glBindVertexArray(m_inputLayoutPNTIW.get());
			glBindVertexBuffer(0, pMesh->GetVertexBuffer(), 0, VertexStreamInputLayoutHelperForPNTIW::VertexStrideInBytes);
			CBufferMeshPartAttributePack shaderMeshParam;
			const bool isToon = m_myEffectSettings.EnablesToonShading;
			if (attrSize == 0)
			{
				shaderMeshParam.MaterialColorDiffuse = defaultGrayDiffuseColor;
				shaderMeshParam.MaterialOpacityAlpha = 1;
				shaderMeshParam.MaterialSpecularPower = MyMath::MinSpecularPowerValue;
				shaderMeshParam.MaterialIndexOfRefraction = 1;
				const int toonMaterialIndex = 0;
				shaderMeshParam.ToonShadingRefTexV =
					MyTextureHelper::CalcBilinearToonShadingGradientRefTexCoordV(toonMaterialIndex);
				this->UpdateCBuffer(&shaderMeshParam);
				this->FindTextureAndBind(mainTexSlotIndex, nullptr);
				DrawMeshSubset(pMesh, programObj, 0, isToon, isToon, false, MyMath::MyReflexType_None);
			}
			else
			{
				for (size_t atr = 0; atr < attrSize; ++atr)
				{
					bool sproutsFur = false;
					auto reflexType = MyMath::MyReflexType_None;
					if (!pModelMeshInfo->GetMaterialsArray().empty())
					{
						const int matIndex = (atr < pModelMeshInfo->GetMaterialIndicesArrayForAttribTable().size()) ?
							pModelMeshInfo->GetMaterialIndicesArrayForAttribTable()[atr] : 0;
						_ASSERTE(0 <= matIndex && static_cast<size_t>(matIndex) < pModelMeshInfo->GetMaterialsArray().size());
						const auto& mat = *pModelMeshInfo->GetMaterialsArray()[matIndex].get();
						shaderMeshParam.MaterialColorDiffuse = mat.GetDiffuse();
						shaderMeshParam.MaterialColorAmbient = mat.GetAmbient();
						shaderMeshParam.MaterialColorSpecular = mat.GetSpecular();
						shaderMeshParam.MaterialColorEmissive = mat.GetEmissive();
						shaderMeshParam.MaterialOpacityAlpha = mat.GetOpacityAlpha();
						shaderMeshParam.MaterialSpecularPower = mat.GetSpecularPower();
						shaderMeshParam.UniMaterialRoughness = mat.GetRoughness();
						shaderMeshParam.MaterialReflectivity = mat.GetReflectivity();
						shaderMeshParam.MaterialIndexOfRefraction = mat.GetIndexOfRefraction();
						const int toonMaterialIndex = 1;
						shaderMeshParam.ToonShadingRefTexV =
							MyTextureHelper::CalcBilinearToonShadingGradientRefTexCoordV(toonMaterialIndex);
						this->FindTextureAndBind(mainTexSlotIndex, &mat.GetTexFileNameDiffuseMap());
						sproutsFur = mat.GetSproutsFur();
						if (shaderMeshParam.MaterialReflectivity != 0)
						{
							reflexType = mat.GetReflexType();
						}
					}
					else
					{
						shaderMeshParam.MaterialColorDiffuse = defaultGrayDiffuseColor;
						shaderMeshParam.MaterialColorAmbient = MyMath::ZERO_VECTOR4F;
						shaderMeshParam.MaterialColorSpecular = MyMath::ZERO_VECTOR4F;
						shaderMeshParam.MaterialColorEmissive = MyMath::ZERO_VECTOR4F;
						shaderMeshParam.MaterialOpacityAlpha = 1;
						shaderMeshParam.MaterialSpecularPower = MyMath::MinSpecularPowerValue;
						shaderMeshParam.UniMaterialRoughness = 0;
						shaderMeshParam.MaterialReflectivity = 0;
						shaderMeshParam.MaterialIndexOfRefraction = 1;
						const int toonMaterialIndex = 1;
						shaderMeshParam.ToonShadingRefTexV =
							MyTextureHelper::CalcBilinearToonShadingGradientRefTexCoordV(toonMaterialIndex);
						this->FindTextureAndBind(mainTexSlotIndex, nullptr);
					}
					this->UpdateCBuffer(&shaderMeshParam);
					DrawMeshSubset(pMesh, programObj, atr, isToon, isToon, sproutsFur, reflexType);
				}
			}
			glBindVertexArray(0);
		}
	}


	static void SetupSingleViewport(float width, float height)
	{
		// OpenGL のビューポートは left, top ではなく left, bottom なので、
		// テクスチャをスクリーン全体に張り付けるときなどに注意が必要。

		// ID3D10Device/ID3D11DeviceContext::RSSetViewports() に相当するのは
		// OpenGL 4.1 の glViewportArrayv() らしい。
		// しかし相変わらず分かりにくいインターフェイス。いいかげん構造体導入すればいいのに……
		// GL_ARB_viewport_array 自体は OpenGL 3.2 で拡張機能として導入されていたらしい。
		// ちなみにジオメトリ シェーダーが正式に標準化されたのも OpenGL 3.2。
		const float viewports[] =
		{ 0, 0, width, height };
		glViewportArrayv(0, 1, viewports);
	}

	static void SetupSingleViewport(uint32_t width, uint32_t height)
	{
#if 0
		// OpenGL 1.x から存在する従来バージョン。ビューポートを1つしか指定できない。
		glViewport(0, 0, width, height);
#else
		SetupSingleViewport(float(width), float(height));
#endif
	}


	bool MyOGLManager::Render(bool advancesFrame)
	{
		if (!m_isInitialized)
		{
			// 初期化が済んでいないのに描画することはできない。
			return false;
		}

		MyUtils::HRStopwatch stopwatch;
		stopwatch.Start();

		_ASSERTE(m_hDC != nullptr);
		_ASSERTE(m_hGLRC != nullptr);

#pragma region // コンピュート シェーダーで乱数テーブルを更新する。//
		{
			glUseProgram(m_programObjUpdateRandomTable.get());

			// SSBO のバインド。
			// glBindBufferRange() の使い方がよく分からない……
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssboRandomNumTableBuffer.get());
#if 0
			glDispatchCompute(ComputeDispatchCountX, 1, 1);
#else
			// OpenGL コンピュート シェーダーのローカル グループ サイズは、
			// DirectX 11 / HLSL のように GLSL シェーダー側でハード コーディングする方法のほか、
			// ホスト プログラム側でも指定することができる拡張 GL_ARB_compute_variable_group_size が用意されている。
			// ただしホスト側で指定する場合、シェーダーコンパイル時には layout に local_size_variable の指定が必要な模様。
			glDispatchComputeGroupSizeARB(ComputeDispatchCountX, 1, 1,
				ComputingThreadLocalGroupSizeX, ComputingThreadLocalGroupSizeY, 1);
#endif

			glUseProgram(0);
		}
#pragma endregion

		//glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
		glClearColor(
			m_myCommonSettings.BackColor.x,
			m_myCommonSettings.BackColor.y,
			m_myCommonSettings.BackColor.z,
			m_myCommonSettings.BackColor.w);

		SetupSingleViewport(m_myCameraSettings.GetScreenWidthF(), m_myCameraSettings.GetScreenHeightF());

		this->UpdateEffectMatrices();

		// 深度バッファへの書き込みを有効にする。描画だけでなく、Clear する前に ON にしておく必要があるので注意。
		glDepthMask(true);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		// 不透明オブジェクトを描画する際はアルファ ブレンドを切っておいたほうが高速になる。
		glDisable(GL_BLEND);
#ifdef ENABLES_MY_GL_MSAA
		glEnable(GL_MULTISAMPLE);
#endif

#if 0
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0, m_myCameraSettings.GetAspectRatio(), 0.1, 1000.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(
			m_myCameraSettings.m_vCameraEye.x, m_myCameraSettings.m_vCameraEye.y, m_myCameraSettings.m_vCameraEye.z,
			m_myCameraSettings.m_vCameraAt.x, m_myCameraSettings.m_vCameraAt.y, m_myCameraSettings.m_vCameraAt.z,
			m_myCameraSettings.m_vCameraUp.x, m_myCameraSettings.m_vCameraUp.y, m_myCameraSettings.m_vCameraUp.z);
#endif

#if 0
		MyMath::MatrixF mWorld;
		MyMath::MatrixF mView;
		MyMath::MatrixF mProj;
		MyMath::MatrixF mWdVw;
		MyMath::MatrixF mVwPj;
		MyMath::MatrixF mWdVwPj; // シェーダーで何度も使用される行列 [World x View x Proj] はあらかじめ CPU で乗算しておく。
		m_myCameraSettings.GetMatrixWorldViewProj(&mWorld, &mView, &mProj, &mWdVw, &mVwPj, &mWdVwPj);
#endif

		// OpenGL 2.x では、必ず頂点バッファのバインド後に頂点属性（頂点レイアウト）の設定を行なう必要がある。
		// すなわち、glVertexAttribPointer(), glVertexAttribIPointer() などを呼ぶ前に glBindBuffer() を呼び出す必要がある。
		// 同じ頂点レイアウト構造を使用する頂点バッファであっても、この呼び出し順序を守って毎回設定する必要がある。
		// これは Direct3D と異なる。
		// Direct3D では頂点レイアウト オブジェクトは（作成時に頂点シェーダー BLOB が必要になるものの）
		// 完全に頂点バッファ オブジェクトおよびシェーダープログラム オブジェクトとは分離されていて、
		// 同じ入力レイアウト フォーマットを使用するシェーダー間で使いまわせるが、
		// OpenGL は各頂点属性の Location が各シェーダープログラム オブジェクト固有であることに起因するらしい。
		// ちなみに glDrawElements() は glBindBuffer(GL_ARRAY_BUFFER) を直接見ていないらしく、描画に影響を与えないらしい。
		// glDrawElements() が実際に見ているのは glVertexAttribPointer() のほうらしい（頂点属性と頂点バッファがバインドされている）。
		// Direct3D の Input Layout や Vertex Declaration に比較的近いのは、OpenGL 3.x 以降の Vertex Array Object と、
		// OpenGL 4.3 以降の Vertex Attribute Format と Vertex Attribute Binding の組み合わせ。
		// VAO は頂点 Attribute の設定情報を保存し、描画時に利用することができる。ステート設定の記録＆再生のような仕組みを備えているらしい。
		// なお、ほかにも OpenGL は 3.1 で Uniform Buffer Object & Uniform Block を、3.3 で Sampler Object を、
		// 4.1 で Program Pipeline Object を導入するなど、
		// 徐々に Direct3D に近いオブジェクト機構を追加している模様。

		glBindFramebuffer(GL_FRAMEBUFFER, m_subFrameBuffer.get());
		{
			SetupSingleViewport(m_myCameraSettings.GetScreenWidthF(), m_myCameraSettings.GetScreenHeightF());

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// 座標軸の描画。
			if (m_myCommonSettings.DisplaysCoordAxes)
			{
				glLineWidth(1.0f);

				glUseProgram(m_programObjShadingLess.get());

#if 0
				// VBO のみを使ったバージョン。OpenGL 2.x および ES 2.0 用。
				glBindBuffer(GL_ARRAY_BUFFER, m_coordAxisLineVertexBufferPC.get());
				m_locationsShadingLess.EnableVertexStreamLayout();

				glDrawArrays(GL_LINES, 0, 6);

				m_locationsShadingLess.DisableVertexStreamLayout();
				glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
				// VAO と VBO を使ったバージョン。OpenGL 4.3 以降および ES 3.0 以降用。
				// こちらのほうが Direct3D に近く、移植しやすい。
				glBindVertexArray(m_inputLayoutPC.get());
				glBindVertexBuffer(0, m_coordAxisLineVertexBufferPC.get(), 0, VertexStreamInputLayoutHelperForPC::VertexStrideInBytes);
				glDrawArrays(GL_LINES, 0, 6);
				//glBindVertexBuffer(0, 0, 0, 0); // 特に必要ではない。
				glBindVertexArray(0);
#endif

				glUseProgram(0);
			}

			if (!m_mainMeshArray.empty())
			{
				const auto& currentLocations = m_locationsSkinningPhong;
				const auto& currentUniforms = currentLocations.CommonUniforms;

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glUseProgram(m_programObjSkinningPhong.get());

				// マルチテクスチャ。OpenGL は Direct3D に比べてめんどくさい……
				// OpenGL 4.5 では Direct State Access (DSA) によって多少改善されるはず。
				// uniform の設定は glUseProgram() の後で行なう。
				// UBO は毎回バインドする必要はない（たぶん SSBO も）。

				const int toonGradTexSlotIndex = 1;
				const int envMapTexSlotIndex = 2;
				{
					const GLuint currCubeTex = m_envCubeTexture ? m_envCubeTexture.get() : m_dummyCubeTexture.get();
					// スロット 1 にトゥーン グラデーション テクスチャを設定する。
					ActivateTextureSlot(toonGradTexSlotIndex);
					glBindTexture(GL_TEXTURE_2D, m_toonShadingRefTexture.get());
					currentUniforms.UpdateUniToonShadingRefTextureSlot(toonGradTexSlotIndex);
					glBindSampler(toonGradTexSlotIndex, m_samplerLinearClamp.get());
					// スロット 2 に環境マッピング用のキューブ テクスチャを設定する。
					ActivateTextureSlot(envMapTexSlotIndex);
					glBindTexture(GL_TEXTURE_CUBE_MAP, currCubeTex);
					currentUniforms.UpdateUniEnvMapTextureSlot(envMapTexSlotIndex);
					glBindSampler(envMapTexSlotIndex, m_samplerLinearClamp.get());
					ActivateTextureSlot(0);
				}

				this->DrawMeshArray(m_programObjSkinningPhong);

				{
					// 描画が終了したらテクスチャのスロットをリセットしておく。
					ActivateTextureSlot(toonGradTexSlotIndex);
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindSampler(toonGradTexSlotIndex, 0);
					ActivateTextureSlot(envMapTexSlotIndex);
					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
					glBindSampler(envMapTexSlotIndex, 0);
					ActivateTextureSlot(0);
				}

				glUseProgram(0);

				glDisable(GL_BLEND);
			}

			if (m_myEffectSettings.DisplaysWaveFront)
			{
				// コンピュート シェーダーの波面シミュレーション テスト。
				glUseProgram(m_programObjSimpleComputingTest.get());

#if 0
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssboWaveSimWorkBuffers[0].get());
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ssboWaveSimWorkBuffers[1].get());
#endif
				m_locationsSimpleComputingTest.UpdateUniWaveSimInput0ImageSlot(0);
				m_locationsSimpleComputingTest.UpdateUniWaveSimOutput0ImageSlot(1);

				const GLuint bindingInput = m_waveSimInOutFlipFlag ? 1 : 0;
				const GLuint bindingOutput = m_waveSimInOutFlipFlag ? 0 : 1;
				glBindImageTexture(bindingInput, m_waveSimWorkTextures[0].get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
				glBindImageTexture(bindingOutput, m_waveSimWorkTextures[1].get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

				glDispatchComputeGroupSizeARB(ComputeDispatchCountX, 1, 1,
					ComputingThreadLocalGroupSizeX, ComputingThreadLocalGroupSizeY, 1);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				// シミュレーション結果の可視化。
				glUseProgram(m_programObjDisplayWaveSim.get());

				{
					CBufferLightParamsPack lightParam;
					ApplyTestLightColor(lightParam);
					lightParam.LightPos = MyMath::Vector3F(100, 300, 100);
					this->UpdateCBuffer(&lightParam);

					CBufferMeshPartAttributePack shaderMeshParam;
					shaderMeshParam.MaterialColorDiffuse = MyMath::Vector4F(0.6f, 0.8f, 1.0f, 1.0f);
					shaderMeshParam.MaterialColorAmbient = MyMath::Vector4F(0.8f, 0.8f, 0.8f, 1.0f);
					shaderMeshParam.MaterialColorSpecular = MyMath::Vector4F(1.6f, 1.8f, 2.0f, 1.0f);
					shaderMeshParam.MaterialOpacityAlpha = 0.7f;
					shaderMeshParam.MaterialSpecularPower = 4.0f;
					this->UpdateCBuffer(&shaderMeshParam);
				}

				const int mainTexSlotIndex = 0;
				m_locationsDisplayWaveSim.CommonUniforms.UpdateUniWaveSimResultTextureSlot(mainTexSlotIndex);
				glBindSampler(mainTexSlotIndex, m_samplerLinearClamp.get());
				glBindTexture(GL_TEXTURE_2D, m_waveSimInOutFlipFlag ? m_waveSimWorkTextures[1].get() : m_waveSimWorkTextures[0].get());

				glBindVertexArray(m_inputLayoutPCNT.get());
				glBindVertexBuffer(0, m_waveFrontPlaneVertexBufferPCNT.get(), 0, VertexStreamInputLayoutHelperForPCNT::VertexStrideInBytes);
				this->DrawOneQuad();
				glBindVertexArray(0);

				glBindSampler(mainTexSlotIndex, 0);
				glBindTexture(GL_TEXTURE_2D, 0);

				glUseProgram(0);

				glDisable(GL_BLEND);

				// OpenGL における memcpy() 相当機能がよく分からない。
				// つまり、ID3D11DeviceContext::CopyResource(), ID3D11DeviceContext::CopySubresourceRegion() に相当する機能。
				// テクスチャのディープコピーは OpenGL 1.1 の glCopyTexSubImage2D() を使うのか？
				// それとも OpenGL 4.2 の glCopyImageSubData() なのか？（GL_ARB_copy_image）
				// VBO などのバッファのディープコピーは OpenGL 3.0 の glCopyBufferSubData(),
				// GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER とかを使うのか？（GL_ARB_copy_buffer）
				// FBO 経由のコピーに関しては glBlitFramebuffer() なのか？
				// http://qiita.com/cabtbl@github/items/b2bbaecc50a0e111ad08
				// Direct3D 10/11 の場合はバッファやテクスチャの上位概念として「リソース」というものが定義されていて、
				// リソースのインターフェイス経由で統一的に扱える。フォーマットが同じであればメソッド一発で直接コピーも可能。
				// なお、出力に使っていたものを入力に使用するだけであれば、コピーは不要でフラグメント シェーダーでもそのまま参照すればいいだけ。
				// Direct3D ピクセル シェーダー 5.0 では RWTexture2D と UAV 経由での読み出しにフォーマット制約があるので、
				// 二つの UAV をバインドしておくのではなく、入出力（SRV, UAV）を入れ替える方法を採る必要がある。
				// 入出力を入れ替えてフレームを進めるのであれば、最初にバインドするときにフリップすることになる。
				if (advancesFrame)
				{
					m_waveSimInOutFlipFlag = !m_waveSimInOutFlipFlag;
				}
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// フレーム バッファ（テクスチャ）に描画した内容を、バック バッファに転送する。
		{
			glUseProgram(m_programObjMSAATransport.get());

			const int mainTexSlotIndex = 0;
#ifdef ENABLES_MY_GL_MSAA
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_subColorBufferTexture.get());
			glBindSampler(mainTexSlotIndex, 0);
#else
			glBindTexture(GL_TEXTURE_2D, m_subColorBufferTexture.get());
#endif

			// テクスチャ ユニット 0 を指定する（スロット 0 にテクスチャを設定する）。
			m_locationsMSAATransport.CommonUniforms.UpdateUniMainTextureSlot(mainTexSlotIndex);

			glBindVertexArray(m_inputLayoutPCT.get());
			glBindVertexBuffer(0, m_oneSquareVertexBufferPCT.get(), 0, VertexStreamInputLayoutHelperForPCT::VertexStrideInBytes);
			this->DrawOneQuad();
			glBindVertexArray(0);

#ifdef ENABLES_MY_GL_MSAA
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#else
			glBindTexture(GL_TEXTURE_2D, 0);
#endif

			glUseProgram(0);
		}

#ifdef ENABLES_MY_GL_MSAA
		glDisable(GL_MULTISAMPLE);
#endif
#pragma region // HUD 文字列の描画。最前面になるように、最後に実施する。//
		{
			static wchar_t messageString[1024];
			if (advancesFrame)
			{
				swprintf_s(messageString, L"OGL: PeriodicMode: %7.1f FPS", m_fpsCounter.GetFpsValue());
			}
			else
			{
				wcscpy_s(messageString, L"OGL: EventDrivenMode");
			}

			// 深度テストを切って（無視して）最前面に描画する。深度バッファへの書き込みも禁止しておく。
			glDisable(GL_DEPTH_TEST);
			glDepthMask(false);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			const MyMath::Vector2F msgStartPos(4, 4);
			const MyMath::Vector4F upperColor(1.0f, 0.7f, 0.7f, 1);
			const MyMath::Vector4F lowerColor(0.7f, 0.7f, 1.0f, 1);
			ATLVERIFY(m_fontRects.UpdateVertexBufferByString(
				messageString,
				msgStartPos, upperColor, lowerColor,
				m_pHudFontTexData->FontHeight,
				m_pHudFontTexData->UsesMonospaceFont,
				m_pHudFontTexData->TextureData.TextureWidth,
				m_pHudFontTexData->TextureData.TextureHeight,
				m_pHudFontTexData->CodeUVMap));

			glUseProgram(m_programObjFontSprite.get());

			const int mainTexSlotIndex = 0;
			// テクスチャ ユニット 0 を指定する（スロット 0 にテクスチャを設定する）。
			ActivateTextureSlot(mainTexSlotIndex);
			m_locationsFontSprite.CommonUniforms.UpdateUniMainTextureSlot(mainTexSlotIndex);
			glBindSampler(mainTexSlotIndex, m_samplerLinearClamp.get());
			glBindTexture(GL_TEXTURE_2D, m_hudFontAlphaTexture.get());

			CBufferViewParamsPack viewParam;
			MyMath::CreateMatrixOrtho2DRH(&viewParam.UniWorldViewProjMatrix,
				0,
				m_myCameraSettings.GetScreenWidthF(),
				m_myCameraSettings.GetScreenHeightF(),
				0);
			this->UpdateCBuffer(&viewParam);

			glBindVertexArray(m_inputLayoutPCT.get());
			glBindVertexBuffer(0, m_fontRects.GetVertexBuffer(), 0, VertexStreamInputLayoutHelperForPCT::VertexStrideInBytes);
			m_fontRects.DrawString();
#if 0
			// デスクトップ GPU では書き換えたバッファをすぐに使用しても普通に描画できる。
			// デスクトップの場合はイミディエイト モードで動作できるからか？
			ATLVERIFY(m_fontRects.UpdateVertexBufferByString(
				L"hogehoge",
				MyMath::Vector2F(4, 30), MyMath::Vector4F(1, 1, 0, 1), MyMath::Vector4F(1, 1, 0, 1),
				m_pHudFontTexData->FontHeight,
				m_pHudFontTexData->UsesMonospaceFont,
				m_pHudFontTexData->TextureData.TextureWidth,
				m_pHudFontTexData->TextureData.TextureHeight,
				m_pHudFontTexData->CodeUVMap));
			m_fontRects.DrawString();
#endif
			glBindVertexArray(0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindSampler(mainTexSlotIndex, 0);

			glUseProgram(0);
		}
#pragma endregion

		// バック バッファからフロント バッファへ転送。D3D の Present() 相当……ではないらしい。
		// GDI の SwapBuffers() は D3D の Present() とは若干違い、直ちに制御を返すらしい。
		// https://sites.google.com/site/toriaezuzakki/opengl
		::SwapBuffers(m_hDC);

		glFinish();

		stopwatch.Stop();
		if (advancesFrame)
		{
			m_fpsCounter.AdvanceFrameCounter();
			m_fpsCounter.AddElapsedTimeSec(stopwatch.GetElapsedTimeInSeconds());
			const uint32_t FpsCounterLimit = 20; // N フレーム分の経過時間を使って FPS を計算する。
			if (m_fpsCounter.GetFrameCounter() >= FpsCounterLimit)
			{
				//ATLTRACE("Frame=%u, Time=%.12f, FPS=%.1f\n", m_fpsCounter.GetFrameCounter(), m_fpsCounter.GetElapsedTimeSec(), m_fpsCounter.GetFpsValue());
				m_fpsCounter.UpdateFpsValue();
				m_fpsCounter.Reset();
			}
		}
		return true;
	}

	bool MyOGLManager::ResizeScreen(UINT width, UINT height)
	{
		// スクリーン サイズに変更があった場合、FBO や深度バッファの再作成などが必要。
		if (width == m_myCameraSettings.m_screenWidth && height == m_myCameraSettings.m_screenHeight)
		{
			return false;
		}

		{
			// HDR ポスト エフェクトなどを実現する目的で、
			// 浮動小数バッファにレンダリングするためのフレーム バッファ オブジェクト（FBO）を作成する。
			// Direct3D でいうレンダーターゲット テクスチャに相当。というかデバイス コンテキストの RTV のスロットのほうに近いのかも。
			// マルチレンダーターゲットの場合、複数のテクスチャを単一の FBO に関連付けることになる。
			// なお、レンダーターゲットを切り替える場合、レンダー先テクスチャが全て同じフォーマットであれば、FBO を複数用意する必要はないらしい。
			// 使う FBO は一つにして、それに関連付けるテクスチャを glFramebufferTexture2D() で切り替えることでレンダーターゲットの切り替えを実現すればよいらしい。
			// ちなみに OpenGL には古くから PBO という機能もあるが、新しいハードウェアでは FBO を使ったほうがよいらしい。
			// また、Direct3D とは違い、テクスチャ側にはレンダーターゲットにするための専用フラグを特に立てたりする必要はないらしい。
			// http://cheeerioooo.blogspot.jp/2010/10/blog-post.html
			// http://www.cprogramdevelop.com/1159590/
			// http://cheeerioooo.blogspot.jp/2010/10/fbopbo.html
			// http://hacksoflife.blogspot.jp/2006/10/vbos-pbos-and-fbos.html

			m_subFrameBuffer = Factory::CreateOneFrameBufferPtr();
			_ASSERTE(m_subFrameBuffer.get() != 0);

			// 16bit HDR テクスチャ（FP16）を作成して、レンダーターゲットとして使用する。
			// ただし浮動小数バッファが正式にサポートされているのは OpenGL 3.0 以降。
			// 「Floating-point color and depth internal formats for textures and renderbuffers」
			// また、MSAA テクスチャが正式に使えるのは OpenGL 3.2 以降らしい。
			// ちなみに Direct3D では 9.0 の時代からすでに FP16 などの HDR レンダリング フォーマットや MSAA を正式サポートしていた。
			// 正式に FP16 で MSAA を同時に実行できるのは Direct3D 10 以降。
			// MSAA テクスチャの作成には下記関数を使う。
			// glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, internalFormat, width, height, fixedSampleLocations);
			// FBO のマルチサンプル能力（サンプル カウント数など）を取得する場合、
			// バック バッファとは違って、wglChoosePixelFormatARB() を使って逐一調べていく必要はない模様。
			// OpenGL の場合、GL_MAX_SAMPLES で取得した値以下であれば、フォーマットを問わず必ず MSAA テクスチャを作成できる模様。
			// NVIDIA Quadro 2000 と 311.15 ドライバーでテストした結果は 64 だった。
			// なお、Direct3D 11 の D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT は 32 となっている。
			// ちなみに iOS や Mac OS X には glResolveMultisampleFramebufferAPPLE() という、
			// ID3D10Device::ResolveSubresource() / ID3D11DeviceContext::ResolveSubresource() に似た拡張機能があるらしい？

#ifdef ENABLES_MY_GL_MSAA
			const GLenum renderTargetTexStyle = GL_TEXTURE_2D_MULTISAMPLE;
			const int MY_MSAA_ALG_TEX_SAMPLE_COUNT = 8;
			const int msaaSampleCount = MY_MSAA_ALG_TEX_SAMPLE_COUNT;
#else
			const GLenum renderTargetTexStyle = GL_TEXTURE_2D;
#endif
			GLenum lastError = GL_NO_ERROR;

			// FP16 カラーバッファのセットアップ。
			m_subColorBufferTexture = Factory::CreateOneTexturePtr();
			_ASSERTE(m_subColorBufferTexture.get() != 0);
			glBindTexture(renderTargetTexStyle, m_subColorBufferTexture.get());
#ifdef ENABLES_MY_GL_MSAA
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSampleCount, GL_RGBA16F, width, height, GL_TRUE);
#else
			// Direct3D テクスチャ サンプラーの変更に相当。
			// レガシーな OpenGL ではオブジェクトが独立しておらず、テクスチャごとに直接設定されるらしい。
			// なお、必ずなんらかのフィルターを明示的に設定しておかないとサンプリングが失敗するので注意。
			// 設定するタイミングは作成時に1回もしくはフレーム描画時に毎回でもよい。
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
			//glTexParameteri(renderTargetTexStyle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameteri(renderTargetTexStyle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(renderTargetTexStyle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(renderTargetTexStyle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
			lastError = glGetError();
			_ASSERTE(lastError == GL_NO_ERROR);
			glTexParameteri(renderTargetTexStyle, GL_TEXTURE_MAX_LEVEL, 0);
			lastError = glGetError();
			_ASSERTE(lastError == GL_NO_ERROR);
			// MSAA テクスチャにサンプラーは設定できない。設定しようとすると GL_INVALID_ENUM が返ってきてしまう。
			glBindTexture(renderTargetTexStyle, 0);

			// D24S8 深度ステンシル バッファもしくは D32 深度バッファのセットアップ。
			// ステンシルが不要であれば、深度に 32bit を使う FP32 でもよい。
			// 特に奥行きが広いシーン空間やシャドウ マップを扱う場合、24bit では精度が足らなくなってしまう。
			// AMD GCN 上の OpenGL の場合、D24S8 でも内部的には深度バッファに 32bit 確保するらしいが……
			// http://diary.kumaryu.net/?date=20130723
			// カラーバッファが MSAA の場合、対応する深度ステンシルも同一サンプル カウントの MSAA にする必要がある。
			m_subDepthBufferTexture = Factory::CreateOneTexturePtr();
			_ASSERTE(m_subDepthBufferTexture.get() != 0);
			glBindTexture(renderTargetTexStyle, m_subDepthBufferTexture.get());
#ifdef ENABLES_MY_GL_MSAA
			// 最後の引数 fixedSampleLocations はよくわからない。
			// Direct3D 10.1 の GetSamplePosition() と何か関係があるのかも。
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSampleCount, GL_DEPTH_COMPONENT32F, width, height, GL_TRUE);
			//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSampleCount, GL_DEPTH24_STENCIL8, width, height, GL_TRUE); // D24S8 を使う場合。
#else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr); // D24S8 を使う場合。
#endif
			glTexParameteri(renderTargetTexStyle, GL_TEXTURE_MAX_LEVEL, 0);
			lastError = glGetError();
			_ASSERTE(lastError == GL_NO_ERROR);
			glBindTexture(renderTargetTexStyle, 0);

			// テクスチャをフレーム バッファ オブジェクトにバインド。
			glBindFramebuffer(GL_FRAMEBUFFER, m_subFrameBuffer.get());
			// レンダーターゲット0番。
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTargetTexStyle, m_subColorBufferTexture.get(), 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, renderTargetTexStyle, m_subDepthBufferTexture.get(), 0);

			// デフォルトのレンダーターゲット（通常のバック バッファ）に戻す。
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// あとは、glBindFramebuffer() を使ってレンダリング先を制御するだけ。
			// いったん HDR テクスチャに描画して、それを張り付けた矩形ポリゴンを通常のバック バッファに描画する。
		}

		m_myCameraSettings.m_screenWidth = width;
		m_myCameraSettings.m_screenHeight = height;

		return true;
	}
} // end of namespace
