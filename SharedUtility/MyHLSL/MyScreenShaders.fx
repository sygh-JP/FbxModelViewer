// Only UTF-8 or ASCII is available.

#include "VSInOutStruct.hlsli"
#include "GaussianWeights.hlsli"
#include "CommonConst.hlsli"

#include "MyConstantBuffers.hlsli"
#include "MyTexSamplers.hlsli"


#define MY_MOVING_AVERAGE_FILTER_HORIZONTAL_FUNC_NAME   MovingAverageFilterHorizontalFunc3
#define MY_MOVING_AVERAGE_FILTER_VERTICAL_FUNC_NAME     MovingAverageFilterVerticalFunc3
#define MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT        3
#include "MovingAverageFilter.hlsli"

// テクスチャ配列の各スライスにまとめてレンダリングするには、MRT だけでなくジオメトリ シェーダーも使う必要がある模様。
// コンピュート シェーダーを使うと劇的に楽になる。
// ただしカーネル サイズが小さいとき（3x3程度）はピクセル シェーダーのほうが明らかに高速で、
// コンピュート シェーダーのほうが逆に遅くなる模様。
// また、ピクセル シェーダーはグループ共有メモリのサイズ上限や、ローカル スレッド グループ サイズの上限を考えなくともよい。
// 19x19のガウスぼかしくらいまでいくとコンピュート シェーダーのほうが有利になると思われるが、
// そこまで有意な差は見られない模様。
// もしかしたら NVIDIA と AMD とでは異なるかも。


struct GS_OUTPUT
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	uint targetIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(MAX_SHADOW_CASCADES_NUM * 3)]
void gsCreateShadowBlurTargetVertices(triangle VS_OUTPUT_PCT gsIn[3], inout TriangleStream<GS_OUTPUT> triStream)
{
	for (int split = 0; split < UniAvailableCascadeCount; ++split)
	{
		GS_OUTPUT gsOut = (GS_OUTPUT)0;
		gsOut.targetIndex = split;
		for (int vertex = 0; vertex < 3; ++vertex)
		{
			gsOut.pos = gsIn[vertex].pos;
			gsOut.color = gsIn[vertex].color;
			gsOut.tex = gsIn[vertex].tex;
			triStream.Append(gsOut);
		}
		triStream.RestartStrip();
	}
}

float2 psApplyMovingAverageFilter3ToShadowTexHorz(in GS_OUTPUT psIn) : SV_Target
{
	const float3 texCoord = float3(psIn.tex, psIn.targetIndex);
	return MovingAverageFilterHorizontalFunc3(SHADOW_MAP_TEXTURE_SIZE, texCoord, SS_LinearClamp, SrvCascadedShadowMapsTex);
}

float2 psApplyMovingAverageFilter3ToShadowTexVert(in GS_OUTPUT psIn) : SV_Target
{
	const float3 texCoord = float3(psIn.tex, psIn.targetIndex);
	return MovingAverageFilterVerticalFunc3(SHADOW_MAP_TEXTURE_SIZE, texCoord, SS_LinearClamp, SrvCascadedShadowMapsTex);
}


// カーネルサイズは 9 * 2 + 1 = 19 になる。
#define MY_GAUSSIAN_BLUR_HORIZONTAL_FUNC_NAME   GaussianBlurHorizontalFunc10
#define MY_GAUSSIAN_BLUR_VERTICAL_FUNC_NAME     GaussianBlurVerticalFunc10
#define MY_GAUSSIAN_WEIGHTS_ARRAY_NAME          GaussianWeights10
#define MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT    10
#include "GaussianBlurFunc.hlsli"

#if 0
// カーネルサイズは 7 * 2 + 1 = 15 になる。
#define MY_GAUSSIAN_BLUR_HORIZONTAL_FUNC_NAME   GaussianBlurHorizontalFunc8
#define MY_GAUSSIAN_BLUR_VERTICAL_FUNC_NAME     GaussianBlurVerticalFunc8
#define MY_GAUSSIAN_WEIGHTS_ARRAY_NAME          GaussianWeights8
#define MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT    8
#include "GaussianBlurFunc.hlsli"
#endif

#if 0
// カーネルサイズは 5 * 2 + 1 = 11 になる。
#define MY_GAUSSIAN_BLUR_HORIZONTAL_FUNC_NAME   GaussianBlurHorizontalFunc6
#define MY_GAUSSIAN_BLUR_VERTICAL_FUNC_NAME     GaussianBlurVerticalFunc6
#define MY_GAUSSIAN_WEIGHTS_ARRAY_NAME          GaussianWeights6
#define MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT    6
#include "GaussianBlurFunc.hlsli"
#endif

float4 psApplyGaussianBlur19ToDownSampledTexHorz(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	// 水平方向に重み付きマルチ サンプリングして適当なカーネル サイズのガウスぼかしをかける。

	//return float4(GaussianBlurHorizontalFunc8(DOWN_SAMPLED_TEX_SIZE, vsOut.tex, SS_LinearClamp, DownSampledTex).rgb, 1);
	return float4(GaussianBlurHorizontalFunc10(DOWN_SAMPLED_TEX_SIZE, vsOut.tex, SS_LinearClamp, DownSampledTex).rgb, 1);
}


float4 psApplyGaussianBlur19ToDownSampledTexVert(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	// 垂直方向に重み付きマルチ サンプリングして適当なカーネル サイズのガウスぼかしをかける。

	//return float4(GaussianBlurVerticalFunc8(DOWN_SAMPLED_TEX_SIZE, vsOut.tex, SS_LinearClamp, DownSampledTex).rgb, 1);
	return float4(GaussianBlurVerticalFunc10(DOWN_SAMPLED_TEX_SIZE, vsOut.tex, SS_LinearClamp, DownSampledTex).rgb, 1);
}


#define MY_MSAA_DOWN_SAMPLE_FUNC_NAME  TransportFromMSAA8Tex
#define MY_SM41_MSAA_DOWN_SAMPLE_FUNC_NAME SM41TransportFromMSAA8Tex
#define MY_MSAA_ALG_TEX_SAMPLE_COUNT   8
#include "MsaaAlgTemplates.hlsli"
#define MY_MSAA_DOWN_SAMPLE_FUNC_NAME  TransportFromMSAA4Tex
#define MY_SM41_MSAA_DOWN_SAMPLE_FUNC_NAME SM41TransportFromMSAA4Tex
#define MY_MSAA_ALG_TEX_SAMPLE_COUNT   4
#include "MsaaAlgTemplates.hlsli"
#define MY_MSAA_DOWN_SAMPLE_FUNC_NAME  TransportFromMSAA2Tex
#define MY_SM41_MSAA_DOWN_SAMPLE_FUNC_NAME SM41TransportFromMSAA2Tex
#define MY_MSAA_ALG_TEX_SAMPLE_COUNT   2
#include "MsaaAlgTemplates.hlsli"


// MSAA バッファ転送のピクセル シェーダー。
// 
// D3D10/D3D11 ではデバイスによる StretchRect() が廃止されており、さらに代替機能の
// D3DX10LoadTextureFromTexture(), D3DX11LoadTextureFromTexture() では MSAA テクスチャからの転送に失敗してしまうので、
// 自前で MSAA のダウンサンプル処理コードを記述する必要がある模様。
// AMD が GDC 2007 で MSAA ダウンサンプル時にトーン マッピングやガンマ補正を同時に行なうコード例を紹介している。
// http://developer.amd.com/assets/Riguer-DX10_tips_and_tricks_for_print.pdf
// 2014 年現在、スライドのアーカイブは下記に移動されている模様。
// しかし AMD は勝手に資料の URL を変えるわ、以前の URL に対するリダイレクト サービスもやってないわで困る……
// http://developer.amd.com/wordpress/media/2012/10/Riguer-DX10_tips_and_tricks_for_print.pdf
// ただし、フォーマットや次元（サイズ）に互換性がある場合は、転送するだけであれば、
// ID3D10Device::ResolveSubresource()
// ID3D11DeviceContext::ResolveSubresource()
// を使う方法が楽。
// 非 MSAA 同士もしくは同レベルの MSAA 同士であれば、CopyResource() が使える。
// MSAA 浮動小数テクスチャから非 MSAA 整数テクスチャへダウンサンプルする場合は、シェーダーを使うしかなさげ。
// なお、もしフォーマット変換・サイズ変換を行なう場合、コンピュート シェーダーをうまく使えば、
// 従来の頂点シェーダー＋ピクセル シェーダーよりも高速に実行できる可能性がある。
// トーンマッピングも併せてコンピュート シェーダーで行なうと効果的。
// HACK: 頂点シェーダー出力の頂点カラー値を考慮（合成）する。
// フェードイン・フェードアウトやホワイトイン・ホワイトアウトなどのエフェクトはこのタイミングで実行するとよいかも。

float4 psTransportFromMSAA8Tex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	return TransportFromMSAA8Tex(UniScreenSize, vsOut.tex, TransportSrcTexMSAA8);
}

float4 psTransportFromMSAA4Tex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	return TransportFromMSAA4Tex(UniScreenSize, vsOut.tex, TransportSrcTexMSAA4);
}

float4 psTransportFromMSAA2Tex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	return TransportFromMSAA2Tex(UniScreenSize, vsOut.tex, TransportSrcTexMSAA2);
}

float4 psTransportFromNonMSAATex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	return TransportFromNonMSAATex(UniScreenSize, vsOut.tex, TransportSrcTexNonMSAA);
	//return TransportFromMSAA8Tex(UniScreenSize, vsOut.tex, SrvNormalDepthTexNonMSAA); // 法線・深度マップのテスト表示。
	//return TransportFromMSAA8Tex(UniScreenSize, vsOut.tex, SrvFurGeoMapTexNonMSAA); // ファージオメトリ マップのテスト表示。
}

// 高輝度抽出の閾値。通例、トーン マッピングの結果に左右される。
// HACK: トーン マッピングは GPU で実行するが、CPU からのリードバックなしにそのまま活用する。
// つまり、ピクセル シェーダーやコンピュート シェーダーでテクスチャに書き込んだ結果を SRV 経由で読み出す。
// 定数バッファは使わない。
static const float HighIntensityExtractionThreshold = 0.9f;

float4 ExtractHighIntensity(float4 color)
{
	// HACK: 完全透明色を返すようにしたほうがいいかも。
	// discard はアルファテスト相当機能だが、GPU によっては失速の原因になることがあるらしい（特にモバイル）。
	// 「アルファテストに失敗したら深度値を書き込まない」という場合は discard が必須だが……
	if (dot(color.rgb, RgbToYFactor3F) < HighIntensityExtractionThreshold)
	{
		discard; // ピクセルを破棄。
	}
	return color;
}

// MSAA バッファ転送のピクセル シェーダー（高輝度抽出版）。
float4 psExtractHighIntensityFromMSAA8Tex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	const float4 color = TransportFromMSAA8Tex(UniScreenSize, vsOut.tex, TransportSrcTexMSAA8);
	return ExtractHighIntensity(color);
}

float4 psExtractHighIntensityFromMSAA4Tex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	const float4 color = TransportFromMSAA4Tex(UniScreenSize, vsOut.tex, TransportSrcTexMSAA4);
	return ExtractHighIntensity(color);
}

float4 psExtractHighIntensityFromMSAA2Tex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	const float4 color = TransportFromMSAA2Tex(UniScreenSize, vsOut.tex, TransportSrcTexMSAA2);
	return ExtractHighIntensity(color);
}

float4 psExtractHighIntensityFromNonMSAATex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	const float4 color = TransportFromNonMSAATex(UniScreenSize, vsOut.tex, TransportSrcTexNonMSAA);
	return ExtractHighIntensity(color);
}

// 1回のサンプリング数を減らしてピクセル シェーダーのトータル起動回数を増やしたほうがいいのか、それとも
// 1回のサンプリング数を増やしてピクセル シェーダーのトータル起動回数を減らしたほうがいいのか……
// → 少なくとも NVIDIA Fermi 世代では、256ピクセル四方の場合は1回のサンプリング数が2x2よりも4x4のほうがかなり高速になる。
// 8x8サンプルと組み合わせるとさらに高速化する模様。
// しかし、それでもコンピュート シェーダー 5.0 によって最適化された並列リダクションの速度には敵わない模様。
// AMD 環境での検証もしたほうがよいかも（さらに差が開く可能性もある）。

// 一辺が 2^N サイズの画像を 1x1 サイズに縮小するには、N 回の適用が必要となる。
float4 psReduction2x2(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
#if 0
	float result = dot(UniReductionInputTex2D.Sample(SS_PointBorderTransparent, vsOut.tex), RgbToYFactor4F);
	const int2 offsets[4] =
	{
		int2(0, 0),
		int2(1, 0),
		int2(0, 1),
		int2(1, 1),
	};
	[unroll]
	for (int i = 1; i < 4; ++i)
	{
		result += dot(UniReductionInputTex2D.Sample(SS_PointBorderTransparent, vsOut.tex, offsets[i]), RgbToYFactor4F);
	}
#else
	float result = 0;
	const int kernelSize = 2;
	[unroll]
	for (int i = 0; i < kernelSize; ++i)
	{
		[unroll]
		for (int j = 0; j < kernelSize; ++j)
		{
			result += dot(UniReductionInputTex2D.Sample(SS_PointBorderTransparent, vsOut.tex, int2(j, i)), RgbToYFactor4F);
		}
	}
#endif
	return result;
}

// 一辺が 4^N サイズの画像を 1x1 サイズに縮小するには、N 回の適用が必要となる。
float4 psReduction4x4(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	float result = 0;
	const int kernelSize = 4;
	[unroll]
	for (int i = 0; i < kernelSize; ++i)
	{
		[unroll]
		for (int j = 0; j < kernelSize; ++j)
		{
			result += dot(UniReductionInputTex2D.Sample(SS_PointBorderTransparent, vsOut.tex, int2(j, i)), RgbToYFactor4F);
		}
	}
	return result;
}

// 一辺が 8^N サイズの画像を 1x1 サイズに縮小するには、N 回の適用が必要となる。
float4 psReduction8x8(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	float result = 0;
	const int kernelSize = 8;
	[unroll]
	for (int i = 0; i < kernelSize; ++i)
	{
		[unroll]
		for (int j = 0; j < kernelSize; ++j)
		{
			result += dot(UniReductionInputTex2D.Sample(SS_PointBorderTransparent, vsOut.tex, int2(j, i)), RgbToYFactor4F);
		}
	}
	return result;
}

/////////////////////////////////////////////////////////////////////

#include "MyRenderStates.hlsli"

technique11 TechApplyHorizontalMovingAverageFilterToShadowTex
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_4_0, gsCreateShadowBlurTargetVertices()));
		SetPixelShader(CompileShader(ps_4_0, psApplyMovingAverageFilter3ToShadowTexHorz()));
	}
}

technique11 TechApplyVerticalMovingAverageFilterToShadowTex
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_4_0, gsCreateShadowBlurTargetVertices()));
		SetPixelShader(CompileShader(ps_4_0, psApplyMovingAverageFilter3ToShadowTexVert()));
	}
}

// ダウンサンプル テクスチャを水平方向にぼかす。
technique11 TechApplyHorizontalGaussianBlurToDownSampledTex
{
	// ワーク用一時レンダーターゲットの作成や切替などが激しくめんどいので、1パスで実行できるガウスぼかしにできないか？
	// ……と思ったが、1パスで水平・垂直ぼかしをまとめてやるよりも、
	// 水平方向と垂直方向を分離したほうがテクスチャフェッチのトータル回数は減る。
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_SrcAlphaBlendingAdd, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);
		//SetRasterizerState(RS_Solid);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psApplyGaussianBlur19ToDownSampledTexHorz()));
	}
}

// ダウンサンプル テクスチャを垂直方向にぼかす。
technique11 TechApplyVerticalGaussianBlurToDownSampledTex
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_SrcAlphaBlendingAdd, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);
		//SetRasterizerState(RS_Solid);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psApplyGaussianBlur19ToDownSampledTexVert()));
	}
}

// MSAA テクスチャから非 MSAA バック バッファへのダウンサンプル転送。
technique11 TechTransportFromMSAA8
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psTransportFromMSAA8Tex()));
	}
}

technique11 TechTransportFromMSAA4
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psTransportFromMSAA4Tex()));
	}
}

technique11 TechTransportFromMSAA2
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psTransportFromMSAA2Tex()));
	}
}

technique11 TechTransportFromNonMSAA
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psTransportFromNonMSAATex()));
	}
}


// MSAA テクスチャから非 MSAA バック バッファへのダウンサンプル転送（高輝度抽出版）。
technique11 TechExtractHighIntensityFromMSAA8
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psExtractHighIntensityFromMSAA8Tex()));
	}
}

technique11 TechExtractHighIntensityFromMSAA4
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psExtractHighIntensityFromMSAA4Tex()));
	}
}

technique11 TechExtractHighIntensityFromMSAA2
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psExtractHighIntensityFromMSAA2Tex()));
	}
}

technique11 TechExtractHighIntensityFromNonMSAA
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psExtractHighIntensityFromNonMSAATex()));
	}
}

technique11 TechReduction2x2
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psReduction2x2()));
	}
}

technique11 TechReduction4x4
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psReduction4x4()));
	}
}

technique11 TechReduction8x8
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psReduction8x8()));
	}
}
