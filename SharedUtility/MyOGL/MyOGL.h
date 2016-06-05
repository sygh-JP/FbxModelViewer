#pragma once


#include "MyOGLMesh.h"
#include "MyOGLFonTex.h"
#include "MyAppSettings.hpp"
#include "MyConstantBufferPacks.hpp"
#include "MyOGLDynamicFontRects.hpp"
#include "MyGLShaderLoc.hpp"



namespace MyOGL
{
	extern bool CreateCompressedTexture(GLenum targetDimension, GLenum texFormat, GLsizei imageWidth, GLsizei imageHeight, GLsizei imageDepth, GLint numMipmaps, const uint8_t* pCompressedImageData, size_t compressedImageDataSizeInBytes, TextureResourcePtr& outTexture);


	typedef MyCpuGpuCommon::MyBoneMatrixPalettePack CBufferBoneMatrixPalettePack;
	typedef MyCpuGpuCommon::MyBoneQuatPalettePack CBufferBoneQuatPalettePack;

	typedef MyCpuGpuCommon::MyViewParamsPack CBufferViewParamsPack;
	typedef MyCpuGpuCommon::MyMeshPartAttributePack CBufferMeshPartAttributePack;
	typedef MyCpuGpuCommon::MyLightParamsPack CBufferLightParamsPack;


	//! @brief  OpenGL インターフェイスを集中管理する。<br>
	//! レンダリング エンジンの役割を担う。<br>
	//! 非 MFC / WTL 環境でもそのまま流用可能にするため、MFC / WTL に依存するクラスは使用しない。<br>
	class MyOGLManager final : boost::noncopyable
	{
#pragma region // メンバー変数。//

		bool m_isInitialized;

		CPathW m_pathLogDir;
		CPathW m_pathMediaDir;

		HWND m_hWnd;
		HDC m_hDC;
		HGLRC m_hGLRC;

		ProgramResourcePtr m_programObjSkinningPhong;
		ProgramResourcePtr m_programObjShadingLess;
		ProgramResourcePtr m_programObjFontSprite;
		ProgramResourcePtr m_programObjMSAATransport;
		ProgramResourcePtr m_programObjDisplayWaveSim;
		ProgramResourcePtr m_programObjSimpleComputingTest;
		ProgramResourcePtr m_programObjUpdateRandomTable;

		ShaderParamLocationsBase m_locationsSkinningPhong;
		ShaderParamLocationsBase m_locationsShadingLess;
		ShaderParamLocationsBase m_locationsFontSprite;
		ShaderParamLocationsBase m_locationsMSAATransport;
		ShaderParamLocationsBase m_locationsDisplayWaveSim;
		ShaderParamCommonUniformLocations m_locationsSimpleComputingTest;

		VertexArrayResourcePtr m_inputLayoutPC;
		VertexArrayResourcePtr m_inputLayoutPCT;
		VertexArrayResourcePtr m_inputLayoutPCNT;
		VertexArrayResourcePtr m_inputLayoutPNTIW;

		BufferResourcePtr m_cbufferBoneMatrixPalette;
		BufferResourcePtr m_cbufferViewParams;
		BufferResourcePtr m_cbufferMeshPartAttribute;
		BufferResourcePtr m_cbufferLightParams;

		SamplerResourcePtr m_samplerPointWrap;
		SamplerResourcePtr m_samplerLinearWrap;
		SamplerResourcePtr m_samplerLinearClamp;

		BufferResourcePtr m_oneSquareVertexBufferPCT; // 正規化済み正方形（XY 面）の頂点バッファ。

		BufferResourcePtr m_coordAxisLineVertexBufferPC; // 座標軸の頂点バッファ。
		BufferResourcePtr m_waveFrontPlaneVertexBufferPCNT; // 水面（ZX 面）の矩形の頂点バッファ。

		BufferResourcePtr m_oneQuadIndexBuffer; // 四辺形のインデックス バッファ。頂点フォーマットによらず、使いまわしが可能。

		FrameBufferResourcePtr m_subFrameBuffer;
		TextureResourcePtr m_subColorBufferTexture;
		TextureResourcePtr m_subDepthBufferTexture;

		TextureResourcePtr m_dummyWhiteTexture;
		TextureResourcePtr m_dummyCubeTexture;

		TextureResourcePtr m_toonShadingRefTexture;
		TextureResourcePtr m_hudFontAlphaTexture;

		TextureResourcePtr m_envCubeTexture;

		//MyDeviceMeshPack m_squareMeshPCT; // 正規化済み正方形のメッシュ。

		BufferResourcePtr m_ssboRandomNumTableBuffer;
		TextureResourcePtr m_waveSimWorkTextures[2];
#if 0
		BufferResourcePtr m_ssboWaveSimWorkBuffers[2];
#endif

		TMyModelMeshPtrsArray m_mainMeshArray;
		TMyFileNameToTexture2DTable m_mainTexTable;

		// 定数バッファ転送の際の、ワーク用の一時領域。ローカル変数として毎回確保するのがややヘビーなデータ型に関しては、メンバーで持っておく。
		mutable CBufferBoneMatrixPalettePack m_boneMatrixPalette;
		mutable CBufferBoneQuatPalettePack m_boneQuatPalette;

		MyDynamicFontRects m_fontRects;
		const MyTextureHelper::FontTextureDataPack* m_pHudFontTexData;
		const MyTextureHelper::TextureDataPack* m_pToonShadingDiffuseCoefRefTexData;
		const MyCommon::TMyModelMeshDetailInfoPtrsArray* m_pModelMeshInfoArray;
		const MyCommon::TMyAnimMixerPtrsArrayPtrsArray* m_pAnimMixerArrayArray;
		const MyMath::MyGlobalMaterialTable* m_pGlobalMaterialTable;

		bool m_waveSimInOutFlipFlag;

		MyApp::FpsCounter m_fpsCounter;

		// TODO: 設定はインスタンスを単一にして、ポインタ経由で D3D/GL とで共有する。もしビューを複数持つ場合は、カメラを複数設ける。
		MyApp::MyCameraSettings m_myCameraSettings;
		MyApp::MyCommonSettings m_myCommonSettings;
		MyApp::MyEffectSettings m_myEffectSettings;

#pragma endregion
	private:
		void UpdateCBuffer(const CBufferBoneMatrixPalettePack* pSrcData) const;
		void UpdateCBuffer(const CBufferViewParamsPack* pSrcData) const;
		void UpdateCBuffer(const CBufferMeshPartAttributePack* pSrcData) const;
		void UpdateCBuffer(const CBufferLightParamsPack* pSrcData) const;

	private:
		void IdentifyAllSkinningBonePalette()
		{
			m_boneMatrixPalette.IdentifyAllMatrices();
			m_boneQuatPalette.IdentifyAllQuats();
		}

	private:
		void FindTextureAndBind(uint32_t slot, const std::wstring* pTexName)
		{
			// バインド先のスロット番号は glActiveTexture() で事前にアクティブ化しておく。
			// また、シェーダーに割り当てるスロット番号を glUniform1i() で事前に割り当てておく。
			// なお OpenGL は Direct3D とは違ってサンプラーをシェーダー側で指定できない。
			GLuint texId = m_dummyWhiteTexture.get();
			if (pTexName)
			{
				auto it = m_mainTexTable.find(*pTexName);
				if (it != m_mainTexTable.end())
				{
					texId = it->second.get();
				}
			}
			glBindSampler(slot, m_samplerLinearWrap.get());
			glBindTexture(GL_TEXTURE_2D, texId);
		}

	public:
		MyOGLManager()
			: m_isInitialized()
			, m_hWnd()
			, m_hDC()
			, m_hGLRC()
			, m_pHudFontTexData()
			, m_pToonShadingDiffuseCoefRefTexData()
			, m_pModelMeshInfoArray()
			, m_pAnimMixerArrayArray()
			, m_pGlobalMaterialTable()
			, m_waveSimInOutFlipFlag()
		{
		}

		~MyOGLManager()
		{
			this->Destroy();
		}

	public:
		void SetLogDirPath(const wchar_t* inPath) { m_pathLogDir.m_strPath = inPath; }
		void SetMediaDirPath(const wchar_t* inPath) { m_pathMediaDir.m_strPath = inPath; }

	public:
		TMyModelMeshPtrsArray& GetMainMeshArray() { return m_mainMeshArray; }

		void ClearMainMeshArray()
		{
			m_mainMeshArray.clear();
		}

		TMyFileNameToTexture2DTable& GetMainTexTable() { return m_mainTexTable; }

		void ClearMainTexTable()
		{
			m_mainTexTable.clear();
		}

	public:
		void SetHudFontTextureData(const MyTextureHelper::FontTextureDataPack* pFontTexData)
		{ m_pHudFontTexData = pFontTexData; }

		void SetToonShadingDiffuseCoefRefTextureData(const MyTextureHelper::TextureDataPack* pTexData)
		{ m_pToonShadingDiffuseCoefRefTexData = pTexData; }

		void SetModelMeshInfoArray(MyCommon::TMyModelMeshDetailInfoPtrsArray* pModelMeshInfoArray)
		{ m_pModelMeshInfoArray = pModelMeshInfoArray; }

		void SetAnimMixerArrayArray(MyCommon::TMyAnimMixerPtrsArrayPtrsArray* pAnimMixerArrayArray)
		{ m_pAnimMixerArrayArray = pAnimMixerArrayArray; }

		void SetGlobalMaterialTable(const MyMath::MyGlobalMaterialTable* pGlobalMaterialTable)
		{ m_pGlobalMaterialTable = pGlobalMaterialTable; }

	public:
#if 0
		void GetRotationAmount(MyMath::Vector3F* pAmount) const
		{ *pAmount = m_myCameraSettings.m_vRotation; }
#endif

		void SetRotationAmount(const MyMath::Vector3F* pAmount)
		{ m_myCameraSettings.m_vRotation = *pAmount; }

#if 0
		void GetCameraEye(MyMath::Vector3F* pEyePos) const
		{ *pEyePos = m_myCameraSettings.m_vCameraEye; }
#endif

		void SetCameraEye(const MyMath::Vector3F* pEyePos)
		{
			m_myCameraSettings.SetCameraEye(*pEyePos);
		}

		void PanCamera(MyMath::Vector2F shiftInPix)
		{
			m_myCameraSettings.PanCamera(shiftInPix);
		}

#if 0
		void GetMainLightRotationAmount(MyMath::Vector3F* pAmount) const
		{ *pAmount = m_myCommonSettings.MainLightRotation; }
#endif

		void SetMainLightRotationAmount(const MyMath::Vector3F* pAmount)
		{ m_myCommonSettings.MainLightRotation = *pAmount; }

		MyApp::MyCommonSettings& GetCommonSettings() { return m_myCommonSettings; }
		const MyApp::MyCommonSettings& GetCommonSettings() const { return m_myCommonSettings; }

		MyApp::MyEffectSettings& GetEffectSettings() { return m_myEffectSettings; }
		const MyApp::MyEffectSettings& GetEffectSettings() const { return m_myEffectSettings; }

	public:
		bool CreateEnvCubeMap(GLenum texFormat, GLsizei imageWidth, GLsizei imageHeight, GLint numMipmaps, const uint8_t* pImageData, size_t imageDataSizeInBytes)
		{
			return CreateCompressedTexture(GL_TEXTURE_CUBE_MAP, texFormat, imageWidth, imageHeight, 1, numMipmaps, pImageData, imageDataSizeInBytes, m_envCubeTexture);
		}

	private:
		bool CreateMyDummyWhiteTexture();
		bool CreateMyToonShadingRefTexture();
		void InitializeCameraSettings();
		void UpdateEffectMatrices();
		void DrawMeshArray(const ProgramResourcePtr& programObj);

		//! @brief  四辺形を1つ描画する。<br>
		//! @pre  頂点バッファと頂点レイアウト（VBO と VAO）は事前に呼び出し側でバインドしておくこと。<br>
		void DrawOneQuad()
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_oneQuadIndexBuffer.get());
			glDrawElements(GL_TRIANGLES, MyMath::OneQuadIndexCount, GL_UNSIGNED_SHORT, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		void DrawQuadInstances(uint32_t instanceCount)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_oneQuadIndexBuffer.get());
			glDrawElementsInstanced(GL_TRIANGLES, MyMath::OneQuadIndexCount, GL_UNSIGNED_SHORT, 0, instanceCount);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

	public:
		bool Create(UINT width, UINT height, HWND hWnd);

		// HACK: レンダリング時にフラグに応じてフレームを進めるのではなく、フレームを進める専用メソッドを作って分離する。
		bool Render(bool advancesFrame);

	private:
		bool ResizeScreen(UINT width, UINT height);
	public:
		bool SafeResizeScreen(UINT width, UINT height)
		{
			if (m_isInitialized)
			{
				return this->ResizeScreen(width, height);
			}
			return false;
		}

	private:
		bool CreateMyFontTexture();
	private:
		void Destroy();
		static bool CreateCoordAxisLineVertexBufferPC(BufferResourcePtr& outVertexBuffer);
		static bool CreateOneSquareVertexBufferPCT(BufferResourcePtr& outVertexBuffer);
		static bool CreateWaveFrontPlaneVertexBufferPCNT(BufferResourcePtr& outVertexBuffer);
		static bool CreateOneQuadIndexBuffer(BufferResourcePtr& outIndexBuffer);

	};
} // end of namespace
