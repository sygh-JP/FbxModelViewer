// Only UTF-8 or ASCII is available.

/////////////////////////////////////////////////////////////////////
// 使用可能なテクスチャ スロットの最大数は、「Direct3D feature levels」の「Max Input Slots」の項目が相当するのか？
// http://msdn.microsoft.com/en-us/library/windows/apps/ff476876.aspx

Texture2D<float4> SrvDiffuseTex : register(t0);

Texture2D<float4> SrvToonShadingGradRefTex : register(t1);
TextureCube<float4> SrvEnvMapTex : register(t2);
Texture2DArray<float2> SrvCascadedShadowMapsTex : register(t3);

Texture2D<float4> SrvAlphaTex : register(t2);

Texture2DMS<float4, 8> TransportSrcTexMSAA8 : register(t0);
Texture2DMS<float4, 4> TransportSrcTexMSAA4 : register(t0);
Texture2DMS<float4, 2> TransportSrcTexMSAA2 : register(t0);
Texture2D<float4> TransportSrcTexNonMSAA : register(t0);

//Texture2DMS<float4, 8> SrvNormalDepthTexMSAA8 : register(t4);
//Texture2DMS<float4, 4> SrvNormalDepthTexMSAA4 : register(t4);
//Texture2DMS<float4, 2> SrvNormalDepthTexMSAA2 : register(t4);
//Texture2D<float4> SrvNormalDepthTexNonMSAA : register(t4);

//Texture2DMS<float4, 8> SrvFurGeoMapTexMSAA8 : register(t4);
//Texture2DMS<float4, 4> SrvFurGeoMapTexMSAA4 : register(t4);
//Texture2DMS<float4, 2> SrvFurGeoMapTexMSAA2 : register(t4);
//Texture2D<float4> SrvFurGeoMapTexNonMSAA : register(t4);

Texture2D<float4> NormalDepthTex : register(t4);

Texture2D<float4> FurGeoMapTex : register(t4);

Texture2D<float4> DownSampledTex : register(t0);

StructuredBuffer<float4> UniWaveSimResultBuffer : register(t0);
Texture2D<float4> UniWaveSimResultTex : register(t0);

Texture2D<float> SrvWaveSimMaskTex : register(t2);
Buffer<int2> SrvWaveFrontPlaneGridBuffer : register(t3);
StructuredBuffer<uint4> SrvRandomNumTable : register(t4);


// Input0 = PrePre
// Input1 = Pre
// Output = Current
// 構造化バッファを使えば、Input0 と Output を RW にまとめることができるが、破壊的操作になるので、演算を実行すれば必ずフレームを進めることになる。
StructuredBuffer<float4> UniWaveSimInputBuffer0 : register(t0);
StructuredBuffer<float4> UniWaveSimInputBuffer1 : register(t1);
RWStructuredBuffer<float4> UniWaveSimOutputBuffer : register(u0);

Texture2D<float4> UniWaveSimInputTex0 : register(t0);
Texture2D<float4> UniWaveSimInputTex1 : register(t1);
RWTexture2D<float4> UniWaveSimOutputTex : register(u0);


Texture2D<float4> UniReductionInputTex2D : register(t0);
Texture1D<float> UniReductionInputTex1D : register(t0);
RWTexture1D<float> UniReductionOutputTex1D : register(u0);

RWTexture2D<float4> UniRWGaussianBlurOutTex : register(u0);
RWTexture2DArray<float2> UniRWShadowSmoothingOutTex : register(u0);
RWStructuredBuffer<uint4> UniRWRandomNumTable : register(u0);

struct MyFlakeVertex
{
	float3 Position;
	float4 Attitude;
};

StructuredBuffer<MyFlakeVertex> SrvFlakeParticleBuffer : register(t0);
RWStructuredBuffer<MyFlakeVertex> UavRWFlakeParticleBuffer : register(u0);

/////////////////////////////////////////////////////////////////////

// リフレクションを切った Compact Effects11 では、シェーダー側でサンプラーステート構文によって定義することができない。
// C++ 側で作成して、必要なシェーダーステージに対して明示的にスロット指定してバインドする必要がある。

#if 0
SamplerState SS_Test
{
	Filter = MIN_MAG_MIP_POINT;
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	AddressU = CLAMP;
	AddressV = CLAMP;
};
#endif

SamplerState SS_PointWrap : register(s0);

SamplerState SS_LinearWrap : register(s1);

SamplerState SS_LinearClamp : register(s2);

SamplerState SS_ShadowClamp : register(s3); // AnisoClamp or LinearClamp

SamplerState SS_PointBorderTransparent : register(s4);
