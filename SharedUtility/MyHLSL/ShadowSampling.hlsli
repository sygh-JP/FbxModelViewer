// Only UTF-8 or ASCII is available.

//--------------------------------------------------------------------------------------
// File: RenderCascadeScene.hlsl
//
// This is the main shader file.  This shader is compiled with several different flags
// to provide different customizations based on user controls.
//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

//#define BLEND_BETWEEN_CASCADE_LAYERS_FLAG 1
//#define SELECT_CASCADE_BY_INTERVAL_FLAG 1

// This flag enables the shadow to blend between cascades.  This is most useful when the
// the shadow maps are small and artifact can be seen between the various cascade layers.
#ifndef BLEND_BETWEEN_CASCADE_LAYERS_FLAG
#define BLEND_BETWEEN_CASCADE_LAYERS_FLAG 0
#endif

// There are two methods for selecting the proper cascade a fragment lies in.  Interval selection
// compares the depth of the fragment against the frustum's depth partition.
// Map based selection compares the texture coordinates against the acutal cascade maps.
// Map based selection gives better coverage.
// Interval based selection is easier to extend and understand.
#ifndef SELECT_CASCADE_BY_INTERVAL_FLAG
#define SELECT_CASCADE_BY_INTERVAL_FLAG 0
#endif

// The number of cascades
#ifndef CASCADE_COUNT_FLAG
#define CASCADE_COUNT_FLAG 3
#endif


// Most titles will find that 3-4 cascades with
// BLEND_BETWEEN_CASCADE_LAYERS_FLAG, is good for lower end PCs.

cbuffer CBufferShadowSamplingInfoPack : register(b6)
{
	//matrix          m_mWorldViewProjection;
	//matrix          m_mWorld;
	//matrix          m_mWorldView;

	matrix UniShadowViewMatrix : packoffset(c0);
	float4 m_vCascadeOffset[MAX_SHADOW_CASCADES_NUM] : packoffset(c4);
	float4 m_vCascadeScale[MAX_SHADOW_CASCADES_NUM] : packoffset(c12);

	//int             m_nCascadeLevels; // Number of Cascades
	// DirectX SDK のサンプルに存在したが、まったく使われていなかった。
	// CascadedShadowMaps11 も VarianceShadows11 も同様。

	// For Map based selection scheme, this keeps the pixels inside of the the valid range.
	// When there is no boarder, these values are 0 and 1 respectively.
	float m_fMinBorderPadding : packoffset(c20.x);
	float m_fMaxBorderPadding : packoffset(c20.y);

	float m_fCascadeBlendArea : packoffset(c20.z); // Amount to overlap when blending between cascades.
	float m_fTexelSize : packoffset(c20.w); // Padding variables exist because CBs must be a multiple of 16 bytes.
	float m_fNativeTexelSizeInX : packoffset(c21.x);
	int m_iVisualizeCascades : packoffset(c21.y); // 1 is to visualize the cascades in different colors. 0 is to just draw the scene
	float UniShadowDepthBias : packoffset(c21.z);
	//float       m_fPaddingForCB3; // Padding variables CBs must be a multiple of 16 bytes.

#if 0
	float4          m_fCascadeFrustumsEyeSpaceDepthsData[MAX_SHADOW_CASCADES_NUM / 4]; // The values along Z that separate the cascades.
	// This code creates an array based pointer that points towards the vectorized input data.
	// This is the only way to index arbitrary arrays of data.
	// If the array is used at run time, the compiler will generate code that uses logic to index the correct component.

	static float    m_fCascadeFrustumsEyeSpaceDepths[MAX_SHADOW_CASCADES_NUM] =
		(float[MAX_SHADOW_CASCADES_NUM])m_fCascadeFrustumsEyeSpaceDepthsData;

	// これはフィールドの再解釈テクニックらしい（More Aggressive Packing）。
	// HLSL では、デフォルトで配列はパッキングされないというルールがある。
	// すなわち、float someArray[2]; とすると、実際は float4 someArray[2]; と同じだけレジスタが消費される。
	// GPU でのアドレッシングは float4 単位になることが起因している。
	// packoffset での代替は無理そう？　GLSL には果たして相当機能があるのか？
	// HLSL でもシェーダーリフレクションを切っていると使えない？
	// D3D11 WARNING: ID3D11DeviceContext::DrawIndexed: The size of the Constant Buffer at slot 6 of the Pixel Shader unit is too small (384 bytes provided, 416 bytes, at least, expected). This is OK, as out-of-bounds reads are defined to return 0. It is also possible the developer knows the missing data will not be used anyway. This is only a problem if the developer actually intended to bind a sufficiently large Constant Buffer for what the shader expects.  [ EXECUTION WARNING #351: DEVICE_DRAW_CONSTANT_BUFFER_TOO_SMALL]
	// http://msdn.microsoft.com/ja-jp/library/ee418340.aspx
	// http://msdn.microsoft.com/en-us/library/bb509632.aspx
#else
	// 定数バッファのレジスタを節約。
	float UniCascadeFrustumsEyeSpaceDepths0 : packoffset(c22.x);
	float UniCascadeFrustumsEyeSpaceDepths1 : packoffset(c22.y);
	float UniCascadeFrustumsEyeSpaceDepths2 : packoffset(c22.z);
	float UniCascadeFrustumsEyeSpaceDepths3 : packoffset(c22.w);
	float UniCascadeFrustumsEyeSpaceDepths4 : packoffset(c23.x);
	float UniCascadeFrustumsEyeSpaceDepths5 : packoffset(c23.y);
	float UniCascadeFrustumsEyeSpaceDepths6 : packoffset(c23.z);
	float UniCascadeFrustumsEyeSpaceDepths7 : packoffset(c23.w);
#endif

	//float3          m_vLightDir;
	//float           m_fPaddingCB4;

};


#if 0
//--------------------------------------------------------------------------------------
// Textures and Samplers
//--------------------------------------------------------------------------------------
Texture2D           g_txDiffuse             : register( t0 );
//Texture2DArray      g_txShadow              : register( t5 );

SamplerState g_samLinear                    : register( s0 ); // LinearWrap
//SamplerState g_samShadow                    : register( s5 ); // AnisoClamp or LinearClamp

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition                        : POSITION;
	float3 vNormal                          : NORMAL;
	float2 vTexcoord                        : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 vPosition                        : SV_Position;
	float3 vNormal                          : NORMAL;
	float2 vTexcoord                        : COLOR0;
	float4 vTexShadow                       : TEXCOORD1;
	//float4 vInterpPos                       : TEXCOORD2; // DirectX SDK のサンプルに存在したが、まったく使われていなかった。
	// CascadedShadowMaps11 も VarianceShadows11 も同様。
	float  vDepth                           : TEXCOORD3;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain( VS_INPUT Input )
{
	VS_OUTPUT Output;

	Output.vPosition = mul( Input.vPosition, UniWorldViewProj );
	Output.vNormal = mul( Input.vNormal, (float3x3)UniWorld );
	Output.vTexcoord = Input.vTexcoord;
	//Output.vInterpPos = Input.vPosition; // DirectX SDK のサンプルに存在したが、まったく使われていなかった。
	// CascadedShadowMaps11 も VarianceShadows11 も同様。
	Output.vDepth = mul( Input.vPosition, UniWorldView ).z;

	// Transform the shadow texture coordinates for all the cascades.
	Output.vTexShadow = mul( Input.vPosition, UniShadowViewMatrix );
	// TODO: ライトのビュー行列を乗算する前に、ワールド変換が必要なはず。
	// SDK サンプルではワールド変換は単位行列だったので、乗じていないだけらしい？

	return Output;
}
#endif


static const float4 CascadeColorMultipliers[MAX_SHADOW_CASCADES_NUM] =
{
	float4( 1.5f, 0.0f, 0.0f, 1.0f ),
	float4( 0.0f, 1.5f, 0.0f, 1.0f ),
	float4( 0.0f, 0.0f, 5.5f, 1.0f ),
	float4( 1.5f, 0.0f, 5.5f, 1.0f ),
	float4( 1.5f, 1.5f, 0.0f, 1.0f ),
	float4( 1.0f, 1.0f, 1.0f, 1.0f ),
	float4( 0.0f, 1.0f, 5.5f, 1.0f ),
	float4( 0.5f, 3.5f, 0.75f, 1.0f ),
};


void ComputeCoordinatesTransform(
	in int iCascadeIndex,
	//in float4 InterpolatedPosition, // DirectX SDK のサンプルに存在したが、まったく使われていなかった。
	// CascadedShadowMaps11 も VarianceShadows11 も同様。
	in out float4 vShadowTexCoord,
	in float4 vShadowTexCoordViewSpace)
{
	// Now that we know the correct map, we can transform the world space position of the current fragment
	if( SELECT_CASCADE_BY_INTERVAL_FLAG )
	{
		vShadowTexCoord = vShadowTexCoordViewSpace * m_vCascadeScale[iCascadeIndex];
		vShadowTexCoord += m_vCascadeOffset[iCascadeIndex];
	}
	vShadowTexCoord.w = vShadowTexCoord.z; // We put the z value in w so that we can index the texture array with Z.
	vShadowTexCoord.z = iCascadeIndex;

}

//--------------------------------------------------------------------------------------
// Use PCF to sample the depth map and return a percent lit value.
//--------------------------------------------------------------------------------------
float CalculateVarianceShadow(in float4 vShadowTexCoord, in float4 vShadowMapTextureCoordViewSpace, int iCascade)
{
	// vShadowTexCoord は Z にスライス インデックス、W に深度が格納されている。
#if 1
	// This loop could be unrolled, and texture immediate offsets could be used if the kernel size were fixed.
	// This would be a performance improvement.

	// In order to pull the derivative out of divergent flow control we calculate the
	// derivative off of the view space coordinates an then scale the deriviative.

	// オリジナルのコードでは ddx(), ddy(), SampleGrad() の戻り値や引数に関して warning X3206 が出ていたが、応急処置。
#if 1
	const float2 vShadowTexCoordDDX =
		ddx(vShadowMapTextureCoordViewSpace).xy * m_vCascadeScale[iCascade].xy;
	const float2 vShadowTexCoordDDY =
		ddy(vShadowMapTextureCoordViewSpace).xy * m_vCascadeScale[iCascade].xy;

	const float2 mapDepth = SrvCascadedShadowMapsTex.SampleGrad(SS_ShadowClamp,
		vShadowTexCoord.xyz,
		vShadowTexCoordDDX,
		vShadowTexCoordDDY);
#else
	float3 vShadowTexCoordDDX =
		ddx(vShadowMapTextureCoordViewSpace);
	vShadowTexCoordDDX *= m_vCascadeScale[iCascade].xyz;
	float3 vShadowTexCoordDDY =
		ddy(vShadowMapTextureCoordViewSpace);
	vShadowTexCoordDDY *= m_vCascadeScale[iCascade].xyz;

	const float2 mapDepth = SrvCascadedShadowMapsTex.SampleGrad(SS_ShadowClamp,
		vShadowTexCoord.xyz,
		vShadowTexCoordDDX,
		vShadowTexCoordDDY);
#endif
	// The sample instruction uses gradients for some filters.

	const float fAvgZ  = mapDepth.x; // Filtered z
	const float fAvgZ2 = mapDepth.y; // Filtered z-squared

	if ( vShadowTexCoord.w <= fAvgZ ) // We put the z value in w so that we can index the texture array with Z.
	{
		return 1;
	}
	else
	{
		const float MinVariance = 0.00001f;
		float variance = ( fAvgZ2 ) - ( fAvgZ * fAvgZ );
		//variance       = min( 1.0f, max( 0.0f, variance + 0.00001f ) );
		//variance = saturate(variance + MinVariance);
		variance = max(variance, MinVariance);

		const float mean     = fAvgZ;
		const float d        = vShadowTexCoord.w - mean; // We put the z value in w so that we can index the texture array with Z.
		const float p_max    = variance / ( variance + d * d );

		// To combat light-bleeding, experiment with raising p_max to some power
		// (Try values from 0.1 to 100.0, if you like.)
		//return pow(p_max, 4);
		return p_max;
	}
#else
	const float depthcompare = vShadowTexCoord.w;
	// A very simple solution to the depth bias problems of PCF is to use an offset.
	// Unfortunately, too much offset can lead to Peter-panning (shadows near the base of object disappear )
	// Too little offset can lead to shadow acne ( objects that should not be in shadow are partially self shadowed ).
	// Compare the transformed pixel depth to the depth read from the map.
	return depthcompare <= SrvCascadedShadowMapsTex.Sample(SS_ShadowClamp,
		vShadowTexCoord.xyz).r + UniShadowDepthBias;
#endif
}

//--------------------------------------------------------------------------------------
// Calculate amount to blend between two cascades and the band where blending will occur.
//--------------------------------------------------------------------------------------
void CalculateBlendAmountForInterval(
	in int iNextCascadeIndex,
	in out float fPixelDepth,
	out float fCurrentPixelsBlendBandLocation,
	out float fBlendBetweenCascadesAmount)
{
	float m_fCascadeFrustumsEyeSpaceDepths[] =
	{
		UniCascadeFrustumsEyeSpaceDepths0,
		UniCascadeFrustumsEyeSpaceDepths1,
		UniCascadeFrustumsEyeSpaceDepths2,
		UniCascadeFrustumsEyeSpaceDepths3,
		UniCascadeFrustumsEyeSpaceDepths4,
		UniCascadeFrustumsEyeSpaceDepths5,
		UniCascadeFrustumsEyeSpaceDepths6,
		UniCascadeFrustumsEyeSpaceDepths7,
	};

	// We need to calculate the band of the current shadow map where it will fade into the next cascade.
	// We can then early out of the expensive PCF for loop.
	//
	float fBlendInterval = m_fCascadeFrustumsEyeSpaceDepths[ iNextCascadeIndex - 1 ];
	if( iNextCascadeIndex > 1 )
	{
		fPixelDepth -= m_fCascadeFrustumsEyeSpaceDepths[ iNextCascadeIndex - 2 ];
		fBlendInterval -= m_fCascadeFrustumsEyeSpaceDepths[ iNextCascadeIndex - 2 ];
	}
	// The current pixel's blend band location will be used to determine when we need to blend and by how much.
	fCurrentPixelsBlendBandLocation = 1.0f - (fPixelDepth / fBlendInterval);
	// The fBlendBetweenCascadesAmount is our location in the blend band.
	fBlendBetweenCascadesAmount = fCurrentPixelsBlendBandLocation / m_fCascadeBlendArea;
}


//--------------------------------------------------------------------------------------
// Calculate amount to blend between two cascades and the band where blending will occur.
//--------------------------------------------------------------------------------------
void CalculateBlendAmountForMap(
	in float4 vShadowMapTextureCoord,
	out float fCurrentPixelsBlendBandLocation,
	out float fBlendBetweenCascadesAmount)
{
	// Calcaulte the blend band for the map based selection.
	const float2 distanceToOne = float2( 1.0f - vShadowMapTextureCoord.x, 1.0f - vShadowMapTextureCoord.y );
	fCurrentPixelsBlendBandLocation = min( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y );
	const float fCurrentPixelsBlendBandLocation2 = min( distanceToOne.x, distanceToOne.y );
	fCurrentPixelsBlendBandLocation =
		min( fCurrentPixelsBlendBandLocation, fCurrentPixelsBlendBandLocation2 );
	fBlendBetweenCascadesAmount = fCurrentPixelsBlendBandLocation / m_fCascadeBlendArea;
}

//--------------------------------------------------------------------------------------
// Calculate the shadow based on several options and render the scene.
//--------------------------------------------------------------------------------------
void CalcShadowLightPercent(float inWVDepth, float4 inTexCoordShadow, out int iCurrentCascadeIndex, out float fPercentLit)
{
	fPercentLit = 0.0f;
	//iCurrentCascadeIndex = 1;
	iCurrentCascadeIndex = 0;

	float4 vShadowMapTextureCoord = 0.0f;
	float4 vShadowMapTextureCoord_blend = 0.0f;

	int iNextCascadeIndex = 0;

	// This for loop is not necessary when the frustum is uniformly divided and interval based selection is used.
	// In this case fCurrentPixelDepth could be used as an array lookup into the correct frustum.
	const float4 vShadowMapTextureCoordViewSpace = inTexCoordShadow;

	if( SELECT_CASCADE_BY_INTERVAL_FLAG )
	{
		//iCurrentCascadeIndex = 0;
		if (CASCADE_COUNT_FLAG > 1)
		{
			const float4 vCurrentPixelDepth = inWVDepth;
#if 0
			const float4 fComparison0 = (vCurrentPixelDepth > m_fCascadeFrustumsEyeSpaceDepthsData[0]);
			const float4 fComparison1 = (vCurrentPixelDepth > m_fCascadeFrustumsEyeSpaceDepthsData[1]);
#else
			const float4 m_fCascadeFrustumsEyeSpaceDepthsData[2] =
			{
				float4(
				UniCascadeFrustumsEyeSpaceDepths0,
				UniCascadeFrustumsEyeSpaceDepths1,
				UniCascadeFrustumsEyeSpaceDepths2,
				UniCascadeFrustumsEyeSpaceDepths3),
				float4(
				UniCascadeFrustumsEyeSpaceDepths4,
				UniCascadeFrustumsEyeSpaceDepths5,
				UniCascadeFrustumsEyeSpaceDepths6,
				UniCascadeFrustumsEyeSpaceDepths7),
			};
			const float4 fComparison0 = (vCurrentPixelDepth > m_fCascadeFrustumsEyeSpaceDepthsData[0]);
			const float4 fComparison1 = (vCurrentPixelDepth > m_fCascadeFrustumsEyeSpaceDepthsData[1]);
#endif
			const float fIndex =
				dot(
				float4(
				CASCADE_COUNT_FLAG > 0,
				CASCADE_COUNT_FLAG > 1,
				CASCADE_COUNT_FLAG > 2,
				CASCADE_COUNT_FLAG > 3)
				, fComparison0 )
				+
				dot(
				float4(
				CASCADE_COUNT_FLAG > 4,
				CASCADE_COUNT_FLAG > 5,
				CASCADE_COUNT_FLAG > 6,
				CASCADE_COUNT_FLAG > 7)
				, fComparison1 );

			iCurrentCascadeIndex = (int)min(fIndex, CASCADE_COUNT_FLAG - 1);
		}
	}
	else
	{
		//iCurrentCascadeIndex = 0;
		if ( CASCADE_COUNT_FLAG == 1 )
		{
			vShadowMapTextureCoord = vShadowMapTextureCoordViewSpace * m_vCascadeScale[0];
			vShadowMapTextureCoord += m_vCascadeOffset[0];
		}
		else if ( CASCADE_COUNT_FLAG > 1 )
		{
			for( int iCascadeIndex = 0; iCascadeIndex < CASCADE_COUNT_FLAG; ++iCascadeIndex )
			{
				// ビュー変換後の座標を、カスケードごとに射影変換・テクスチャ座標変換する。
				vShadowMapTextureCoord = vShadowMapTextureCoordViewSpace * m_vCascadeScale[iCascadeIndex];
				vShadowMapTextureCoord += m_vCascadeOffset[iCascadeIndex];

				if (
					min( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y ) > m_fMinBorderPadding &&
					max( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y ) < m_fMaxBorderPadding
					)
				{
					iCurrentCascadeIndex = iCascadeIndex;
					break;
				}
			}
		}
	}

	// Found the correct map.

	ComputeCoordinatesTransform( iCurrentCascadeIndex, vShadowMapTextureCoord, vShadowMapTextureCoordViewSpace );

	if( BLEND_BETWEEN_CASCADE_LAYERS_FLAG && CASCADE_COUNT_FLAG > 1 )
	{
		// Repeat text coord calculations for the next cascade.
		// The next cascade index is used for blurring between maps.
		iNextCascadeIndex = min ( CASCADE_COUNT_FLAG - 1, iCurrentCascadeIndex + 1 );
		if( !SELECT_CASCADE_BY_INTERVAL_FLAG )
		{
			vShadowMapTextureCoord_blend = vShadowMapTextureCoordViewSpace * m_vCascadeScale[iNextCascadeIndex];
			vShadowMapTextureCoord_blend += m_vCascadeOffset[iNextCascadeIndex];
		}
		ComputeCoordinatesTransform( iNextCascadeIndex, vShadowMapTextureCoord_blend, vShadowMapTextureCoordViewSpace );
	}

	float fBlendBetweenCascadesAmount = 1.0f;
	float fCurrentPixelsBlendBandLocation = 1.0f;
	// The interval based selection technique compares the pixel's depth against the frustum's cascade divisions.
	float fCurrentPixelDepth = inWVDepth;

	if (BLEND_BETWEEN_CASCADE_LAYERS_FLAG && CASCADE_COUNT_FLAG > 1)
	{
		if( SELECT_CASCADE_BY_INTERVAL_FLAG )
		{
			CalculateBlendAmountForInterval( iNextCascadeIndex, fCurrentPixelDepth,
				fCurrentPixelsBlendBandLocation, fBlendBetweenCascadesAmount );
		}
		else
		{
			CalculateBlendAmountForMap( vShadowMapTextureCoord,
				fCurrentPixelsBlendBandLocation, fBlendBetweenCascadesAmount );
		}
	}

	// Because the Z coordinate specifies the texture array,
	// the derivative will be 0 when there is no divergence
	//float fDivergence = abs( ddy( vShadowMapTextureCoord.z ) ) + abs( ddx( vShadowMapTextureCoord.z ) );
	fPercentLit = CalculateVarianceShadow(vShadowMapTextureCoord, vShadowMapTextureCoordViewSpace,
		iCurrentCascadeIndex);

	// We repeat the calcuation for the next cascade layer, when blending between maps.
	if( BLEND_BETWEEN_CASCADE_LAYERS_FLAG && CASCADE_COUNT_FLAG > 1 )
	{
		if( fCurrentPixelsBlendBandLocation < m_fCascadeBlendArea )
		{
			// the current pixel is within the blend band.

			// Because the Z coordinate species the texture array,
			// the derivative will be 0 when there is no divergence
			//float fDivergence = abs( ddy( vShadowMapTextureCoord_blend.z ) ) + abs( ddx( vShadowMapTextureCoord_blend.z) );
			const float fPercentLit_blend = CalculateVarianceShadow(vShadowMapTextureCoord_blend, vShadowMapTextureCoordViewSpace,
				iNextCascadeIndex);

			// Blend the two calculated shadows by the blend amount.
			fPercentLit = lerp( fPercentLit_blend, fPercentLit, fBlendBetweenCascadesAmount );
		}
	}
}


#if 0
float4 psShadowCompileTest(VS_OUTPUT Input) : SV_Target
{
	int iCurrentCascadeIndex;
	float fPercentLit;
	CalcShadowLightPercent(Input.vDepth, Input.vTexShadow, iCurrentCascadeIndex, fPercentLit);

	const float3 vLightDir1 = float3( -1.0f, 1.0f, -1.0f );
	const float3 vLightDir2 = float3( +1.0f, 1.0f, -1.0f );
	const float3 vLightDir3 = float3( 0.0f, -1.0f, 0.0f );
	const float3 vLightDir4 = float3( 1.0f, 1.0f, 1.0f );
	// Some ambient-like lighting.
	float fLighting =
		saturate( dot( vLightDir1, Input.vNormal ) ) * 0.05f +
		saturate( dot( vLightDir2, Input.vNormal ) ) * 0.05f +
		saturate( dot( vLightDir3, Input.vNormal ) ) * 0.05f +
		saturate( dot( vLightDir4, Input.vNormal ) ) * 0.05f;

	const float vShadowLighting = fLighting * 0.5f;
	fLighting += saturate( dot(LightDir, Input.vNormal) );
	fLighting = lerp( vShadowLighting, fLighting, fPercentLit );

	const float4 vDiffuse = g_txDiffuse.Sample( g_samLinear, Input.vTexcoord );

	float4 vVisualizeCascadeColor;
	if (m_iVisualizeCascades)
	{
		vVisualizeCascadeColor = CascadeColorMultipliers[iCurrentCascadeIndex];
	}
	else
	{
		vVisualizeCascadeColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	const float4 outFinalColor = fLighting * vVisualizeCascadeColor * vDiffuse;
	return outFinalColor;
}
#endif
