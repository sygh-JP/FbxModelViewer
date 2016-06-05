#pragma once


// OpenGL のオブジェクト ハンドルは基本的に整数の ID 値になる。
// Gen/Delete で生成と破棄を管理する GLuint リソース系は非ゼロが有効値。
// Get～Location などのインデックスを取得するだけの GLint 系は取得に失敗すると -1 などの無効値が返る。
// テクスチャに関しては、ID 0 は無効ではなく「無名テクスチャ」という予約された組み込みの有効値になっており、
// 実際に DIB データをバインドしたり普通のテクスチャとして利用することもできるが、
// 通例は glBindTexture() で設定値をリセットする用途に使われることが多い。
// テクスチャ以外の各種バッファや GLSL プログラム オブジェクトなどもおそらく同様。
// ハンドル値は移植性を高めるためか、ポインタは使われない（OpenCL はポインタを使っているが……）。
// なお、OpenGL の仕様書では、ハンドルのことを "Name" と呼んでいるが、非常に分かりにくいので注意。
// Name といえば普通は整数スカラーじゃなくて文字列を連想するだろうが常識的に考えて……
// 日本語の OpenGL 解説サイトでも「名前」と呼んでいるものが多い。
// なぜこんな救いようのない変態仕様になっているのか、いまだに理解できないし理解したくない……

// リソース管理を楽にするため、また解放時に使用する関数を間違えないようにするため、
// std::unique_ptr を使用した RAII クラスとファクトリ メソッドを作る。
// ダック タイピングに必要な共通シノニム名 pointer を導入することで、ポインタ以外の任意型をポインタとして扱える。
// これは std::shared_ptr にはない機能。
// std::unique_ptr は DirectXTex のソースコードなどでもリソース管理などに活用されている。
// ちなみにカスタム デリーターに使用する関数オブジェクト クラスは final 指定できないので注意。

// OpenGL のリソース生成 API はグローバルにいつでも呼び出せるように見えるが、
// 実際はスレッド セーフではなくコンテキスト スレッドから呼び出す必要がある。
// また、ほとんどの OpenGL 関数はレンダリング コンテキスト作成＋アクティブ化の前に呼び出すことが不可能であるにも関わらず、
// なぜすべての API の第1引数にコンテキスト ハンドルを受け取るようなオブジェクト指向的設計にしなかったのか、
// いまだに理解に苦しむ。場当たり的設計というか、お粗末にもほどがある。
// OpenGL API はごく最近追加された一部を除いてほとんどすべてが直交性の低いステートマシン前提設計になっていて、
// 忌々しいことこのうえない。
// 一方、Direct3D 11 のデバイス（主にリソース生成を担当）のメソッドはスレッド セーフ。
// デバイス インターフェイスと分離されたデバイス コンテキストは各スレッドごとに固有なのは OpenGL と同様だが、
// サブスレッドのディファード コンテキストでレンダリング コマンドを記録して
// メインスレッドのイミディエイト コンテキストで再生するという、比較的洗練されたマルチスレッド レンダリング機能を持つ。
// Direct3D 12 ではマルチスレッド レンダリングのパフォーマンスがさらに強化されるらしい。


namespace MyOGL
{
	// GL_INVALID_INDEX というシンボルが定義されているらしい。
	const GLint InvalidShaderParamLocationVal = -1;

#pragma region // Deleters //

	struct ShaderResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteShader(handle);
				// unique_ptr のデストラクタで呼ばれるだけなので無効値代入はしないでよい。
			}
		}
	};


	struct ProgramResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteProgram(handle);
			}
		}
	};


	struct BufferResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteBuffers(1, &handle);
			}
		}
	};


	struct TextureResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteTextures(1, &handle);
			}
		}
	};


	struct FrameBufferResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteFramebuffers(1, &handle);
			}
		}
	};


	struct VertexArrayResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteVertexArrays(1, &handle);
			}
		}
	};

	struct SamplerResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteSamplers(1, &handle);
			}
		}
	};

	struct ProgramPipelineResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteProgramPipelines(1, &handle);
			}
		}
	};

	struct GpuQueryResourceDeleter
	{
		typedef GLuint pointer;
		void operator ()(GLuint handle)
		{
			if (handle != 0)
			{
				glDeleteQueries(1, &handle);
			}
		}
	};

#pragma endregion

	typedef std::unique_ptr<GLuint, ShaderResourceDeleter> ShaderResourcePtr;
	typedef std::unique_ptr<GLuint, ProgramResourceDeleter> ProgramResourcePtr;
	typedef std::unique_ptr<GLuint, BufferResourceDeleter> BufferResourcePtr;
	typedef std::unique_ptr<GLuint, TextureResourceDeleter> TextureResourcePtr;
	typedef std::unique_ptr<GLuint, FrameBufferResourceDeleter> FrameBufferResourcePtr;
	typedef std::unique_ptr<GLuint, VertexArrayResourceDeleter> VertexArrayResourcePtr;
	typedef std::unique_ptr<GLuint, SamplerResourceDeleter> SamplerResourcePtr;
	typedef std::unique_ptr<GLuint, ProgramPipelineResourceDeleter> ProgramPipelineResourcePtr;
	typedef std::unique_ptr<GLuint, GpuQueryResourceDeleter> GpuQueryResourcePtr;


	class Factory abstract final
	{
		static GLuint GenOneBuffer()
		{
			GLuint temp = 0;
			glGenBuffers(1, &temp);
			return temp;
		}

		static GLuint GenOneTexture()
		{
			GLuint temp = 0;
			glGenTextures(1, &temp);
			return temp;
		}

		static GLuint GenOneFrameBuffer()
		{
			GLuint temp = 0;
			glGenFramebuffers(1, &temp);
			return temp;
		}

		static GLuint GenOneVertexArray()
		{
			GLuint temp = 0;
			glGenVertexArrays(1, &temp);
			return temp;
		}

		static GLuint GenOneSampler()
		{
			GLuint temp = 0;
			glGenSamplers(1, &temp);
			return temp;
		}

		static GLuint GenOneProgramPipeline()
		{
			GLuint temp = 0;
			glGenProgramPipelines(1, &temp);
			return temp;
		}

		static GLuint GenOneGpuQuery()
		{
			GLuint temp = 0;
			glGenQueries(1, &temp);
			return temp;
		}

	public:
		static ShaderResourcePtr CreateVertexShaderPtr()
		{
			return ShaderResourcePtr(glCreateShader(GL_VERTEX_SHADER));
		}

		static ShaderResourcePtr CreateTessControlShaderPtr()
		{
			return ShaderResourcePtr(glCreateShader(GL_TESS_CONTROL_SHADER));
		}

		static ShaderResourcePtr CreateTessEvaluationShaderPtr()
		{
			return ShaderResourcePtr(glCreateShader(GL_TESS_EVALUATION_SHADER));
		}

		static ShaderResourcePtr CreateGeometryShaderPtr()
		{
			return ShaderResourcePtr(glCreateShader(GL_GEOMETRY_SHADER));
		}

		static ShaderResourcePtr CreateFragmentShaderPtr()
		{
			return ShaderResourcePtr(glCreateShader(GL_FRAGMENT_SHADER));
		}

		static ShaderResourcePtr CreateComputeShaderPtr()
		{
			return ShaderResourcePtr(glCreateShader(GL_COMPUTE_SHADER));
		}

		static ProgramResourcePtr CreateProgramPtr()
		{
			return ProgramResourcePtr(glCreateProgram());
		}

		static BufferResourcePtr CreateOneBufferPtr()
		{
			return BufferResourcePtr(GenOneBuffer());
		}

		static TextureResourcePtr CreateOneTexturePtr()
		{
			return TextureResourcePtr(GenOneTexture());
		}

		static FrameBufferResourcePtr CreateOneFrameBufferPtr()
		{
			return FrameBufferResourcePtr(GenOneFrameBuffer());
		}

		static VertexArrayResourcePtr CreateOneVertexArrayPtr()
		{
			return VertexArrayResourcePtr(GenOneVertexArray());
		}

		static SamplerResourcePtr CreateOneSamplerPtr()
		{
			return SamplerResourcePtr(GenOneSampler());
		}

		static ProgramPipelineResourcePtr CreateOneProgramPipelinePtr()
		{
			return ProgramPipelineResourcePtr(GenOneProgramPipeline());
		}

		static GpuQueryResourcePtr CreateOneGpuQueryPtr()
		{
			return GpuQueryResourcePtr(GenOneGpuQuery());
		}
	};


#if 0
	class ShaderResource : boost::noncopyable
	{
	private:
		GLuint m_shader;
	public:
		ShaderResource()
			: m_shader()
		{}

		explicit ShaderResource(GLuint shader)
			: m_shader()
		{
			this->Attach(shader);
		}

		~ShaderResource()
		{
			this->Destroy();
		}

		void Attach(GLuint shader)
		{
			_ASSERTE(m_shader == 0);
			m_shader = shader;
		}

		GLuint Get() { return m_shader; }

		void Destroy()
		{
			if (m_shader)
			{
				glDeleteShader(m_shader);
				m_shader = 0;
			}
		}
	};
#endif

} // end of namespace
