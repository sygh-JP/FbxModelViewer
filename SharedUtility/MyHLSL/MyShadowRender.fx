// Only UTF-8 or ASCII is available.

#include "VSInOutStruct.hlsli"
#include "MyConstantBuffers.hlsli"
#include "Skinning.hlsli"

// 一般的なシャドウマップは、最も手前のサーフェイス（ポリゴン）のライト ビュー深度値を記録するだけなので、
// 現実世界でよくみられる「複数の物体の影が重なった部分は影の濃さが強くなる」という現象が再現できない。
// これを解決するために、ピクセルに深度以外の累積不透明度を記録しておき、影を描画する際に参照する 2.5D 手法がある。
// http://sidefx.jp/doc/rendering/deepshadowmaps.html
// バリアンス シャドウも 2.5D 手法と言えるが、半影（Penumbra）は表現できるものの、影の重なりには対応していない。
// 他にも「Opacity Shadow Maps」という名前のボリューム シャドウ技法に関する技術論文が発表されている。

// HLSL の SV_RenderTargetArrayIndex に相当する GLSL 組み込み変数は gl_Layer らしい。
// SV_ViewportArrayIndex に相当する組み込み変数は gl_ViewportIndex らしい。


struct GS_OUTPUT
{
	float4 pos : SV_Position;
	uint targetIndex : SV_RenderTargetArrayIndex; // Texture2DArray 中の指定スライスに描画するためのセマンティクス。
};

VS_OUTPUT_P vsShadowingVertex(in VS_INPUT_SKIN vsIn)
{
	VS_OUTPUT_P vsOut = (VS_OUTPUT_P)0;

	// TODO: 入力頂点にスキニングとワールド変換を行なう。
	// シャドウマップ作成時は頂点位置のみが必要で、
	// 法線は必要ないので、シーンのレンダリング時と比較して処理を減らせる。
	// その後ライト行列を使って（光源から見た）シャドウイング用の頂点として変換する。
	float4 skinnedPos;
	float3 skinnedNormal;
	DoMatrixSkinning(skinnedPos, skinnedNormal, vsIn);
	const float4 wpos = mul(skinnedPos, UniWorld);
	vsOut.pos = wpos;

	return vsOut;
}

// ジオメトリ シェーダーと MRT を使ってスキニング処理と描画キック回数を減らす。

// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html

[maxvertexcount(MAX_SHADOW_CASCADES_NUM * 3)]
void gsCreateShadowingVertices(triangle VS_OUTPUT_P gsIn[3], inout TriangleStream<GS_OUTPUT> triStream)
{
	// For each split to render
	for (int split = 0; split < UniAvailableCascadeCount; ++split)
	{
		GS_OUTPUT gsOut = (GS_OUTPUT)0;
		// Set render target index.
		gsOut.targetIndex = split;
#if 1
		// For each vertex of triangle
		for (int vertex = 0; vertex < 3; ++vertex)
		{
			// Transform vertex with split-specific crop matrix.
			gsOut.pos = mul(gsIn[vertex].pos, UniCascadedLightViewProj[split]);
			//gsOut.pos /= gsOut.pos.w;
			// Append vertex to stream
			triStream.Append(gsOut);
		}
#else
		// テクスチャ配列への描画テスト。
		const float step1 = 1.0 / 3;
		const float steph = 0.5 / 3;
		gsOut.pos = float4(steph * 0 + step1 * split, 0, 0, 1);
		triStream.Append(gsOut);
		gsOut.pos = float4(steph * 1 + step1 * split, 1, 0, 1);
		triStream.Append(gsOut);
		gsOut.pos = float4(steph * 2 + step1 * split, 0, 0, 1);
		triStream.Append(gsOut);
#endif
		// Mark end of triangle
		triStream.RestartStrip();
	}
}

//! @brief  偏微分（差分）を利用したモーメント計算。<br>
//! 
//! GPU Gems には「遮蔽物がライト方向と平行なとき、過剰な分散を避けるために偏微分をクランプすることは有益」とあるが、<br>
//! 「ハードウェア生成された偏微分は不安定で、ランダムなピクセルの点滅を生じうる」ともある。<br>
//! 要するに実際の必要性は疑問だが、シーンやライティングによってはこの処理が必要なのかもしれない。詳細は不明。<br>
float2 ComputeMoments(float depth)
{
	float2 moments;
	// First moment is the depth itself.
	moments.x = depth;
	// Compute partial derivatives of depth.
	const float dx = ddx(depth);
	const float dy = ddy(depth);
	// 偏微分（差分）を求める ddx(), ddy() を使うので、シェーダーモデル 3.0 以上が必要。
	// 実際は ps_2_x すなわち PS 2.0a / 2.0b でも使えるらしいが……
	// ちなみに OpenGL/GLSL の場合は dFdx(), dFdy() 関数が相当する。

	// Compute second moment over the pixel extents.
	moments.y = depth * depth + 0.25 * (dx * dx + dy * dy);
	return moments;
}

float2 psRenderVarianceShadowMap(GS_OUTPUT psIn) : SV_Target
{
	// 光源から見た深度と深度の2乗を保存する。
	const float z = psIn.pos.z / psIn.pos.w;
#if 0
	return float2(z, z * z);
#elif 1
	return ComputeMoments(z);
#else
	return float2(1, 1);
#endif
}

#include "MyRenderStates.hlsli"

technique11 TechShadowRender
{
	pass p0
	{
		// シャドウマップ描画用のレンダリング ステートを設定する。
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsShadowingVertex()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_4_0, gsCreateShadowingVertices()));
		SetPixelShader(CompileShader(ps_4_0, psRenderVarianceShadowMap()));
	}
}
