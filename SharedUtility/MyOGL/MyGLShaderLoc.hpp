#pragma once

//#define USE_LEGACY_UNIFORM_VAR

namespace MyOGL
{
	// glGetProgramResourceIndex(), glGetUniformBlockIndex(), glGetUniformIndices() のエラー時戻り値用として
	// GL_INVALID_INDEX というシンボルが定義されているが、
	// GL_INVALID_INDEX を glGetUniformLocation() や glGetAttribLocation() の戻り値チェックに使うべきではない。
	// API リファレンスに即値 -1 が記載されているので、ユーザーコードでは即値を使うか、定数シンボルを明示的に定義しておく。
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetAttribLocation.xhtml

	const GLint InvalidShaderParamLocationVal = -1;

	//! @brief  複数のシェーダープログラムで同様に定義される Uniform 変数の Location (Handle) を管理する。<br>
	//! UBO で管理できない、スロット番号などを個別管理する。<br>
	//! 実際に使用されていない Uniform 変数の Location は無効値になる。<br>
	class ShaderParamCommonUniformLocations
	{
		// OpenGL 2.x / ES 2.0 では uniform 変数ごとに API を使って値を個別設定していく必要がある。
		// uniform に構造体を使う場合もそれは変わらない（構造体メンバーごとに個別設定する必要がある）。
		// テクスチャ サンプラーの uniform 変数以外は OpenGL 3.1 / ES 3.0 の GL_UNIFORM_BUFFER で高速化＆効率化する。
		// uniform 変数ではなく Uniform Buffer を使うと、ブロック単位でのデータ転送ができるほか、
		// シェーダープログラムを切り替えるたびに毎回定数データを設定し直す必要がなくなり、
		// Direct3D 10/11 の定数バッファのように複数のシェーダー間で定数データ ブロックを共有できるようになる。
		// なお、OpenGL 4.4 の GL_ARB_buffer_storage, GL_ARB_enhanced_layouts を使うと、
		// Direct3D 10/11 の定数バッファの packoffset のようにパッキング（メモリーレイアウト）をフル コントロールできるようになる。
		// つまり、 Direct3D 版で cbuffer に送信しているアライメント済みデータ構造を共用できるようになる。

		// ロケーションを保存する変数は、GLSL シェーダーで使っている in 変数や uniform 変数と同じシンボル名を使うと検索しやすい。
	private:
		GLint UniWorld;
		GLint UniView;
		GLint UniProjection;
		GLint UniWorldView;
		GLint UniViewProj;
		GLint UniWorldViewProj;

		GLint UniEyePosition;

		GLint UniLightDir;
		GLint UniLightPos;
		GLint UniLightColor;
		GLint UniAmbientLight;

		GLint UniMaterialColorDiffuse;
		GLint UniMaterialColorAmbient;
		GLint UniMaterialColorSpecular;
		GLint UniMaterialColorEmissive;

		GLint UniMaterialOpacityAlpha;
		GLint UniMaterialSpecularPower;
		GLint UniMaterialReflectivity;
		GLint UniMaterialIndexOfRefraction;

		GLint UniToonShadingRefTexV;

		GLint UniBoneMatrixPalette;

	public:
		GLint UniMainTexture;
		GLint UniToonShadingRefTexture;
		GLint UniEnvMapTexture;

		GLint UniWaveSimResultTexture;

		GLint UniWaveSimInputImage;
		GLint UniWaveSimOutputImage;

	public:
		ShaderParamCommonUniformLocations()
			: UniWorld(InvalidShaderParamLocationVal)
			, UniView(InvalidShaderParamLocationVal)
			, UniProjection(InvalidShaderParamLocationVal)
			, UniWorldView(InvalidShaderParamLocationVal)
			, UniViewProj(InvalidShaderParamLocationVal)
			, UniWorldViewProj(InvalidShaderParamLocationVal)
			, UniEyePosition(InvalidShaderParamLocationVal)
			, UniLightDir(InvalidShaderParamLocationVal)
			, UniLightPos(InvalidShaderParamLocationVal)
			, UniLightColor(InvalidShaderParamLocationVal)
			, UniAmbientLight(InvalidShaderParamLocationVal)
			, UniMaterialColorDiffuse(InvalidShaderParamLocationVal)
			, UniMaterialColorAmbient(InvalidShaderParamLocationVal)
			, UniMaterialColorSpecular(InvalidShaderParamLocationVal)
			, UniMaterialColorEmissive(InvalidShaderParamLocationVal)
			, UniMaterialOpacityAlpha(InvalidShaderParamLocationVal)
			, UniMaterialSpecularPower(InvalidShaderParamLocationVal)
			, UniMaterialReflectivity(InvalidShaderParamLocationVal)
			, UniMaterialIndexOfRefraction(InvalidShaderParamLocationVal)
			, UniToonShadingRefTexV(InvalidShaderParamLocationVal)
			, UniBoneMatrixPalette(InvalidShaderParamLocationVal)
			, UniMainTexture(InvalidShaderParamLocationVal)
			, UniToonShadingRefTexture(InvalidShaderParamLocationVal)
			, UniEnvMapTexture(InvalidShaderParamLocationVal)
			, UniWaveSimResultTexture(InvalidShaderParamLocationVal)
			, UniWaveSimInputImage(InvalidShaderParamLocationVal)
			, UniWaveSimOutputImage(InvalidShaderParamLocationVal)
		{}

	public:
		void FindLocationsInProgram(GLuint programObj)
		{
			// attribute/varying/in/out とは違い、uniform は取得できなくても OK。アサーションは不要。
			// 取得できなければ -1 が返ってくる。
			// Uniform Block 内のメンバー変数を取得しようとしても -1 が返るらしい。
#ifdef USE_LEGACY_UNIFORM_VAR
			this->UniWorld = glGetUniformLocation(programObj, "UniWorld");
			this->UniView = glGetUniformLocation(programObj, "UniView");
			this->UniProjection = glGetUniformLocation(programObj, "UniProjection");
			this->UniWorldView = glGetUniformLocation(programObj, "UniWorldView");
			this->UniViewProj = glGetUniformLocation(programObj, "UniViewProj");
			this->UniWorldViewProj = glGetUniformLocation(programObj, "UniWorldViewProj");

			this->UniEyePosition = glGetUniformLocation(programObj, "UniEyePosition");

			this->UniLightDir = glGetUniformLocation(programObj, "UniLightDir");
			this->UniLightPos = glGetUniformLocation(programObj, "UniLightPos");
			this->UniLightColor = glGetUniformLocation(programObj, "UniLightColor");
			this->UniAmbientLight = glGetUniformLocation(programObj, "UniAmbientLight");

			this->UniMaterialColorDiffuse = glGetUniformLocation(programObj, "UniMaterialColorDiffuse");
			this->UniMaterialColorAmbient = glGetUniformLocation(programObj, "UniMaterialColorAmbient");
			this->UniMaterialColorSpecular = glGetUniformLocation(programObj, "UniMaterialColorSpecular");
			this->UniMaterialColorEmissive = glGetUniformLocation(programObj, "UniMaterialColorEmissive");

			this->UniMaterialOpacityAlpha = glGetUniformLocation(programObj, "UniMaterialOpacityAlpha");
			this->UniMaterialSpecularPower = glGetUniformLocation(programObj, "UniMaterialSpecularPower");
			this->UniMaterialReflectivity = glGetUniformLocation(programObj, "UniMaterialReflectivity");
			this->UniMaterialIndexOfRefraction = glGetUniformLocation(programObj, "UniMaterialIndexOfRefraction");

			this->UniToonShadingRefTexV = glGetUniformLocation(programObj, "UniToonShadingRefTexV");

			this->UniBoneMatrixPalette = glGetUniformLocation(programObj, "UniBoneMatrixPalette");
#endif

			this->UniMainTexture = glGetUniformLocation(programObj, "UniMainTexture");
			this->UniToonShadingRefTexture = glGetUniformLocation(programObj, "UniToonShadingRefTexture");
			this->UniEnvMapTexture = glGetUniformLocation(programObj, "UniEnvMapTexture");

			this->UniWaveSimResultTexture = glGetUniformLocation(programObj, "UniWaveSimResultTexture");

			this->UniWaveSimInputImage = glGetUniformLocation(programObj, "UniWaveSimInputImage");
			this->UniWaveSimOutputImage = glGetUniformLocation(programObj, "UniWaveSimOutputImage");
		}

#ifdef USE_LEGACY_UNIFORM_VAR
		void UpdateUniWorldViewProjMatricesData(const MyMath::MatrixF& mWorld, const MyMath::MatrixF& mView, const MyMath::MatrixF& mProj, const MyMath::MatrixF& mWdVw, const MyMath::MatrixF& mVwPj, const MyMath::MatrixF& mWdVwPj) const
		{
			if (this->UniWorld != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniWorld, 1, GL_FALSE, &mWorld._11);
			}
			if (this->UniView != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniView, 1, GL_FALSE, &mView._11);
			}
			if (this->UniProjection != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniProjection, 1, GL_FALSE, &mProj._11);
			}

			this->UpdateUniWorldViewData(mWdVw);
			this->UpdateUniViewProjData(mVwPj);
			this->UpdateUniWorldViewProjData(mWdVwPj);
		}

		void UpdateUniWorldViewData(const MyMath::MatrixF& mWdVw) const
		{
			if (this->UniWorldView != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniWorldView, 1, GL_FALSE, &mWdVw._11);
			}
		}

		void UpdateUniViewProjData(const MyMath::MatrixF& mVwPj) const
		{
			if (this->UniViewProj != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniViewProj, 1, GL_FALSE, &mVwPj._11);
			}
		}

		void UpdateUniWorldViewProjData(const MyMath::MatrixF& mWdVwPj) const
		{
			if (this->UniWorldViewProj != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniWorldViewProj, 1, GL_FALSE, &mWdVwPj._11);
			}
		}

		void UpdateUniEyePositionData(const MyMath::Vector3F& vPos) const
		{
			if (this->UniEyePosition != InvalidShaderParamLocationVal)
			{
				glUniform3fv(this->UniEyePosition, 1, &vPos.x);
			}
		}

		void UpdateUniBoneMatrixPaletteData(const MyMath::MatrixF pMatrixArray[], GLsizei count) const
		{
			if (this->UniBoneMatrixPalette != InvalidShaderParamLocationVal)
			{
				glUniformMatrix4fv(this->UniBoneMatrixPalette, count, GL_FALSE, &pMatrixArray[0]._11);
			}
		}

		void UpdateUniLightDirData(const MyMath::Vector3F& vDir) const
		{
			if (this->UniLightDir != InvalidShaderParamLocationVal)
			{
				glUniform3fv(this->UniLightDir, 1, &vDir.x);
			}
		}

		void UpdateUniLightPosData(const MyMath::Vector3F& vPos) const
		{
			if (this->UniLightPos != InvalidShaderParamLocationVal)
			{
				glUniform3fv(this->UniLightPos, 1, &vPos.x);
			}
		}

		void UpdateUniLightColorData(const MyMath::Vector4F& vColor) const
		{
			if (this->UniLightColor != InvalidShaderParamLocationVal)
			{
				glUniform4fv(this->UniLightColor, 1, &vColor.x);
			}
		}

		void UpdateUniAmbientLightData(const MyMath::Vector4F& vColor) const
		{
			if (this->UniAmbientLight != InvalidShaderParamLocationVal)
			{
				glUniform4fv(this->UniAmbientLight, 1, &vColor.x);
			}
		}

		void UpdateUniMaterialDiffuseColorData(const MyMath::Vector4F& vColor) const
		{
			if (this->UniMaterialColorDiffuse != InvalidShaderParamLocationVal)
			{
				glUniform4fv(this->UniMaterialColorDiffuse, 1, &vColor.x);
			}
		}

		void UpdateUniMaterialAmbientColorData(const MyMath::Vector4F& vColor) const
		{
			if (this->UniMaterialColorAmbient != InvalidShaderParamLocationVal)
			{
				glUniform4fv(this->UniMaterialColorAmbient, 1, &vColor.x);
			}
		}

		void UpdateUniMaterialSpecularColorData(const MyMath::Vector4F& vColor) const
		{
			if (this->UniMaterialColorSpecular != InvalidShaderParamLocationVal)
			{
				glUniform4fv(this->UniMaterialColorSpecular, 1, &vColor.x);
			}
		}

		void UpdateUniMaterialEmissiveColorData(const MyMath::Vector4F& vColor) const
		{
			if (this->UniMaterialColorEmissive != InvalidShaderParamLocationVal)
			{
				glUniform4fv(this->UniMaterialColorEmissive, 1, &vColor.x);
			}
		}

		void UpdateUniMaterialOpacityAlphaData(float val) const
		{
			if (this->UniMaterialOpacityAlpha != InvalidShaderParamLocationVal)
			{
				glUniform1f(this->UniMaterialOpacityAlpha, val);
			}
		}

		void UpdateUniMaterialSpecularPowerData(float val) const
		{
			if (this->UniMaterialSpecularPower != InvalidShaderParamLocationVal)
			{
				glUniform1f(this->UniMaterialSpecularPower, val);
			}
		}

		void UpdateUniMaterialReflectivityData(float val) const
		{
			if (this->UniMaterialReflectivity != InvalidShaderParamLocationVal)
			{
				glUniform1f(this->UniMaterialReflectivity, val);
			}
		}

		void UpdateUniMaterialIndexOfRefractionData(float val) const
		{
			if (this->UniMaterialIndexOfRefraction != InvalidShaderParamLocationVal)
			{
				glUniform1f(this->UniMaterialIndexOfRefraction, val);
			}
		}


		void UpdateUniToonShadingRefTexV(float val) const
		{
			if (this->UniToonShadingRefTexV != InvalidShaderParamLocationVal)
			{
				glUniform1f(this->UniToonShadingRefTexV, val);
			}
		}
#endif
		// HACK: sampler2D などの uniform 変数も OpenGL 4.2 ARB_shading_language_420pack の layout:binding を使うことでシェーダー側で初期値を指定できるはず。
		// 柔軟性は下がるがその代わり冗長なロケーション取得処理が不要になる。
		// Direct3D のテクスチャ オブジェクトに対するレジスタースロット番号指定に似ている。

		// glUniform 系の API は事前に glUseProgram() が呼ばれている必要がある。
		// Program Pipeline Object を使う場合、glProgramUniform 系の API を使うなどの方法がある。
		// http://wlog.flatlib.jp/item/1636

		//! @brief  指定されたテクスチャ スロット番号と、Uniform 変数をバインドする（シェーダープログラム側にデータ転送する）。<br>
		//! 
		//! @pre  glUseProgram() が事前に呼び出され、適切なシェーダープログラムがバインドされていること。<br>
		void UpdateUniMainTextureSlot(GLint slotNum) const
		{
			if (this->UniMainTexture != InvalidShaderParamLocationVal)
			{
				glUniform1i(this->UniMainTexture, slotNum);
			}
		}

		void UpdateUniToonShadingRefTextureSlot(GLint slotNum) const
		{
			if (this->UniToonShadingRefTexture != InvalidShaderParamLocationVal)
			{
				glUniform1i(this->UniToonShadingRefTexture, slotNum);
			}
		}

		void UpdateUniEnvMapTextureSlot(GLint slotNum) const
		{
			if (this->UniEnvMapTexture != InvalidShaderParamLocationVal)
			{
				glUniform1i(this->UniEnvMapTexture, slotNum);
			}
		}

		void UpdateUniWaveSimResultTextureSlot(GLint slotNum) const
		{
			if (this->UniWaveSimResultTexture != InvalidShaderParamLocationVal)
			{
				glUniform1i(this->UniWaveSimResultTexture, slotNum);
			}
		}

		void UpdateUniWaveSimInput0ImageSlot(GLint slotNum) const
		{
			if (this->UniWaveSimInputImage != InvalidShaderParamLocationVal)
			{
				glUniform1i(this->UniWaveSimInputImage, slotNum);
			}
		}

		void UpdateUniWaveSimOutput0ImageSlot(GLint slotNum) const
		{
			if (this->UniWaveSimOutputImage != InvalidShaderParamLocationVal)
			{
				glUniform1i(this->UniWaveSimOutputImage, slotNum);
			}
		}
	};

	class ShaderParamLocationsBase
	{
	public:
		ShaderParamCommonUniformLocations CommonUniforms;

#if 0
	private:
		GLint VsInPosition;
		GLint VsInColor;
		GLint VsInNormal;
		GLint VsInTexCoord;
		GLint VsInBoneIndices0;
		GLint VsInBoneIndices1;
		GLint VsInBoneWeights0;
		GLint VsInBoneWeights1;
#endif

	public:
		ShaderParamLocationsBase()
#if 0
			: VsInPosition(InvalidShaderParamLocationVal)
			, VsInColor(InvalidShaderParamLocationVal)
			, VsInNormal(InvalidShaderParamLocationVal)
			, VsInTexCoord(InvalidShaderParamLocationVal)
			, VsInBoneIndices0(InvalidShaderParamLocationVal)
			, VsInBoneIndices1(InvalidShaderParamLocationVal)
			, VsInBoneWeights0(InvalidShaderParamLocationVal)
			, VsInBoneWeights1(InvalidShaderParamLocationVal)
#endif
		{}

	public:
		void FindLocationsInProgram(GLuint programObj)
		{
			this->CommonUniforms.FindLocationsInProgram(programObj);

#if 0
			this->VsInPosition = glGetAttribLocation(programObj, "VsInPosition");
			//_ASSERTE(this->VsInPosition != InvalidShaderParamLocationVal);
			this->VsInColor = glGetAttribLocation(programObj, "VsInColor");
			//_ASSERTE(this->VsInColor != InvalidShaderParamLocationVal);
			this->VsInNormal = glGetAttribLocation(programObj, "VsInNormal");
			//_ASSERTE(this->VsInNormal != InvalidShaderParamLocationVal);
			this->VsInTexCoord = glGetAttribLocation(programObj, "VsInTexCoord");
			//_ASSERTE(this->VsInTexCoord != InvalidShaderParamLocationVal);
			this->VsInBoneIndices0 = glGetAttribLocation(programObj, "VsInBoneIndices0");
			//_ASSERTE(this->VsInBoneIndices0 != InvalidShaderParamLocationVal);
			this->VsInBoneIndices1 = glGetAttribLocation(programObj, "VsInBoneIndices1");
			//_ASSERTE(this->VsInBoneIndices1 != InvalidShaderParamLocationVal);
			this->VsInBoneWeights0 = glGetAttribLocation(programObj, "VsInBoneWeights0");
			//_ASSERTE(this->VsInBoneWeights0 != InvalidShaderParamLocationVal);
			this->VsInBoneWeights1 = glGetAttribLocation(programObj, "VsInBoneWeights1");
			//_ASSERTE(this->VsInBoneWeights1 != InvalidShaderParamLocationVal);
#endif
		}
	};

	// HACK: glVertexAttribPointer() や glVertexAttribFormat() を使う際、
	// C++ テンプレートをうまく使うと、もっと型安全かつ楽に自動定義できるようになるかも。

	// NOTE: 頂点アトリビュートのための glEnableVertexAttribArray(), glVertexAttribPointer() 呼び出しのセットは、
	// OpenGL 3.0 / ES 3.0 で標準化された VAO を使って記録・再生するようにすると楽になる。
	// GPU との通信 API コールも減ることで高速化も期待できる。
	// http://shikihuiku.wordpress.com/2013/10/03/drawcall%E3%81%8C9%E5%80%8D%E6%97%A9%E3%81%8F%E3%81%AA%E3%82%8B%E3%83%AF%E3%82%B1/?relatedposts_exclude=750
	// なお、VAO は OpenGL 2.x までのユーザーメモリ頂点配列（Direct3D 9 の UP = User Memory Pointer 相当）とは別物。名前が紛らわしい。
	// また、VAO を使う場合も VBO は別途必要になる（VAO は FBO 同様、リソース本体ではない）ので注意。
	// ちなみに VAO に対して VBO や EBO を直接バインドする場合、VBO/EBO のインスタンスごとに VAO を用意しなければならず、
	// D3D の頂点バッファ＋頂点レイアウト（バッファとレイアウトが完全に分離されている）とは似て非なるものとなってしまう。
	// オブジェクトの管理も煩雑になるし、VAO の再利用性が下がる。
	// OpenGL 4.3 / ES 3.0 では Vertex Buffer と Vertex Attribute を分けて管理できる（D3D に近い）仕組みがようやく追加されたらしい。
	// http://wlog.flatlib.jp/item/1629
	// http://www.sinanc.org/blog/?p=450
	// VAO には glVertexAttribBinding() と glVertexAttribFormat() を使って初期化時に頂点レイアウト設定を事前登録（ステート保存）しておき、
	// レンダリング時には glBindBuffer(GL_ARRAY_BUFFER, vbo) の代わりに
	// glBindVertexArray(vao) と glBindVertexBuffer(bindingIndex, vbo, offset, stride) を使えばよいらしい。
	// glBindVertexBuffer() の bindingIndex はマルチ頂点ストリーム用として使えるらしい。
	// Direct3D は 10.x/11.x の頂点レイアウトの前身として、9.0 ですでに頂点宣言（FVF ではない）を導入していて、
	// さらにマルチ頂点ストリームの設定も簡単だったが、
	// OpenGL は 10 年近く経過してようやくその設計思想の利点を認めたらしい。気付くのが遅すぎる。

	// ※ OpenGL ES で VAO が「標準化」されたのは ES 3.0 以降で、ES 2.0 時点では拡張扱いだったのだが、
	// 「OpenGL ES 2.0 では VAO が推奨されている」という間違った情報が氾濫している。
	// 「Apple の iOS（というか PowerVR）では VAO 推奨」というのを間違って解釈している開発者がいるらしい。
	// 実際に glGenVertexArraysAPPLE(), glDeleteVertexArraysAPPLE() などの例にみられるように、
	// 拡張としてベンダー名を末尾に冠した関数シンボルが存在する。
	// 移植性を重視するならば、OpenGL ES 2.0 では VAO を使わないほうがよい。3.0 が普通に使えるようになるまで待つべし。
	// また、OpenGL 3.x は完成度が低く（Direct3D 10.0 の比ではない）、VAO の機能も中途半端なので、
	// OpenGL 4.3 以降を使ったほうがよい。


	//! @brief  <br>
	class VertexStreamInputLayoutHelperForPC final
	{
	public:
		typedef MyVertexTypes::MyVertexPC TVertex;
		static const GLsizei VertexStrideInBytes = sizeof(TVertex);
	public:
		static const GLuint VsInPosition = 0;
		static const GLuint VsInColor = 1;
	public:
		static void SaveVertexStreamInputLayoutStateToVao(const VertexArrayResourcePtr& vao)
		{
			_ASSERTE(vao.get() != 0);
			glBindVertexArray(vao.get());

			// NOTE: GLSL 側で in 変数に location を指定すれば、C/C++ 側でもその値を直接使える。
			// HLSL のセマンティクスのようなもの。
			// glGetAttribLocation() を使ってプログラム オブジェクトから取得する必要はない。
			// 固定値を使うと柔軟性は確かに下がるが、シェーダープログラムにおいて過度な柔軟性は混乱のもと。

			glEnableVertexAttribArray(VsInPosition);
			glEnableVertexAttribArray(VsInColor);

			glVertexAttribBinding(VsInPosition, 0);
			glVertexAttribBinding(VsInColor, 0);

			GLuint byteOffset = 0;
			glVertexAttribFormat(VsInPosition, 3, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Position)) == sizeof(float) * 3, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Position));
			glVertexAttribFormat(VsInColor, 4, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Color)) == sizeof(float) * 4, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Color));

			glBindVertexArray(0);
		}
	};

	//! @brief  <br>
	class VertexStreamInputLayoutHelperForPCT final
	{
	public:
		typedef MyVertexTypes::MyVertexPCT TVertex;
		static const GLsizei VertexStrideInBytes = sizeof(TVertex);
	public:
		static const GLuint VsInPosition = 0;
		static const GLuint VsInColor = 1;
		static const GLuint VsInTexCoord = 2;
	public:
		static void SaveVertexStreamInputLayoutStateToVao(const VertexArrayResourcePtr& vao)
		{
			_ASSERTE(vao.get() != 0);
			glBindVertexArray(vao.get());

			glEnableVertexAttribArray(VsInPosition);
			glEnableVertexAttribArray(VsInColor);
			glEnableVertexAttribArray(VsInTexCoord);

			glVertexAttribBinding(VsInPosition, 0);
			glVertexAttribBinding(VsInColor, 0);
			glVertexAttribBinding(VsInTexCoord, 0);

			GLuint byteOffset = 0;
			glVertexAttribFormat(VsInPosition, 3, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Position)) == sizeof(float) * 3, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Position));
			glVertexAttribFormat(VsInColor, 4, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Color)) == sizeof(float) * 4, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Color));
			glVertexAttribFormat(VsInTexCoord, 2, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::TexCoord)) == sizeof(float) * 2, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::TexCoord));

			glBindVertexArray(0);
		}
	};

	//! @brief  <br>
	class VertexStreamInputLayoutHelperForPCNT final
	{
	public:
		typedef MyVertexTypes::MyVertexPCNT TVertex;
		static const GLsizei VertexStrideInBytes = sizeof(TVertex);
	public:
		static const GLuint VsInPosition = 0;
		static const GLuint VsInColor = 1;
		static const GLuint VsInNormal = 2;
		static const GLuint VsInTexCoord = 3;
	public:
		static void SaveVertexStreamInputLayoutStateToVao(const VertexArrayResourcePtr& vao)
		{
			_ASSERTE(vao.get() != 0);
			glBindVertexArray(vao.get());

			glEnableVertexAttribArray(VsInPosition);
			glEnableVertexAttribArray(VsInColor);
			glEnableVertexAttribArray(VsInNormal);
			glEnableVertexAttribArray(VsInTexCoord);

			glVertexAttribBinding(VsInPosition, 0);
			glVertexAttribBinding(VsInColor, 0);
			glVertexAttribBinding(VsInNormal, 0);
			glVertexAttribBinding(VsInTexCoord, 0);

			GLuint byteOffset = 0;
			glVertexAttribFormat(VsInPosition, 3, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Position)) == sizeof(float) * 3, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Position));
			glVertexAttribFormat(VsInColor, 4, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Color)) == sizeof(float) * 4, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Color));
			glVertexAttribFormat(VsInNormal, 3, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Normal)) == sizeof(float) * 3, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Normal));
			glVertexAttribFormat(VsInTexCoord, 2, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::TexCoord)) == sizeof(float) * 2, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::TexCoord));

			glBindVertexArray(0);
		}
	};

	//! @brief  <br>
	class VertexStreamInputLayoutHelperForPNTIW final
	{
	public:
		typedef MyVertexTypes::MySkinVertex TVertex;
		static const GLsizei VertexStrideInBytes = sizeof(TVertex);
	public:
		static const GLuint VsInPosition = 0;
		static const GLuint VsInNormal = 1;
		static const GLuint VsInTexCoord = 2;
		static const GLuint VsInBoneIndices0 = 3;
		static const GLuint VsInBoneIndices1 = 4;
		static const GLuint VsInBoneWeights0 = 5;
		static const GLuint VsInBoneWeights1 = 6;
	public:
		static void SaveVertexStreamInputLayoutStateToVao(const VertexArrayResourcePtr& vao)
		{
			_ASSERTE(vao.get() != 0);
			glBindVertexArray(vao.get());

			glEnableVertexAttribArray(VsInPosition);
			glEnableVertexAttribArray(VsInNormal);
			glEnableVertexAttribArray(VsInTexCoord);
			glEnableVertexAttribArray(VsInBoneIndices0);
			glEnableVertexAttribArray(VsInBoneIndices1);
			glEnableVertexAttribArray(VsInBoneWeights0);
			glEnableVertexAttribArray(VsInBoneWeights1);

			glVertexAttribBinding(VsInPosition, 0);
			glVertexAttribBinding(VsInNormal, 0);
			glVertexAttribBinding(VsInTexCoord, 0);
			glVertexAttribBinding(VsInBoneIndices0, 0);
			glVertexAttribBinding(VsInBoneIndices1, 0);
			glVertexAttribBinding(VsInBoneWeights0, 0);
			glVertexAttribBinding(VsInBoneWeights1, 0);

			GLuint byteOffset = 0;
			glVertexAttribFormat(VsInPosition, 3, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Position)) == sizeof(float) * 3, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Position));
			glVertexAttribFormat(VsInNormal, 3, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::Normal)) == sizeof(float) * 3, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::Normal));
			glVertexAttribFormat(VsInTexCoord, 2, GL_FLOAT, GL_FALSE, byteOffset);
			static_assert(sizeof(decltype(TVertex::TexCoord)) == sizeof(float) * 2, "Invalid size!!");
			byteOffset += sizeof(decltype(TVertex::TexCoord));

			static_assert(sizeof(decltype(TVertex::BoneIndices)) == sizeof(uint8_t) * 8, "Invalid size!!");
			static_assert(sizeof(decltype(TVertex::BoneWeights)) == sizeof(float) * 8, "Invalid size!!");

			glVertexAttribIFormat(VsInBoneIndices0, 4, GL_UNSIGNED_BYTE, byteOffset);
			byteOffset += sizeof(uint8_t) * 4;
#if 1
			glVertexAttribIFormat(VsInBoneIndices1, 4, GL_UNSIGNED_BYTE, byteOffset);
			byteOffset += sizeof(uint8_t) * 4;
#endif
			glVertexAttribFormat(VsInBoneWeights0, 4, GL_FLOAT, GL_FALSE, byteOffset);
			byteOffset += sizeof(float) * 4;
#if 1
			glVertexAttribFormat(VsInBoneWeights1, 4, GL_FLOAT, GL_FALSE, byteOffset);
			byteOffset += sizeof(float) * 4;
#endif

			glBindVertexArray(0);
		}

	private:
		//! @brief  頂点ストリームのレイアウトを有効にする。<br>
		//! 
		//! @pre  glBindBuffer() が事前に呼び出され、適切な VBO がバインドされていること。<br>
		//! もしくは glBindVertexArray() が事前に呼び出され、適切な VAO がバインドされていること。<br>
		static void EnableVertexStreamLayout()
		{
			glEnableVertexAttribArray(VsInPosition);
			glEnableVertexAttribArray(VsInNormal);
			glEnableVertexAttribArray(VsInTexCoord);
			glEnableVertexAttribArray(VsInBoneIndices0);
			glEnableVertexAttribArray(VsInBoneIndices1);
			glEnableVertexAttribArray(VsInBoneWeights0);
			glEnableVertexAttribArray(VsInBoneWeights1);

			const uint8_t* pByteOffset = nullptr;
			glVertexAttribPointer(VsInPosition, 3, GL_FLOAT, GL_FALSE, VertexStrideInBytes, pByteOffset);
			static_assert(sizeof(decltype(TVertex::Position)) == sizeof(float) * 3, "Invalid size!!");
			pByteOffset += sizeof(decltype(TVertex::Position));
			glVertexAttribPointer(VsInNormal, 3, GL_FLOAT, GL_FALSE, VertexStrideInBytes, pByteOffset);
			static_assert(sizeof(decltype(TVertex::Normal)) == sizeof(float) * 3, "Invalid size!!");
			pByteOffset += sizeof(decltype(TVertex::Normal));
			glVertexAttribPointer(VsInTexCoord, 2, GL_FLOAT, GL_FALSE, VertexStrideInBytes, pByteOffset);
			static_assert(sizeof(decltype(TVertex::TexCoord)) == sizeof(float) * 2, "Invalid size!!");
			pByteOffset += sizeof(decltype(TVertex::TexCoord));
			//glVertexAttribPointer(VsInBoneIndices0, 4, GL_UNSIGNED_BYTE, GL_FALSE, VertexStrideInBytes, pByteOffset); // NG.
			//glVertexAttribPointer(VsInBoneIndices0, 4, GL_UNSIGNED_BYTE, GL_TRUE, VertexStrideInBytes, pByteOffset); // OK.
			// byte[4] は glVertexAttribPointer() の normalized パラメータに GL_TRUE を渡し、
			// 整数を正規化して浮動小数点数値として渡さないとダメっぽい。
			// GLSL シェーダー側では vec4 で受け取り、正規化された値を明示的にスケーリングして復元する必要がある。
			// short[4] などに関しても同様。
			// ivec4 では直接受け取れないらしい。
			// Direct3D の HLSL のように頂点シェーダー側に渡すときセマンティクスを使って自動でマッピングしてくれる仕組みはないらしい。
			// ivec4 に正規化なしで直接マッピングできるのは、GL_UNSIGNED_INT[4] と GL_INT[4] だけらしい？
			// そもそも GLSL 1.2 までの attribute/varying や、GLSL 1.3 以降の in/out には、ivec4 は使えないらしい。
			// uniform はどうなのか、また GLSL 3.x や 4.x の場合もその制限があるのかどうかは不明。
			// ……と思っていたが、OpenGL 3.3 以降では、glVertexAttribIPointer() を使えばいいらしい。GLSL 3.3 以降では、in/out にも ivec4 が使える模様。
			// OpenGL 4.3 以降では Attribute Pointer よりも Attribute Format を使ったほうが圧倒的に楽になる。
			glVertexAttribIPointer(VsInBoneIndices0, 4, GL_UNSIGNED_BYTE, VertexStrideInBytes, pByteOffset); // OK.
			pByteOffset += sizeof(uint8_t) * 4;
#if 1
			glVertexAttribIPointer(VsInBoneIndices1, 4, GL_UNSIGNED_BYTE, VertexStrideInBytes, pByteOffset); // OK.
			pByteOffset += sizeof(uint8_t) * 4;
#endif
			glVertexAttribPointer(VsInBoneWeights0, 4, GL_FLOAT, GL_FALSE, VertexStrideInBytes, pByteOffset);
			pByteOffset += sizeof(float) * 4;
#if 1
			glVertexAttribPointer(VsInBoneWeights1, 4, GL_FLOAT, GL_FALSE, VertexStrideInBytes, pByteOffset);
			pByteOffset += sizeof(float) * 4;
#endif
		}
	private:
		static void DisableVertexStreamLayout()
		{
			glDisableVertexAttribArray(VsInPosition);
			glDisableVertexAttribArray(VsInNormal);
			glDisableVertexAttribArray(VsInTexCoord);
			glDisableVertexAttribArray(VsInBoneIndices0);
			glDisableVertexAttribArray(VsInBoneIndices1);
			glDisableVertexAttribArray(VsInBoneWeights0);
			glDisableVertexAttribArray(VsInBoneWeights1);
		}
	};

} // end of namespace
