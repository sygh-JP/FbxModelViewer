#pragma once

//#include "DirectXHelper.h"

#include "MyD3DCubeMap.h"
#include "MyD3DMesh.h"
#include "MyD3DFonTex.h"
#include "MyD3DDynamicFontRects.hpp"
#include "MyAppSettings.hpp"
#include "MyConstantBufferPacks.hpp"
#include "MyD3DVertexLayout.hpp"
#include "MyUIAnimCenter.h"
#include "MyCascadedShadowHelpers.hpp"


namespace MyD3D
{

	typedef MyCpuGpuCommon::MyBoneMatrixPalettePack CBufferBoneMatrixPalettePack;
	typedef MyCpuGpuCommon::MyBoneQuatPalettePack CBufferBoneQuatPalettePack;

	typedef MyCpuGpuCommon::MyViewParamsPack CBufferViewParamsPack;
	typedef MyCpuGpuCommon::MyMeshPartAttributePack CBufferMeshPartAttributePack;
	typedef MyCpuGpuCommon::MyLightParamsPack CBufferLightParamsPack;

	class CBufferClassInstanceSelectorTable
	{
		//typedef MyMath::Vector4F TEachSelectorDummyType;
		using TEachSelectorDummyType = MyMath::Vector4F;

		TEachSelectorDummyType UniInstanceSpecularDummy;
		TEachSelectorDummyType UniInstanceSpecularBlinnPhong;
		TEachSelectorDummyType UniInstanceSpecularCookTorrance;

		TEachSelectorDummyType UniInstanceMainLightingShaderPhotoReal;
		TEachSelectorDummyType UniInstanceMainLightingShaderToon;

		TEachSelectorDummyType UniInstanceEnvMapColorPickerDummy;
		TEachSelectorDummyType UniInstanceEnvMapColorPickerReflect;
		TEachSelectorDummyType UniInstanceEnvMapColorPickerRefract;
		TEachSelectorDummyType UniInstanceEnvMapColorPickerFresnel;

		TEachSelectorDummyType UniInstanceHogeA;
		TEachSelectorDummyType UniInstanceHogeB;
		TEachSelectorDummyType UniInstanceHogeC;
	public:
		CBufferClassInstanceSelectorTable()
		{}
	};

	static_assert(sizeof(CBufferClassInstanceSelectorTable) % 16 == 0, "Not aligned!!");


	/////////////////////////////////////////////////////////////////////////////////////

	//! @brief  D3D インターフェイスを集中管理する。<br>
	//! レンダリング エンジンの役割を担う。<br>
	class MyD3DManager final : boost::noncopyable
	{
		//static const UINT MAX_GAUSSIAN_WEIGHTS_KERNEL_HALF = 7;
	private:
#pragma region // メンバー変数。//

		bool m_isInitialized;

		CPathW m_pathLogDir;
		CPathW m_pathMediaDir;

		UINT m_msaaSampleCount;
		UINT m_msaaQuality;
		float m_dpi;

		Microsoft::WRL::ComPtr<ID3D11Device> m_pD3DDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pD3DImmediateContext; //!< メインスレッド用の D3D デバイス コンテキスト。<br>

		Microsoft::WRL::ComPtr<IDWriteFactory1> m_dwriteFactory;
		Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
		Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_d2dTargetBitmap;

		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_d2dBlackBrush;
		Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
		Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;

#if 0
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
#else
		Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain;
#endif
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pBackBufferTex; //!< アプリで生成するのではなく、スワップ チェーン（ウィンドウ）から取得する。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV; //!< レンダーターゲットにするために必要。<br>

#pragma region // ファーマップ用。//
		// 頂点シェーダーでは MSAA テクスチャを参照することができない。一方 MRT はフォーマットをそろえる必要がある。
		// DirectX 11.1 では頂点シェーダーも UAV にアクセスできるので、そちらを使ったほうが効率的。

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pFurGeoMapTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pFurGeoMapSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pFurGeoMapRTV; //!< レンダーターゲットにするために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pMSAAFurGeoMapTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pMSAAFurGeoMapSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pMSAAFurGeoMapRTV; //!< レンダーターゲットにするために必要。<br>

#pragma endregion

#pragma region // 法線・深度マップ用。//

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pNormalDepthMapTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pNormalDepthMapSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pNormalDepthMapRTV; //!< レンダーターゲットにするために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pMSAANormalDepthMapTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pMSAANormalDepthMapSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pMSAANormalDepthMapRTV; //!< レンダーターゲットにするために必要。<br>
		// 非 MSAA テクスチャと UAV を作ってピクセル シェーダーに書き込ませたほうがよいか？

#pragma endregion

#pragma region // FP16 MSAA オフスクリーン レンダリング用（ターシャリ バッファ）。//

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pSubColorBufferTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSubColorBufferSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pSubColorBufferRTV; //!< レンダーターゲットにするために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pSubDepthStencilTex;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pSubDSV;

#pragma endregion

#pragma region // バリアンス カスケード シャドウ用。//
		// シャドウマップは MSAA を使わない。
		// 深度ステンシルはカラーバッファと MSAA レベルを合わせる必要があるため、
		// MSAA ターシャリ バッファ用の深度ステンシルは使い回せない。
		// シャドウマップ用に深度ステンシルも別途確保する必要がある。
		// カスケードはテクスチャ配列で実現する。
		// シャドウ自体の描画は、ジオメトリ シェーダーと MRT を使うと1パスでレンダリングできる。また、RTV/SRV は1つ作成するだけでよい。
		// {(カスケード段数×R32FG32F＋D32F)×W×H} でメモリ使用量が見積もれる。
		// 例えば {(カスケード3段＋深度)×1024×768} で 21[MB] になる。
		// {(カスケード3段＋深度)×1024×1024} で 28[MB] になる。
		// {(カスケード3段＋深度)×2048×2048} で 112[MB] になる。
		// {(カスケード3段＋深度)×4096×4096} で 448[MB] になる。
		// システムの空きメモリに合わせてシャドウ解像度をスケーラブルに変更できるようにしておいたほうがいい。
		// また、バリアンス目的のブラー（ぼかし）用に、単一スライスのテクスチャでなくテクスチャ配列を使う場合、
		// 深度ステンシルを除いて同容量のバッファがもう1つ必要になるので、メモリ使用量はテクスチャの解像度に応じてさらに急激に増加する。

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pShadowColorBufferTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pShadowColorBufferSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pShadowColorBufferRTV; //!< レンダーターゲットにするために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pShadowColorBufferUAV; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pShadowBlurWorkTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pShadowBlurWorkSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pShadowBlurWorkRTV; //!< レンダーターゲットにするために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pShadowBlurWorkUAV; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pShadowDepthStencilTex;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pShadowDSV;
#pragma endregion

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pDummyWhiteTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDummyWhiteSRV; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pToonShadingRefTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pToonShadingRefSRV; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pHudFontAlphaTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pHudFontAlphaTexSRV; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3DX11Effect> m_pMainEffect;
		Microsoft::WRL::ComPtr<ID3DX11Effect> m_pComputeEffect;
		Microsoft::WRL::ComPtr<ID3DX11Effect> m_pScreenEffect;
		Microsoft::WRL::ComPtr<ID3DX11Effect> m_pShadowEffect;
		Microsoft::WRL::ComPtr<ID3D11ClassLinkage> m_pClassLinkagePSMainLighting;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPSMainLighting;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterStateSolidMsaa;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterStateWireMsaa;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterStateShadow;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutT;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutP;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutPC;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutPCT;
		//CComPtr<ID3D11InputLayout> m_pLayoutP4CT;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutPCNT;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutPNTIW;
		//Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayoutFlake;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pOneTriangleVertexBufferPCNT; // テスト用の三角形（XY 面）の頂点バッファ。
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pOneSquareVertexBufferPCNT; // テスト用の正方形（XY 面）の頂点バッファ。
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pOneSquareVertexBufferPCT; // 正規化済み正方形（XY 面）の頂点バッファ。

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pImageBasedFurVertexBufferPCT;
		UINT m_imageBasedFurVertexCount;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pSinglePointVertexBufferP; // 原点位置座標のみを持つ頂点バッファ。
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCoordAxisLineVertexBufferPC; // 座標軸の頂点バッファ。
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pWaveFrontPlaneVertexBufferPCNT; // 水面（ZX 面）の矩形の頂点バッファ。
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pWaveFrontPlaneGridBuf;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pWaveFrontPlaneGridSRV;
		UINT m_waveFrontPlaneGridCount;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pFlakeParticleBuf;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pFlakeParticleSRV;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pFlakeParticleUAV;
		UINT m_flakeParticleCount;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pOneQuadIndexBuffer; // 四辺形のインデックス バッファ。頂点フォーマットによらず、使いまわしが可能。

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pRandomTableBuf;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pRandomTableSRV; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pRandomTableUAV; //!< HLSL コード中でリソース参照するために必要。<br>

		// 入力と出力をピンポン（フリップ）するためにバッファが2つ必要となる。

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_ppDownSampledTempWorkTex[2];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ppDownSampledTempWorkSRV[2]; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_ppDownSampledTempWorkRTV[2]; //!< レンダーターゲットにするために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_ppDownSampledTempWorkUAV[2]; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture1D> m_ppCSReductionWorkTex[2];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ppCSReductionWorkSRV[2]; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_ppCSReductionWorkUAV[2]; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_ppPSReductionWorkTex[2];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ppPSReductionWorkSRV[2]; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_ppPSReductionWorkRTV[2]; //!< HLSL コード中でリソース参照するために必要。<br>

		// 波動方程式を数値計算で解くために、3世代分のバッファが必要。
		static const uint32_t WaveSimWorkBufferCount = 3;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_ppWaveSimWorkTex[WaveSimWorkBufferCount];
#if 0
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_ppWaveSimWorkStructBuffers[WaveSimWorkBufferCount];
#endif
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ppWaveSimWorkSRV[WaveSimWorkBufferCount]; //!< HLSL コード中でリソース参照するために必要。<br>
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_ppWaveSimWorkUAV[WaveSimWorkBufferCount]; //!< HLSL コード中でリソース参照するために必要。<br>

		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pWaveSimMaskTex;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pWaveSimMaskSRV;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerPointWrap;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerLinearWrap;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerLinearClamp;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerShadowLinearClamp;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerPointBorderTransparent;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferBoneMatrixPalette;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferViewParams;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferMeshPartAttribute;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferLightParams;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferClassInstanceSelectorTable;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferShadowRenderingInfo;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pCBufferShadowSamplingInfo;

#if 0
		MyRenderTargetCubeMap m_envCubeMap;
#else
		MyCubeMap m_envCubeMap;
#endif

		//! @brief  エフェクト テクニック群。<br>
		//! 
		//! D3DX メソッドの引数でなく戻り値で取得した D3DX インターフェイス ポインタは基本的に Release() する必要は無いらしい。<br>
		struct EffectTechsPack
		{
			ID3DX11EffectTechnique* TechRenderCoordAxisLines;
			ID3DX11EffectTechnique* TechRenderLightDirLine;
			ID3DX11EffectTechnique* TechTessBillboard;
			ID3DX11EffectTechnique* TechGeomBillboard;
			ID3DX11EffectTechnique* TechFlakeParticle;
			ID3DX11EffectTechnique* TechFont;
			ID3DX11EffectTechnique* TechShadowHudTest;
			ID3DX11EffectTechnique* TechLambertPCNT;
			ID3DX11EffectTechnique* TechTransportFromMSAA8;
			ID3DX11EffectTechnique* TechTransportFromMSAA4;
			ID3DX11EffectTechnique* TechTransportFromMSAA2;
			ID3DX11EffectTechnique* TechTransportFromNonMSAA;
			ID3DX11EffectTechnique* TechExtractHighIntensityFromMSAA8;
			ID3DX11EffectTechnique* TechExtractHighIntensityFromMSAA4;
			ID3DX11EffectTechnique* TechExtractHighIntensityFromMSAA2;
			ID3DX11EffectTechnique* TechExtractHighIntensityFromNonMSAA;
			ID3DX11EffectTechnique* TechSkinning;
			ID3DX11EffectTechnique* TechImageBasedFur;
			ID3DX11EffectTechnique* TechAddDownSampledTex;
			ID3DX11EffectTechnique* TechDisplaySimpleComputingTest;
			ID3DX11EffectTechnique* TechEdgeDetectColorSketch;
			ID3DX11EffectTechnique* TechShadowRender;
			ID3DX11EffectTechnique* TechApplyHorizontalMovingAverageFilterToShadowTex;
			ID3DX11EffectTechnique* TechApplyVerticalMovingAverageFilterToShadowTex;
			ID3DX11EffectTechnique* TechApplyHorizontalGaussianBlurToDownSampledTex;
			ID3DX11EffectTechnique* TechApplyVerticalGaussianBlurToDownSampledTex;
			ID3DX11EffectTechnique* TechReduction2x2;
			ID3DX11EffectTechnique* TechReduction4x4;
			ID3DX11EffectTechnique* TechReduction8x8;
			ID3DX11EffectTechnique* CSTechReductionTexture2DTo1D;
			ID3DX11EffectTechnique* CSTechReductionTexture1DTo1D;
			ID3DX11EffectTechnique* CSTechSimpleComputingTest;
			ID3DX11EffectTechnique* CSTechUpdateRandomTable;
			ID3DX11EffectTechnique* CSTechUpdateFlakeParticle;
			ID3DX11EffectTechnique* CSTechApplyGaussianBlurToDownSampledTex;
			ID3DX11EffectTechnique* CSTechApplyBlurToShadowTex;
		};
		EffectTechsPack m_effectTechs;

		TMyModelMeshPtrsArray m_mainMeshArray;
		TMyFileNameToTexture2DTable m_mainTexTable;

		// 定数バッファ転送の際の、ワーク用の一時領域。ローカル変数として毎回確保するのがややヘビーなデータ型に関しては、メンバーで持っておく。
		// HACK: もし並列レンダリングする場合は、キャッシュ領域は複数持っている必要がある。
		mutable CBufferBoneMatrixPalettePack m_boneMatrixPalette;
		mutable CBufferBoneQuatPalettePack m_boneQuatPalette;
		mutable CBufferShadowSamplingInfoPack m_tempShadowSamplingInfo;

		MyShadowCascadeConfig m_shadowCascadeConfig;
		MyShadowMapManager m_shadowMapManager;

		MyDynamicFontRects m_fontRects;
		const MyTextureHelper::FontTextureDataPack* m_pHudFontTexData;
		const MyTextureHelper::TextureDataPack* m_pToonShadingDiffuseCoefRefTexData;
		const MyCommon::TMyModelMeshDetailInfoPtrsArray* m_pModelMeshInfoArray;
		const MyCommon::TMyAnimMixerPtrsArrayPtrsArray* m_pAnimMixerArrayArray;
		const MyMath::MyGlobalMaterialTable* m_pGlobalMaterialTable;

		uint32_t m_waveSimInOutFlipCounter;

		MyApp::FpsCounter m_fpsCounter;

		// TODO: 設定はインスタンスを単一にして、ポインタ経由で D3D/GL とで共有する。もしビューを複数持つ場合は、カメラを複数設ける。
		MyApp::MyCameraSettings m_myCameraSettings;
		MyApp::MyCommonSettings m_myCommonSettings;
		MyApp::MyEffectSettings m_myEffectSettings;

		MyUIAnimCenter m_myUIAnimCenter;

#pragma endregion
	private:
		void UpdateCBuffer(const CBufferBoneMatrixPalettePack* pSrcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferBoneMatrixPalette.Get(), 0, nullptr, pSrcData, 0, 0);
		}
		void UpdateCBuffer(const CBufferViewParamsPack* pSrcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferViewParams.Get(), 0, nullptr, pSrcData, 0, 0);
		}
		void UpdateCBuffer(const CBufferMeshPartAttributePack* pSrcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferMeshPartAttribute.Get(), 0, nullptr, pSrcData, 0, 0);
		}
		void UpdateCBuffer(const CBufferLightParamsPack* pSrcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferLightParams.Get(), 0, nullptr, pSrcData, 0, 0);
		}
#if 0
		void UpdateCBuffer(const CBufferClassInstanceSelectorTable& srcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferClassInstanceSelectorTable.Get(), 0, nullptr, &srcData, 0, 0);
		}
#endif
		void UpdateCBuffer(const CBufferShadowRenderingInfoPack& srcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferShadowRenderingInfo.Get(), 0, nullptr, &srcData, 0, 0);
		}
		void UpdateCBuffer(const CBufferShadowSamplingInfoPack& srcData)
		{
			_ASSERTE(m_pD3DImmediateContext);
			m_pD3DImmediateContext->UpdateSubresource(m_pCBufferShadowSamplingInfo.Get(), 0, nullptr, &srcData, 0, 0);
		}

	private:
		void IdentifyAllSkinningBonePalette()
		{
			m_boneMatrixPalette.IdentifyAllMatrices();
			m_boneQuatPalette.IdentifyAllQuats();
		}

	public:
		MyD3DManager()
			: m_isInitialized()
			, m_msaaSampleCount(1)
			, m_msaaQuality()
			, m_dpi(96.0f)
			, m_imageBasedFurVertexCount()
			, m_waveFrontPlaneGridCount()
			, m_flakeParticleCount()
			, m_effectTechs()
			, m_pHudFontTexData()
			, m_pToonShadingDiffuseCoefRefTexData()
			, m_pModelMeshInfoArray()
			, m_pAnimMixerArrayArray()
			, m_pGlobalMaterialTable()
			, m_waveSimInOutFlipCounter()
		{
		}

		~MyD3DManager()
		{
			this->ClearMainTexTable();
			this->ClearMainMeshArray(); // std::vector のデストラクタで自動解放されるから、別になくても OK。
			// そのほかの COM インターフェイスは、スマートポインタ定義の逆順に（スマートポインタのスタックが破壊される順に）自動解放される。
			m_isInitialized = false;
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
		void GetRotationAmount(MyMath::Vector3F* pAmount) const
		{ *pAmount = m_myCameraSettings.m_vRotation; }

		void SetRotationAmount(const MyMath::Vector3F* pAmount)
		{ m_myCameraSettings.m_vRotation = *pAmount; }

		void GetCameraEye(MyMath::Vector3F* pEyePos) const
		{ *pEyePos = m_myCameraSettings.m_vCameraEye; }

		void SetCameraEye(const MyMath::Vector3F* pEyePos)
		{
			m_myCameraSettings.SetCameraEye(*pEyePos);
		}

		void PanCamera(MyMath::Vector2F shiftInPix)
		{
			m_myCameraSettings.PanCamera(shiftInPix);
		}

		void GetMainLightRotationAmount(MyMath::Vector3F* pAmount) const
		{ *pAmount = m_myCommonSettings.MainLightRotation; }

		void SetMainLightRotationAmount(const MyMath::Vector3F* pAmount)
		{ m_myCommonSettings.MainLightRotation = *pAmount; }

		MyApp::MyCommonSettings& GetCommonSettings() { return m_myCommonSettings; }
		const MyApp::MyCommonSettings& GetCommonSettings() const { return m_myCommonSettings; }

		MyApp::MyEffectSettings& GetEffectSettings() { return m_myEffectSettings; }
		const MyApp::MyEffectSettings& GetEffectSettings() const { return m_myEffectSettings; }

	public:
		void FireProjectile()
		{
			// とりあえずスクリーン右下から投射。
			//m_myUIAnimCenter.FireProjectile(D2D1::Point2F(m_myCameraSettings.GetScreenWidthF(), m_myCameraSettings.GetScreenHeightF()));
			m_myUIAnimCenter.FireProjectile(D2D1::Point2F(m_myCameraSettings.GetScreenWidthF(), 0));
		}

	public:
		bool CreateEnvCubeMap(DXGI_FORMAT texFormat, uint32_t imageWidth, uint32_t imageHeight, uint32_t mipLevels, const uint8_t* pImageData, size_t imageDataSizeInBytes)
		{
			_ASSERTE(m_pD3DDevice);
			m_envCubeMap.Release();
			// HACK: 読み込みに成功したらスワップするようにする。
			return m_envCubeMap.Create(m_pD3DDevice.Get(), texFormat, imageWidth, imageHeight, mipLevels, pImageData, imageDataSizeInBytes);
		}

	private:
		void UpdateEffectMatrices();
#if 0
		void SetupViewport(UINT width, UINT height);
		void SetupViewportWithCurrentScreenSize()
		{
			this->SetupViewport(m_myCameraSettings.m_screenWidth, m_myCameraSettings.m_screenHeight);
		}
#endif

	public:
		bool DumpTextureToFile(ID3D11Texture2D* pTexture, LPCWSTR pFileName);

	private:
		void TransportByCurrentMSAA()
		{
			// 転送元と転送先の解像度とフォーマットが同じであれば、ResolveSubresource() が使える。
			// MSAA から非 MSAA へのダウンサンプル転送にも使える。
			switch (m_msaaSampleCount)
			{
			case 8:
				this->DrawOneQuad(m_effectTechs.TechTransportFromMSAA8);
				break;
			case 4:
				this->DrawOneQuad(m_effectTechs.TechTransportFromMSAA4);
				break;
			case 2:
				this->DrawOneQuad(m_effectTechs.TechTransportFromMSAA2);
				break;
			default:
				this->DrawOneQuad(m_effectTechs.TechTransportFromNonMSAA);
				break;
			}
		}
		void ExtractHighIntensityByCurrentMSAA()
		{
			switch (m_msaaSampleCount)
			{
			case 8:
				this->DrawOneQuad(m_effectTechs.TechExtractHighIntensityFromMSAA8);
				break;
			case 4:
				this->DrawOneQuad(m_effectTechs.TechExtractHighIntensityFromMSAA4);
				break;
			case 2:
				this->DrawOneQuad(m_effectTechs.TechExtractHighIntensityFromMSAA2);
				break;
			default:
				this->DrawOneQuad(m_effectTechs.TechExtractHighIntensityFromNonMSAA);
				break;
			}
		}

	private:
		void FindTextureAndSetToPS(uint32_t slot, const std::wstring* pTexName)
		{
			auto pDeviceContext = this->GetMainThreadDeviceContext();
			ID3D11ShaderResourceView* srv = m_pDummyWhiteSRV.Get();
			if (pTexName)
			{
				auto it = m_mainTexTable.find(*pTexName);
				if (it != m_mainTexTable.end())
				{
					srv = it->second.TextureSRV.Get();
				}
			}
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				srv,
			};
			pDeviceContext->PSSetShaderResources(slot, ARRAYSIZE(ppSRViews), ppSRViews);
		}

	private:
		bool SetupRenderState();
		bool CreateEffect(const void* pBytecode, size_t bytecodeLength, _Out_ Microsoft::WRL::ComPtr<ID3DX11Effect>& pEffect);
		bool CreateInputVertexLayout(const D3D11_INPUT_ELEMENT_DESC pElementDescArray[], UINT numElements, const void* pBytecode, size_t bytecodeLength, _Out_ Microsoft::WRL::ComPtr<ID3D11InputLayout>& pLayout);
		bool CreateInputVertexLayoutP(const void* pBytecode, size_t bytecodeLength);
		bool CreateInputVertexLayoutPC(const void* pBytecode, size_t bytecodeLength);
		bool CreateInputVertexLayoutPCT(const void* pBytecode, size_t bytecodeLength);
		bool CreateInputVertexLayoutPCNT(const void* pBytecode, size_t bytecodeLength);
		bool CreateInputVertexLayoutPNTIW(const void* pBytecode, size_t bytecodeLength);
		//bool CreateInputVertexLayoutP4CT();
		bool CreateRenderTargetColorTexAndView(DXGI_FORMAT format, UINT width, UINT height, UINT msaaSampleCount, UINT msaaQuality);
		bool CreateDepthStencilAndView(UINT width, UINT height, UINT msaaSampleCount, UINT msaaQuality);
		bool CreateBackBufferRenderTargetView(UINT width, UINT height);
		bool CreateWaveSimWorkTexAndView();
		bool CreateWaveSimWorkBufferAndView();
		bool CreateRandomTableBufferAndView();
		bool CreateFlakeParticleBufferAndView();
		void GetTechniquesFromEffect();
		void InitializeCameraSettings();

		bool CreateMyDynamicFontRects();
		bool CreateSinglePointVertexBufferP();
		bool CreateCoordAxisLineVertexBufferPC();
		bool CreateOneTriangleVertexBuffer();
		bool CreateOneSquareVertexBufferPCNT();
		bool CreateWaveFrontPlaneVertexBufferPCNT();
		bool CreateWaveFrontPlaneGridBufferAndView();
		bool CreateOneSquareVertexBufferPCT();
		bool CreateOneQuadIndexBuffer();
		bool CreateImageBasedFurVertexBuffer(UINT width, UINT height);
		bool CreateMyFontTexture();
		bool CreateMyDummyWhiteTexture();
		bool CreateMyToonShadingRefTexture();

	private:
		void ApplyPsBlurToShadowTex();
		void ApplyCsBlurToShadowTex();
		void ApplyPsBlurToDownSampledTex();
		void ApplyCsBlurToDownSampledTex();
		void DoPsParellelReductionTest(ID3D11ShaderResourceView* pReductionSrcImageView, uint32_t reductionSrcImageSize);
		void DoCsParellelReductionTest(ID3D11ShaderResourceView* pReductionSrcImageView, uint32_t reductionSrcImageSize);

	private:
		bool CreateMyShaderResourceView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& pSRV, _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC* pViewDesc = nullptr);
		bool CreateMyRenderTargetView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& pRTV, _In_opt_ const D3D11_RENDER_TARGET_VIEW_DESC* pViewDesc = nullptr);
		bool CreateMyDepthStencilView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& pDSV, _In_opt_ const D3D11_DEPTH_STENCIL_VIEW_DESC* pViewDesc = nullptr);
		bool CreateMyUnorderedAccessView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& pUAV, _In_opt_ const D3D11_UNORDERED_ACCESS_VIEW_DESC* pViewDesc = nullptr);

		bool CreateMyBufferShaderResourceView(ID3D11Buffer* pBuffer, DXGI_FORMAT format, UINT numElements, _Out_ Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& pSRV);
		bool CreateMyBufferUnorderedAccessView(ID3D11Buffer* pBuffer, DXGI_FORMAT format, UINT numElements, _Out_ Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& pUAV);

		bool CreateMyBufferShaderResourceView(ID3D11Buffer* pBuffer, _Out_ Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& pSRV);
		bool CreateMyBufferUnorderedAccessView(ID3D11Buffer* pBuffer, _Out_ Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& pUAV);

		void SetMyVertexBuffer(ID3D11Buffer* pMyBuffer, UINT vertexSizeInBytes, D3D_PRIMITIVE_TOPOLOGY topologyType);

		void SetOneQuadIndexBuffer()
		{ this->GetMainThreadDeviceContext()->IASetIndexBuffer(m_pOneQuadIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0); }

		void DrawPrimitive(ID3DX11EffectTechnique* pTechnique, UINT vertexCount);
		void DrawInstancedPrimitive(ID3DX11EffectTechnique* pTechnique, UINT vertexCountPerInstance, UINT instanceCount);
		void DrawIndexedPrimitive(ID3DX11EffectTechnique* pTechnique, UINT indexCount);
		void DrawIndexedInstancedPrimitive(ID3DX11EffectTechnique* pTechnique, UINT indexCountPerInstance, UINT instanceCount);
		void DrawMeshArray(ID3DX11EffectTechnique* pTechnique, ID3D11ClassLinkage* pClassLinkage = nullptr, ID3D11PixelShader* pPixelShader = nullptr);

		void DrawOneTriangle(ID3DX11EffectTechnique* pTechnique)
		{ this->DrawPrimitive(pTechnique, 3U); }
		void DrawOneQuad(ID3DX11EffectTechnique* pTechnique)
		{ this->DrawIndexedPrimitive(pTechnique, 6U); }

		void DrawTriangleInstances(ID3DX11EffectTechnique* pTechnique, UINT instanceCount)
		{ this->DrawInstancedPrimitive(pTechnique, 3U, instanceCount); }
		void DrawQuadInstances(ID3DX11EffectTechnique* pTechnique, UINT instanceCount)
		{ this->DrawIndexedInstancedPrimitive(pTechnique, 6U, instanceCount); }

		void Dispatch(ID3DX11EffectTechnique* pTechnique, UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);

	public:
		ID3D11Device* GetD3DDevice() const { return m_pD3DDevice.Get(); }

	private:
		ID3D11DeviceContext* GetMainThreadDeviceContext() { return m_pD3DImmediateContext.Get(); }

	public:
		bool Create(UINT width, UINT height, HWND hWnd);

		// HACK: レンダリング時にフラグに応じてフレームを進めるのではなく、フレームを進める専用メソッドを作って分離する。
		bool Render(bool advancesFrame, bool renders3d);

	private:
		void Render3D(bool advancesFrame);

	private:
		bool ResizeScreen(UINT width, UINT height);
	private:
		bool ResizeShadowBuffer(uint32_t width, uint32_t height, uint32_t cascadeCount);
	public:
		bool SafeResizeScreen(UINT width, UINT height)
		{
			// D3D 初期化前にウィンドウへリサイズ メッセージが飛んできたときの対処。ただしサブスレッドからの呼び出しには非対応。
			if (m_isInitialized)
			{
				return this->ResizeScreen(width, height);
			}
			return false;
		}
	};

} // end of namespace
