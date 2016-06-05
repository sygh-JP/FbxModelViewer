#pragma once

// グラフィックス API やリソースの生成状況に依存しない、抽象化されたシャドウ情報を管理するデータ型を記述する。
// とはいっても CBuffer の定義は GPU-friendly なフォーマットになっているが……

#include "MyMathConsts.hpp"


const int MAX_SHADOW_CASCADES_NUM = 8;
const int MAXIMUM_BLUR_LEVELS = 7;


// GPU 側のリソースに関係するカスケード設定情報。
// これに変更があった場合、リソースを再作成する必要がある。
class MyShadowCascadeConfig final
{
public:
	uint32_t m_nCascadeLevels;
	uint32_t m_iBufferSize;

public:
	MyShadowCascadeConfig()
		: m_nCascadeLevels()
		, m_iBufferSize()
	{}

private:
	bool IsSameAs(const MyShadowCascadeConfig& other) const
	{
	    // If any of our paramaters was changed, we must reallocate the D3D/GL resources.
		return
			this->m_nCascadeLevels == other.m_nCascadeLevels &&
			this->m_iBufferSize == other.m_nCascadeLevels;
	}
public:
	bool operator ==(const MyShadowCascadeConfig& other) const
	{ return this->IsSameAs(other); }
	bool operator !=(const MyShadowCascadeConfig& other) const
	{ return !this->IsSameAs(other); }
};

// シャドウマップの描画時に使う定数。
// カスケードされる複数のシャドウマップはジオメトリ シェーダーと MRT を使って1パスでレンダリングするため、
// ライト行列も複数用意しておく。シェーダー側では SV_RenderTargetArrayIndex でカスケード インデックスを取得する。
class CBufferShadowRenderingInfoPack final
{
public:
	DirectX::XMFLOAT4X4 UniCascadedLightViewProj[MAX_SHADOW_CASCADES_NUM];
	//int32_t FirstCascadeIndex;
	//int32_t LastCascadeIndex;
	int32_t UniAvailableCascadeCount;
	int32_t cb_Dummy0[3];
};


// シャドウマップの参照時に使う定数。
class CBufferShadowSamplingInfoPack final
{
public:
	DirectX::XMFLOAT4X4  UniShadowViewMatrix;
	DirectX::XMFLOAT4 m_vCascadeOffset[MAX_SHADOW_CASCADES_NUM];
	DirectX::XMFLOAT4 m_vCascadeScale[MAX_SHADOW_CASCADES_NUM];

	//int32_t         m_nCascadeLevels; // Number of Cascades

	//int32_t         m_iPCFBlurForLoopStart; // For loop begin value. For a 5x5 kernel this would be -2.
	//int32_t         m_iPCFBlurForLoopEnd; // For loop end value. For a 5x5 kernel this would be 3.

	// For Map based selection scheme, this keeps the pixels inside of the the valid range.
	// When there is no boarder, these values are 0 and 1 respectively.
	float       m_fMinBorderPadding;
	float       m_fMaxBorderPadding;

	// A shadow map offset to deal with self shadow artifacts.
	// These artifacts are aggravated by PCF.
	//float       m_fShadowBiasFromGUI;
	//float       m_fShadowPartitionSize;

	float       m_fCascadeBlendArea; // Amount to overlap when blending between cascades.
	float       m_fTexelSize; // Shadow map texel size.
	float       m_fNativeTexelSizeInX; // Texel size in native map ( textures are packed ).
	int32_t         m_iVisualizeCascades; // 1 is to visualize the cascades in different colors. 0 is to just draw the scene.
	//float       m_fPaddingForCB3; // Padding variables CBs must be a multiple of 16 bytes.

	float UniShadowDepthBias; // VSM には必要ない。
	float Dummy0;

	float       m_fCascadeFrustumsEyeSpaceDepths[MAX_SHADOW_CASCADES_NUM]; // The values along Z that separate the cascades.

	// the values along Z that separate the cascades.
	// Wastefully stored in float4 so they are array indexable :(
	//XMFLOAT4 m_fCascadeFrustumsEyeSpaceDepthsFloat4[MAX_SHADOW_CASCADES_NUM];
};

static_assert(sizeof(CBufferShadowRenderingInfoPack) % 16 == 0, "Not aligned!!");
static_assert(sizeof(CBufferShadowSamplingInfoPack) % 16 == 0, "Not aligned!!");

__declspec(deprecated) typedef CBufferShadowSamplingInfoPack CB_ALL_SHADOW_DATA;
__declspec(deprecated) typedef MyShadowCascadeConfig CascadeConfig;


// バリアンス カスケード シャドウマップの管理クラス。
// Direct3D や OpenGL には依存せず、シーンのジオメトリ情報やレンダリング条件のみを管理する。
class MyShadowMapManager final
{
private:
	static const int MAX_CASCADES = MAX_SHADOW_CASCADES_NUM;

	enum FIT_PROJECTION_TO_CASCADES
	{
		FIT_TO_CASCADES,
		FIT_TO_SCENE, // SDK サンプルの初期値。
	};

	// CascadedShadowMaps11 のサンプルには FIT_NEARFAR_PANCAKING というのもあったが、
	// VarianceShadows11 のサンプルには存在しない。

	enum FIT_TO_NEAR_FAR
	{
		FIT_NEARFAR_ZERO_ONE,
		FIT_NEARFAR_AABB,
		FIT_NEARFAR_SCENE_AABB, // SDK サンプルの初期値。
	};

private:
	int32_t                                 m_iCascadePartitionsMax;
	float                               m_fCascadePartitionsFrustum[MAX_CASCADES]; // Values are between near and far
	int32_t                                 m_iCascadePartitionsZeroToOne[MAX_CASCADES]; // Values are 0 to 100 and represent a percent of the frustum
	int32_t                                 m_iShadowBlurSize;
	int32_t                                 m_iBlurBetweenCascades;
	float                               m_fBlurBetweenCascadesAmount;

	bool                                m_bMoveLightTexelSize;
	FIT_PROJECTION_TO_CASCADES          m_eSelectedCascadesFit;
	FIT_TO_NEAR_FAR                     m_eSelectedNearFarFit;

	DirectX::XMFLOAT3                            m_vSceneAABBMin;
	DirectX::XMFLOAT3                            m_vSceneAABBMax;
	// For example: when the shadow buffer size changes.

	DirectX::XMFLOAT4X4                          m_matShadowProj[MAX_CASCADES];
	DirectX::XMFLOAT4X4                          m_matShadowView;

	MyShadowCascadeConfig                       m_CopyOfCascadeConfig;      // This copy is used to determine when settings change.
	// Some of these settings require new buffer allocations.

	//CascadeConfig*                      m_pCascadeConfig;           // Pointer to the most recent setting.

public:
	MyShadowMapManager();

public:
	DirectX::XMFLOAT3 GetSceneAABBMin() const { return m_vSceneAABBMin; }
	DirectX::XMFLOAT3 GetSceneAABBMax() const { return m_vSceneAABBMax; }
	int32_t GetShadowBlurSize() const { return m_iShadowBlurSize; }
	//void SetSceneAABBMin(const DirectX::XMFLOAT3& vIn) { m_vSceneAABBMin = vIn; }
	//void SetSceneAABBMax(const DirectX::XMFLOAT3& vIn) { m_vSceneAABBMax = vIn; }
	const MyShadowCascadeConfig& GetCurrentCascadeConfig() const { return m_CopyOfCascadeConfig; }

private:
	DirectX::XMMATRIX GetShadowViewProjectionMatrix(uint32_t cascadeIndex) const
	{
		_ASSERTE(0 <= cascadeIndex && cascadeIndex < ARRAYSIZE(m_matShadowProj));
		return DirectX::XMMatrixMultiply(
			DirectX::XMLoadFloat4x4(&m_matShadowView),
			DirectX::XMLoadFloat4x4(&m_matShadowProj[cascadeIndex]));
	}
private:
	DirectX::XMMATRIX GetTrShadowViewProjectionMatrix(uint32_t cascadeIndex) const
	{ return DirectX::XMMatrixTranspose(this->GetShadowViewProjectionMatrix(cascadeIndex)); }
private:
	void GetShadowViewProjectionMatrix(DirectX::XMFLOAT4X4& outMatrix, uint32_t cascadeIndex) const
	{ DirectX::XMStoreFloat4x4(&outMatrix, this->GetShadowViewProjectionMatrix(cascadeIndex)); }
private:
	void GetTrShadowViewProjectionMatrix(DirectX::XMFLOAT4X4& outMatrix, uint32_t cascadeIndex) const
	{ DirectX::XMStoreFloat4x4(&outMatrix, this->GetTrShadowViewProjectionMatrix(cascadeIndex)); }
private:
	void GetShadowViewMatrix(DirectX::XMFLOAT4X4& outMatrix) const
	{ outMatrix = m_matShadowView; }
private:
	void GetTrShadowViewMatrix(DirectX::XMFLOAT4X4& outMatrix) const
	{ DirectX::XMStoreFloat4x4(&outMatrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&m_matShadowView))); }

public:
	//--------------------------------------------------------------------------------------
	// This function is where the real work is done. We determine the matrices and constants used in
	// shadow generation and scene generation.
	//--------------------------------------------------------------------------------------
	void InitFrame(
		const MyShadowCascadeConfig& cascadeConfig,
		const DirectX::XMFLOAT3& vSceneAABBMin,
		const DirectX::XMFLOAT3& vSceneAABBMax,
		const DirectX::XMFLOAT4X4& inSceneCameraView,
		const DirectX::XMFLOAT4X4& inSceneCameraProj,
		float inSceneCameraNearClip,
		float inSceneCameraFarClip,
		const DirectX::XMFLOAT4X4& inLightView);

	// NOTE: リソースに関わるカスケード設定は頻繁に変わるものではないが、ビュー行列のほうは通常絶えず変化するので、
	// 結局初期化処理（シャドウ行列およびシャドウ サンプリング情報の生成処理）は毎フレーム呼び出すことになるはず。

	void GetShadowRenderingInfo(CBufferShadowRenderingInfoPack& outShadowRenderingInfo) const
	{
		outShadowRenderingInfo.UniAvailableCascadeCount = m_CopyOfCascadeConfig.m_nCascadeLevels;
		for (uint32_t iCurrentCascade = 0; iCurrentCascade < m_CopyOfCascadeConfig.m_nCascadeLevels; ++iCurrentCascade)
		{
			this->GetTrShadowViewProjectionMatrix(outShadowRenderingInfo.UniCascadedLightViewProj[iCurrentCascade], iCurrentCascade);
		}
	}

#if 0
	void OnRenderShadowsForAllCascades() const
	{
		// カスケードのレンダリングループの疑似コード。
		using namespace DirectX;

		for (uint32_t iCurrentCascade=0; iCurrentCascade < m_CopyOfCascadeConfig.m_nCascadeLevels; ++iCurrentCascade)
		{
			// カスケード インデックスに応じて行列を返すだけの getter メソッドを作り、カスケード ループの中で呼び出すようにすればよい。
			XMFLOAT4X4 lightMatrix;
			this->GetTrShadowWorldViewProjectionMatrix(lightMatrix, iCurrentCascade);

			// TODO: 定数バッファに行列データを送信する。
			// TODO: 各シャドウマップをレンダリングする。
		}
		// TODO: 上記ループで行列の配列を作成しておいて、ジオメトリ シェーダーと MRT を活用した1パスレンダリングを行なう。

		if (m_iShadowBlurSize > 1)
		{
			// TODO: 各シャドウマップにブラー（ぼかし）をかける。
			// ぼかしはピクセル シェーダーで行なうか、コンピュート シェーダーを活用するとよい。
			// CS の場合はワーク用の共有メモリのサイズ上限に注意。シャドウマップのサイズが共有メモリのサイズ上限を超える場合は
			// タイリングを検討する必要がある。
			const int iKernelShader = ( m_iShadowBlurSize / 2 - 1 ); // 3 goes to 0, 5 goes to 1
			for (uint32_t iCurrentCascade=0; iCurrentCascade < m_CopyOfCascadeConfig.m_nCascadeLevels; ++iCurrentCascade)
			{
			}
		}
	}
#endif

	void GetShadowSamplingInfo(CBufferShadowSamplingInfoPack& outShadowSamplingInfo, float shadowDepthBias = 0.0f, bool visualizesCascades = false) const
	{
		// 定数バッファのデータ メンバーをすべて設定する。
		using namespace DirectX;

		outShadowSamplingInfo.m_fCascadeBlendArea = m_fBlurBetweenCascadesAmount;
		outShadowSamplingInfo.m_fTexelSize = 1.0f / m_CopyOfCascadeConfig.m_iBufferSize;
		outShadowSamplingInfo.m_fNativeTexelSizeInX = outShadowSamplingInfo.m_fTexelSize / m_CopyOfCascadeConfig.m_nCascadeLevels;

		//outShadowSamplingInfo.m_nCascadeLevels = m_CopyOfCascadeConfig.m_nCascadeLevels;
		outShadowSamplingInfo.m_iVisualizeCascades = visualizesCascades;
		outShadowSamplingInfo.UniShadowDepthBias = shadowDepthBias;

		// ビュー・射影変換後のクリップ座標を、左上原点のテクスチャ座標に直すための行列らしい。
		const XMMATRIX dxmatTextureScale = XMMatrixScaling(+0.5f, -0.5f, 1.0f);
		const XMMATRIX dxmatTextureTranslation = XMMatrixTranslation(0.5f, 0.5f, 0.0f);

		this->GetTrShadowViewMatrix(outShadowSamplingInfo.UniShadowViewMatrix);
		for (uint32_t index = 0; index < m_CopyOfCascadeConfig.m_nCascadeLevels; ++index)
		{
			XMFLOAT4X4 mShadowTexture;
			XMStoreFloat4x4(&mShadowTexture, XMLoadFloat4x4(&m_matShadowProj[index]) * dxmatTextureScale * dxmatTextureTranslation);
			outShadowSamplingInfo.m_vCascadeScale[index].x = mShadowTexture._11;
			outShadowSamplingInfo.m_vCascadeScale[index].y = mShadowTexture._22;
			outShadowSamplingInfo.m_vCascadeScale[index].z = mShadowTexture._33;
			outShadowSamplingInfo.m_vCascadeScale[index].w = 1;

			outShadowSamplingInfo.m_vCascadeOffset[index].x = mShadowTexture._41;
			outShadowSamplingInfo.m_vCascadeOffset[index].y = mShadowTexture._42;
			outShadowSamplingInfo.m_vCascadeOffset[index].z = mShadowTexture._43;
			outShadowSamplingInfo.m_vCascadeOffset[index].w = 0;
		}
		// Copy intervals for the depth interval selection method.
		std::copy_n(m_fCascadePartitionsFrustum, MAX_SHADOW_CASCADES_NUM, outShadowSamplingInfo.m_fCascadeFrustumsEyeSpaceDepths);

		// The border padding values keep the pixel shader from reading the borders during PCF filtering.
		outShadowSamplingInfo.m_fMaxBorderPadding = float(m_CopyOfCascadeConfig.m_iBufferSize - 1.0f) /
			m_CopyOfCascadeConfig.m_iBufferSize;
		outShadowSamplingInfo.m_fMinBorderPadding = 1.0f /
			m_CopyOfCascadeConfig.m_iBufferSize;
	}
};
