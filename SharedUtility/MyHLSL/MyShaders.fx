// Only UTF-8 or ASCII is available.

#include "VSInOutStruct.hlsli"
#include "CommonConst.hlsli"

#include "MyConstantBuffers.hlsli"
#include "MyTexSamplers.hlsli"

#include "Skinning.hlsli"
#include "ShadowSampling.hlsli"

#include "DiffuseSpecularFunc.hlsli"

/////////////////////////////////////////////////////////////////////


[maxvertexcount(2)]
void gsCreateLightDirLine(point VS_OUTPUT_P gsIn[1], inout LineStream<VS_OUTPUT_PC> outStream)
{
	VS_OUTPUT_PC gsOut = (VS_OUTPUT_PC)0;
	gsOut.color = float4(1, 1, 0, 1);
	// ライトにはワールド変換は不要。
	gsOut.pos = mul(gsIn[0].pos, UniViewProj);
	outStream.Append(gsOut);
	gsOut.pos = mul(float4(LightPos, 1), UniViewProj);
	outStream.Append(gsOut);
	outStream.RestartStrip();
}

// シェーディングなし、座標変換のみ行なう頂点シェーダー。
VS_OUTPUT_PC vsShadingLessPCtoPC(in VS_INPUT_PC vsIn)
{
	VS_OUTPUT_PC vsOut = (VS_OUTPUT_PC)0;

	vsOut.pos = mul(vsIn.pos, UniWorldViewProj);
	vsOut.color = vsIn.color;

	return vsOut;
}

// シェーディングなし、座標変換のみ行なう頂点シェーダー。
VS_OUTPUT_PCT vsShadingLessPCTtoPCT(in VS_INPUT_PCT vsIn)
{
	VS_OUTPUT_PCT vsOut = (VS_OUTPUT_PCT)0;

	vsOut.pos = mul(vsIn.pos, UniWorldViewProj);
	vsOut.color = vsIn.color;
	vsOut.tex = vsIn.tex;

	return vsOut;
}

#if 0
// シェーディングなし、座標変換のみ行なう頂点シェーダー。
VS_OUTPUT_PCT vsShadingLessPCNTtoPCT(in VS_INPUT_PCNT vsIn)
{
	VS_OUTPUT_PCT vsOut = (VS_OUTPUT_PCT)0;

	vsOut.pos = mul(vsIn.pos, UniWorldViewProj);
	vsOut.color = vsIn.color;
	vsOut.tex = vsIn.tex;

	return vsOut;
}
#endif

// ランバート シェーディングの頂点シェーダー。
VS_OUTPUT_PCT vsLambertPCNTtoPCT(in VS_INPUT_PCNT vsIn)
{
	VS_OUTPUT_PCT vsOut = (VS_OUTPUT_PCT)0;

	// 頂点位置をトランスフォームする。
	vsOut.pos = mul(vsIn.pos, UniWorldViewProj);

	// 頂点法線をトランスフォームする。
	const float3 wnormal = normalize(mul(vsIn.normal, (float3x3)UniWorld));
	const float4 wpos = mul(vsIn.pos, UniWorld);
	const float3 light = normalize(LightPos - wpos.xyz / wpos.w);

	// 頂点ライティング（ディフューズのみ）。
#if 1
	// 頂点カラーを使ってライティングをするなら、こっちのコード。
	vsOut.color = vsIn.color * LightColor * CalcLambertDiffuse(wnormal, light);
#else
	vsOut.color = MaterialColorDiffuse * LightColor * CalcLambertDiffuse(wnormal, light);
#endif

	return vsOut;
}

// 頂点カラーだけを処理するシンプルなピクセル シェーダー。
float4 psSimplePCT(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	return vsOut.color;
}

float4 psSimplePC(in VS_OUTPUT_PC vsOut) : SV_Target0
{
	return vsOut.color;
}


float4 psSkinningDummyNoIF(in VS_OUTPUT_WORLD_PNT psIn) : SV_Target0
{
	// Dynamic Shader Linkage（interface）を使わないダミーバージョン。
	// エフェクト パスの Apply() でのエラーメッセージ出力を防止するため。
	return (float4)0;
}


void LegacyToon_deprecated(float3 eye, float3 wnormal, inout float4 inoutColor)
{
	// 視線と法線がなす角度が90度（直交）に近づくにつれてピクセルを黒っぽくする。
	// かなり昔からある輪郭線描画の古典的手法らしいが、アルゴリズムが単純すぎて輪郭線のクオリティが低い。
	// 特に広い平面に対する輪郭線がきれいに出ない。
	// 面が視線と平行になるにつれてひどくなる。却下。
	const float eyedot = dot(eye, wnormal);
	inoutColor.rgb *= step(1.0e-4f, eyedot); // 輪郭線を描く。
}

float4 psToonInk(in VS_OUTPUT_WORLD_PNT psIn) : SV_Target0
{
	// HACK: 輪郭線が常に黒だと「くどい」印象になるので、インクの色をマテリアルによって決定する。いわゆる色トレース。
	return float4(0,0,0,1);
}

// TODO: トゥーン インクは反転拡大した裏面ポリゴンを描画する方法ではなく、
// 画面座標系でのエッジ検出（法線・深度マップからの輪郭抽出と、マテリアル マップからの輪郭抽出の組み合わせ）を使って、
// ポスト プロセス系のフィルターで実装したほうがよさげかも。
// MRT を使えばスキン メッシュのトラバースとスキニングなどのジオメトリ処理は1回で済むので、API コールの減少や帯域の節約になる。
// その代わり、バッファに使用するテクスチャ メモリ量は増大する（特に MSAA を実行する場合）。
// なお、スキニングの結果を保持して使いまわしたいのであれば、ストリーム アウトプットを使う方法もある（OpenGL でいうトランスフォーム フィードバック）。
// ストリーム アウトプットは OMSetRenderTargets() の代わりに SOSetTargets() を使い、
// ピクセル シェーダーの代わりにジオメトリ シェーダーや頂点シェーダーを使うと考えれば分かりやすい。
// ストリーム アウトプット対応の GS 作成には CreateGeometryShader() の代わりに CreateGeometryShaderWithStreamOutput() を使うが、
// Effects10/11 では CompileShader() の代わりに ConstructGSWithSO() を使えばよいらしい。
// なお、D3D10_SO_DECLARATION_ENTRY / D3D11_SO_DECLARATION_ENTRY は OpenGL の glTransformFeedbackVaryings() に相当するものと思われる。

// http://xbox.create.msdn.com/ja-JP/education/catalog/sample/nonrealistic_rendering
// http://wlog.flatlib.jp/archive/1/2007-7-30

// http://codeoncanvas.blogspot.jp/2010/02/xnafps20.html

// エッジの幅とエッジの強さ（暗さ）。
static const float EdgeWidth = 1;
static const float EdgeIntensity = 1;

// 法線の変化量と深度の変化量のための閾値です。
// あまり小さな変化量は無視するために使います。
static const float NormalThreshold = 0.5;
static const float DepthThreshold = 0.1;

// 閾値を超えた、急激に変化している思われる法線と深度に掛け合わせて、
// その変化量を大きくするためのいわば重み（拡大係数）です。
static const float NormalSensitivity = 1;
static const float DepthSensitivity = 10;

float4 EdgeDetectColorSketch(float2 texCoord)
{
	// 画面の解像度からエッジのオフセットを算出。
	const float2 edgeOffset = EdgeWidth / float2(UniScreenSize.x, UniScreenSize.y);

	// 注目画素に隣接する左上、右下、左下、右上の画素の法線・深度を読み取る。
	// おそらく 2x2 Roberts Gradient フィルタの変形版？
	const float4 n1 = NormalDepthTex.Sample(SS_LinearClamp, texCoord + float2(-1, -1) * edgeOffset);
	const float4 n2 = NormalDepthTex.Sample(SS_LinearClamp, texCoord + float2( 1,  1) * edgeOffset);
	const float4 n3 = NormalDepthTex.Sample(SS_LinearClamp, texCoord + float2(-1,  1) * edgeOffset);
	const float4 n4 = NormalDepthTex.Sample(SS_LinearClamp, texCoord + float2( 1, -1) * edgeOffset);

	// 法線と深度の左上と右下の差、左下と右上の差を足したものを傾きとして格納。
	const float4 diagonalDelta = abs(n1 - n2) + abs(n3 - n4);

	// 傾きから法線の変化量と深度の変化量をそれぞれ格納。
	float normalDelta = dot(diagonalDelta.xyz, 1);
	float depthDelta = diagonalDelta.w;

	// 閾値以下の変化を無視し、閾値を超えたものに一定の強度を乗算したものを変化量とする。
	// これにより綺麗にできるようです。
	normalDelta = saturate((normalDelta - NormalThreshold) * NormalSensitivity);
	depthDelta = saturate((depthDelta - DepthThreshold) * DepthSensitivity);

	// 対象のピクセルはどれぐらいエッジに乗っているかを法線と深度の変化量より調べます。
	// エッジの中心ほど法線と深度の変化量は大きくなるため、
	// エッジの中心ほどこの値は大きくなることになる。
	const float edgeAmount = saturate(normalDelta + depthDelta) * EdgeIntensity;

	// シーンと乗算合成する輪郭線の色とアルファ値を出力する。
	// 輪郭線の色は地の色もしくはマテリアルを考慮したほうがいいかもしれないが、
	// その場合2つのマテリアル境界にまたがる線の色はどうする？
	//const float outCoef = (1 - edgeAmount);
	return float4(0, 0, 0, edgeAmount);
	//return float4(NormalDepthTex.Sample(SS_LinearClamp, texCoord));
	//return float4(NormalDepthTex.Sample(SS_LinearClamp, texCoord).rgb, 1);

	// シーンのテクスチャの対象ピクセルをエッジの量だけ暗くする。
	//return float4(sceneColor.rgb * (1 - edgeAmount), sceneColor.a);
}

float4 psEdgeDetectColorSketch(in VS_OUTPUT_PCT psIn) : SV_Target0
{
	return EdgeDetectColorSketch(psIn.tex);
}

// 上記の法線・深度を基準にするピクセル シェーダーベースのポスト エフェクト手法は、
// 処理時間が頂点数ではなくピクセル解像度依存で予測しやすく、板ポリにも適用できるのがメリットだが、
// 入り組んだ部分がヘタな墨入れのように汚なくなってしまう。とうていキャラクターに適用できるレベルではない。
// 特にカメラをズームアウトして、縮小表示した際のクオリティに問題がある。
// 深度値に応じて視点から遠くにある物体の輪郭線は細くするか、描画しないようにするべき。
// SSAO に近い問題点がある。
// 閾値の動的な調整が必要かも。

// 他にも、ジオメトリ シェーダーと隣接情報を活用したライン プリミティブによる輪郭線描画方法もある。
// D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ と、隣接情報付きライン リスト用に作成したインデックス バッファを使ってラインを描画している模様。
// http://zerogram.info/?p=941
// ただし、この方法だとおそらく非共有の独立辺を持つポリゴン（例えば板ポリなど）の輪郭線は描画できないはず。
// 「共有された辺で、接続された2つの三角形面の法線が同一方向ならばラインを描画しない（そうでない場合は辺を描画する）」というアルゴリズムのほうがよさげ。
// あるいは独立辺だけを別途描画するためのインデックス バッファを作る。
// →そんなことはなかった。独立辺の隣接インデックスは反対側の頂点を使うようにすればよい。
// 隣接インデックス情報の作成を適切に行なえば、D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ を使うこともできる。
// なお、輪郭線が単調だと面白みに欠けるので、カメラからの距離などに応じて、線に強弱（入り抜き）を付けるようにしたい。
// カプコンの大神シリーズみたいな感じ。

// キャラクターにオーソドックスな裏面拡大描画方法の拡張版を使い、背景にポスト エフェクト手法を使っている作品もある。
// http://game.watch.impress.co.jp/docs/series/3dcg/20130213_586956.html

#if 0
// 左上を原点とする (0 ～ W, 0 ～ H) のスクリーン座標を、(-1 ～ +1, +1 ～ -1) の正規化座標に変換する。
// Ortho2D 変換に相当。
// Direct3D 9 までのバージョンでは、スプライトなどの描画時にピクセル スナップする際は
// ラスター化ルールに応じてスクリーン位置座標（D3DFVF_XYZRHW）に半ピクセルの下駄を履かせる必要があった。
// Direct3D 10 以降はラスター化ルールが変更されていて、OpenGL や GDI と同じく、下駄を履かせる必要はなくなっているらしい。
// OpenGL のラスター化とは微妙に規則が異なる（0.5 などの非整数を指定したときのラスタライズ結果が微妙に異なる）が、
// HUD スプライト描画などでは基本的に半ピクセルのずれを考慮せず普通に整数単位の座標を指定して Ortho2D 座標変換すればいいらしい。
// D3D10:
// http://msdn.microsoft.com/ja-jp/library/ee415722.aspx
// http://msdn.microsoft.com/ja-jp/library/ee415731.aspx
// http://msdn.microsoft.com/ja-jp/library/bb205073.aspx#Additional_Direct3D_10_Differences_to_Watch_For
// D3D9:
// http://msdn.microsoft.com/ja-jp/library/ee422169.aspx
// http://msdn.microsoft.com/ja-jp/library/ee417850.aspx
// http://shikihuiku.wordpress.com/2012/06/12/directx-%E3%81%AErasterization%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6/
// OpenGL:
// http://www.opengl.org/registry/doc/glspec30.20080811.pdf
// http://www.opengl.org/registry/doc/glspec44.core.pdf
float2 ConvertScreenCoordToNormalDeviceCoord(float2 screenPos, float2 screenSize)
{
	return 2 * float2(+screenPos.x, -screenPos.y) / screenSize + float2(-1, +1);
}

VS_OUTPUT_PCT vsScreenPCT(in VS_INPUT_PCT vsIn)
{
	VS_OUTPUT_PCT vsOut = (VS_OUTPUT_PCT)0;

	// 専用の頂点シェーダーを用意せず、Ortho2D の座標変換行列を CPU 側で用意して、vsShadingLessPCTtoPCT() を使ったほうがよいかも。
	// 回転・拡大縮小だけでなく、ビルボードへの応用も利く。
	//vsOut.pos.x = +vsIn.pos.x / (0.5f * UniScreenSize.x) - 1.0f;
	//vsOut.pos.y = -vsIn.pos.y / (0.5f * UniScreenSize.y) + 1.0f;
	vsOut.pos.xy = ConvertScreenCoordToNormalDeviceCoord(vsIn.pos.xy, float2(UniScreenSize.x, UniScreenSize.y));
	vsOut.pos.w = 1;
	vsOut.color = vsIn.color;
	vsOut.tex = vsIn.tex;

	return vsOut;
}
#endif

float4 psFetchAlphaMap(in VS_OUTPUT_PCT psIn) : SV_Target0
{
	// 頂点シェーダーから受け取ったカラー RGBA と、アルファ マップをモジュレートする。
	// サンプリング時にバイリニア フィルタリングする場合、半テクセルずれることを考慮する必要がありそう。
	// 回転や拡大縮小をしないのであれば、フィルタリングを切ってニアレスト ネイバーにしてしまえばいいが……
	// ビルボードに使う場合はフィルタリングしたほうがいい。

	//return float4(psIn.color.rgb, psIn.color.a * SrvAlphaTex.Sample(SS_LinearWrap, psIn.tex).a);
	return float4(psIn.color.rgb, psIn.color.a * SrvAlphaTex.Sample(SS_PointWrap, psIn.tex).a);
	//return psIn.color;
	//return float4(0,0,0,1); // ラスター化ルールのテストコード。
}

#if 0
float4 psFetchShadowMap(in VS_OUTPUT_PCT psIn) : SV_Target0
{
	// カスケードシャドウの各スライスのテスト。
	const float2 t0 = SrvCascadedShadowMapsTex.Sample(SS_LinearClamp, float3(psIn.tex, 0));
	const float2 t1 = SrvCascadedShadowMapsTex.Sample(SS_LinearClamp, float3(psIn.tex, 1));
	const float2 t2 = SrvCascadedShadowMapsTex.Sample(SS_LinearClamp, float3(psIn.tex, 2));
	return float4(t0.r, t1.r, t2.r, 1);
}
#endif

[maxvertexcount(MAX_SHADOW_CASCADES_NUM * 3)]
void gsCreateOffsetVerticesForCascadedShadowTest(triangle VS_OUTPUT_PCT gsIn[3], inout TriangleStream<VS_OUTPUT_PCTI> triStream)
{
	const float scaledShadowSize = 128;
	const float offsetSize = 4;
	for (int split = 0; split < UniAvailableCascadeCount; ++split)
	{
		VS_OUTPUT_PCTI gsOut = (VS_OUTPUT_PCTI)0;
		gsOut.index = split;
		for (int vertex = 0; vertex < 3; ++vertex)
		{
			gsOut.pos = gsIn[vertex].pos;
			gsOut.pos.x += (2 * (scaledShadowSize + offsetSize) / UniScreenSize.x) * split;
			gsOut.color = gsIn[vertex].color;
			gsOut.tex = gsIn[vertex].tex;
			triStream.Append(gsOut);
		}
		triStream.RestartStrip();
	}
}

float4 psFetchCascadedShadowMap(in VS_OUTPUT_PCTI psIn) : SV_Target0
{
	const float2 t = SrvCascadedShadowMapsTex.Sample(SS_LinearClamp, float3(psIn.tex, psIn.index));
	return float4(t.r, t.r, t.r, 0.8);
	//return psIn.color;
}


float4 psFetchDownSampledTex(in VS_OUTPUT_PCT vsOut) : SV_Target0
{
	return float4(DownSampledTex.Sample(SS_LinearWrap, vsOut.tex).xyz, 0.5f);
}


// 構造化バッファをテクスチャのように扱う場合、サンプラー（フィルター）は自前で実装する必要がある。
// RWTexture2D で書き込んで Texture2D で読み取る場合は不要。
// なお、HLSL は C++ のように名前が同じで引数の型だけ異なる関数オーバーロードを作成できる。
#if 0
#define MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE float4
#define MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_NAME SampleLinearClampStructuredBuffer
#include "BilinearFilter.hlsli"

#define MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE uint4
#define MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_NAME SampleLinearClampStructuredBuffer
#include "BilinearFilter.hlsli"
#endif

#include "RandomNumHelper.hlsli"

struct VS_OUTPUT_WAVE
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
	//float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
	float3 wvpos : TEXCOORD1;
	float3 wvnormal : TEXCOORD2;
};

// 波面シミュレーション結果表示の頂点シェーダー。
// シミュレーション自体はコンピュート シェーダーで事前に行ない、UAV 経由でテクスチャなどに出力しておく。
#if 0
VS_OUTPUT_WAVE vsShowWaveSimResult(in VS_INPUT_PCNT vsIn)
{
	VS_OUTPUT_WAVE vsOut = (VS_OUTPUT_WAVE)0;

	const float planeLength = 300;
	vsOut.pos = mul(float4(vsIn.pos.x * planeLength, vsIn.pos.y, vsIn.pos.z * planeLength, 1.0), UniWorldViewProj);
	vsOut.color = vsIn.color;
	//vsOut.normal = vsIn.normal;
	vsOut.tex = vsIn.tex;
	vsOut.wvpos = mul(vsIn.pos, UniWorldView).xyz;

	return vsOut;
}
#else
VS_OUTPUT_WAVE vsShowWaveSimResult(in VS_INPUT_PCNT vsIn, uint instanceId : SV_InstanceID, uint vertexId : SV_VertexID)
{
	VS_OUTPUT_WAVE vsOut = (VS_OUTPUT_WAVE)0;

	// インスタンス ID や頂点 ID を使って、正規化テクスチャ座標ではなく整数インデックスで指定できる。
	// サンプラーによる補間も不要なので、もはやテクスチャである必要性は小さい。
	// 矩形以外の任意形状に対応するならば 2D テクスチャよりも構造化バッファを使うほうがいいかも。
	// もしくは BAB 経由でコンピュート シェーダーによって頂点バッファに含めた情報を直接書き換えてもらうことも可能。
	// 法線の厳密さが不要であれば、頂点シェーダーで改めてテクスチャを複数点フェッチ／サンプリングして法線計算するよりも、
	// コンピュート シェーダーで変位計算時に前フレームの情報を使って法線計算してしまったほうがよい。
	// 頂点シェーダーで法線計算することにこだわるならば、任意形状への対応も考えると BAB ではなく構造化バッファのほうがよい。
	// しかし、矩形の場合はやはり構造化バッファよりも 2D テクスチャのほうが若干高パフォーマンスらしい。
	// 変位のストレージには 2D テクスチャを使い、不要領域は拘束条件を規定する別の 2D テクスチャ
	// （R8_UNORM で格子点ごとの有効／無効を表す 1.0/0.0 フラグを格納する）を別途用意して、変位の計算時にそちらを参照するようにする？
	// また、使われていない格子点はレンダリングしないように適切な頂点バッファとインデックス バッファを構成し、
	// 頂点バッファに格子点 ID を含めておく。あるいは格子点バッファを別途用意して、インスタンス描画を併用する。
	// 頂点バッファに情報を含めるのはレイアウトなどの準備も含めて結構手間がかかるので、
	// できればインスタンス ID などをインデックスとして別のバッファを参照するようにしたほうが拡張性が高い。

	//const int2 gridIndex = int2(instanceId % COMPUTING_TEMP_WORK_SIZE, instanceId / COMPUTING_TEMP_WORK_SIZE);
	const int2 gridIndex = SrvWaveFrontPlaneGridBuffer[instanceId];
	const int2 index = gridIndex + int2(vsIn.tex);
	const int2 size = int2(COMPUTING_TEMP_WORK_SIZE, COMPUTING_TEMP_WORK_SIZE);
#if 0
	const float4 t0 = UniWaveSimResultBuffer[index.y * size.x + index.x];
	const float4 t1 = (index.x - 1 >= 0) ?
		UniWaveSimResultBuffer[index.y * size.x + (index.x - 1)] : (float4)0;
	const float4 t2 = (index.x + 1 < size.x) ?
		UniWaveSimResultBuffer[index.y * size.x + (index.x + 1)] : (float4)0;
	const float4 t3 = (index.y - 1 >= 0) ?
		UniWaveSimResultBuffer[(index.y - 1) * size.x + index.x] : (float4)0;
	const float4 t4 = (index.y + 1 < size.y) ?
		UniWaveSimResultBuffer[(index.y + 1) * size.x + index.x] : (float4)0;
#else
	const float4 t0 = UniWaveSimResultTex[index];
	const float4 t1 = (index.x - 1 >= 0) ?
		UniWaveSimResultTex[uint2(index.x - 1, index.y)] : (float4)0;
	const float4 t2 = (index.x + 1 < size.x) ?
		UniWaveSimResultTex[uint2(index.x + 1, index.y)] : (float4)0;
	const float4 t3 = (index.y - 1 >= 0) ?
		UniWaveSimResultTex[uint2(index.x, index.y - 1)] : (float4)0;
	const float4 t4 = (index.y + 1 < size.y) ?
		UniWaveSimResultTex[uint2(index.x, index.y + 1)] : (float4)0;
#endif

	const float diffHx = (t2.w - t1.w); // X 方向の高さ差分。
	const float diffHz = (t4.w - t3.w); // Z 方向の高さ差分。
	const float planeLength = 300;
	const float oneGridLen = 2.0 * planeLength / COMPUTING_TEMP_WORK_SIZE;
	// 4近傍から傾きを求め、法線を算出する。
	const float3 tanvecx = float3(+oneGridLen, diffHx, 0);
	const float3 tanvecz = float3(0, diffHz, -oneGridLen);
	const float3 normal = cross(tanvecx, tanvecz);
	// 法線の正規化はピクセル シェーダー側で行なう。

	const float4 pos = float4(
		-planeLength + planeLength * (1 + 2 * gridIndex.x + vsIn.pos.x) / COMPUTING_TEMP_WORK_SIZE,
		t0.w,
		-planeLength + planeLength * (1 + 2 * gridIndex.y + vsIn.pos.z) / COMPUTING_TEMP_WORK_SIZE,
		1);

	// コンピュート シェーダーの計算結果はローカル座標系なのでワールド変換が必要。
	vsOut.pos = mul(pos, UniWorldViewProj);
	vsOut.color = vsIn.color;
	//vsOut.tex = tex;
	const float4 wvpos = mul(pos, UniWorldView);
	vsOut.wvpos = wvpos.xyz / wvpos.w;
	vsOut.wvnormal = mul(normal, (float3x3)UniWorldView);

	return vsOut;
}
#endif

// 波面シミュレーションのピクセル シェーダー。
float4 psShowWaveSimResult(in VS_OUTPUT_WAVE vsOut) : SV_Target0
{
#if 0
	// スクリーンにアスペクト比を維持したまま表示させる場合。
	const int maxSize = max(UniScreenSize.x, UniScreenSize.y);
	const uint2 index = (vsOut.tex * UniScreenSize * COMPUTING_TEMP_WORK_SIZE / maxSize) % COMPUTING_TEMP_WORK_SIZE;
#endif

#if 0
	// 構造化バッファの線形補間。
	// 乱数テーブルのテスト用。
	// Texture2D/RWTexture2D は構造化バッファと比べてフォーマットに制約があるが、
	// ランダム アクセスもサンプラーも両方使えるので、
	// コンピュート シェーダーの出力結果をピクセル シェーダーでそのまま利用しやすい。
	// 4つを超える要素を格納することなく、
	// また RWTexture2D が使える cs_5_0 でシミュレーションを行なうならば、
	// サンプラーを指定して Texture2D.Sample() するだけでよい。
	const uint4 t = SampleLinearClampStructuredBuffer(SrvRandomNumTable,
		uint2(COMPUTING_TEMP_WORK_SIZE, COMPUTING_TEMP_WORK_SIZE), vsOut.tex);
	// 乱数を輝度化して出力。
	const float rand = Xorshift128Random::GetRandomComponentUF(t);
	return float4(rand, rand, rand, 1);
#endif

#if 1
	// ライティング（点光源、減衰なし≒平行光源）。
	const float4 lightWVPos = mul(float4(LightPos, 1), UniWorldView);

	// ワールド・ビュー変換した頂点位置、法線およびライト ベクトルを使って照明計算する。
	const float3 lp = normalize(lightWVPos.xyz - vsOut.wvpos);
	const float3 wvnormal = normalize(vsOut.wvnormal);
	const float hl = dot(lp, normalize(reflect(vsOut.wvpos, wvnormal)));
	const float diffuse = max(0, dot(wvnormal, lp));

	const float3 color =
		MaterialColorAmbient.rgb * AmbientLight.rgb +
		diffuse * MaterialColorDiffuse.rgb +
		pow(abs(hl), MaterialSpecularPower) * MaterialColorSpecular.rgb;

	// ピクセル色の出力。
	return float4(color * LightColor.rgb, MaterialOpacityAlpha);
#endif
}

//void FinalProcOfSkinning(float4 skinnedPos, float3 skinnedNormal, inout VS_OUTPUT_PNT finalOutput)
void FinalProcOfSkinning(float4 skinnedPos, float3 skinnedNormal, inout VS_OUTPUT_WORLD_PNT finalOutput)
{
	finalOutput.pos = mul(skinnedPos, UniWorldViewProj); // ローカル座標系を正規化デバイス座標系に変換する。
	finalOutput.normal = mul(skinnedNormal, (float3x3)UniWorldViewProj); // ローカル座標系を正規化デバイス座標系に変換する。

	const float4 wpos = mul(skinnedPos, UniWorld); // ローカル座標系をワールド座標系に変換する。
	finalOutput.wpos = wpos.xyz / wpos.w; // Project to homogeneous coordinate w = 1
	// 法線ベクトルはピクセル シェーダーでピクセル単位のライティングを行なうときに必要。ピクセル シェーダー側で正規化する。
	// ワールド座標系におけるライトとの位置関係でライティングが決まるので、デバイス座標系ではなくワールド座標系での値を要する。
	finalOutput.wnormal = mul(skinnedNormal, (float3x3)UniWorld); // ローカル座標系をワールド座標系に変換する。
}

// スキニングの頂点シェーダー。
VS_OUTPUT_WORLD_PNT vsSkinningFunc(in VS_INPUT_SKIN vsIn)
{
	VS_OUTPUT_WORLD_PNT vsOut = (VS_OUTPUT_WORLD_PNT)0;

	float4 skinnedPos;
	float3 skinnedNormal;
	DoMatrixSkinning(skinnedPos, skinnedNormal, vsIn);
	FinalProcOfSkinning(skinnedPos, skinnedNormal, vsOut);
	vsOut.tex = vsIn.tex;
	// シャドウ サンプリングのためのデータ。
	// Transform the shadow texture coordinates for all the cascades.
	vsOut.LightViewPos = mul(mul(skinnedPos, UniWorld), UniShadowViewMatrix);
	//vsOut.LightViewPos = mul(skinnedPos, UniWorld);
	const float4 wvpos = mul(skinnedPos, UniWorldView);
	//vsOut.WVPos = wvpos;
	vsOut.WVDepth = wvpos.z / wvpos.w;
	return vsOut;
}

// 適応型テッセレーションのコード。
#include "MyAdaptiveTess.hlsli"

#if 1
// ハル シェーダー or ジオメトリ シェーダーに出力を渡す関係上、ワールド変換のみ行なうバージョン。
// なお、面法線を計算で求めるので、頂点法線の出力は不要。
// つまり法線のスキニング計算結果は出力に関係なくなることは自明なので、
// GLSL のようにコンパイラの最適化によって取り除かれることを期待して、位置スキニングだけのオーバーロード関数は書かない。
// HACK: シャドウマップ生成時にもスキニングを行なう必要があるが、
// スキニングの頂点シェーダーは何度も走らせるとパフォーマンス低下の原因になるので（というか描画キックが多数発生するのが問題）、
// ストリーム アウトプットを使うか、
// コンピュート シェーダーでの計算結果を追加／消費構造化バッファもしくは
// 出力用頂点バッファにバインドしたバイト アドレス バッファにキャッシュとして出力して使いまわす方法もある。
// ただしテッセレータを使う場合、出力用頂点バッファのサイズをどうするのか、という問題がある。
VS_OUTPUT_PT vsSkinningWorldOnlyPT(in VS_INPUT_SKIN vsIn)
{
	VS_OUTPUT_PT vsOut = (VS_OUTPUT_PT)0;

	// スキニング後の頂点にワールド変換のみ行なう。
	float4 skinnedPos;
	float3 skinnedNormal;
	DoMatrixSkinning(skinnedPos, skinnedNormal, vsIn);
	const float4 wpos = mul(skinnedPos, UniWorld);
#if 1
	// Project to homogeneous coordinate w = 1
	vsOut.pos = wpos / wpos.w;
#elif 0
	vsOut.pos = wpos.xyz / wpos.w;
#else
	vsOut.pos = wpos.xyz; // NG。
#endif
	vsOut.tex = vsIn.tex;
	return vsOut;
}
#endif


#if 0
// 隣接情報付き三角形を受け取って、中央の三角形のみに変換する。テスト用。
[maxvertexcount(3)]
void gsSimpleAdjPassFunc(triangleadj VS_OUTPUT_WORLD_PNT gsIn[6], inout TriangleStream<VS_OUTPUT_WORLD_PNT> outStream)
{
	outStream.Append(gsIn[0]);
	outStream.Append(gsIn[2]);
	outStream.Append(gsIn[4]);

	outStream.RestartStrip();
}
#endif

[maxvertexcount(6)]
void gsDetectEdge(triangleadj VS_OUTPUT_WORLD_PNT gsIn[6], inout LineStream<VS_OUTPUT_WORLD_PNT> outStream)
{
	float3 v[6];
	[unroll]
	for (int i = 0; i < 6; ++i)
	{
		v[i] = gsIn[i].pos.xyz / gsIn[i].pos.w;
	}

	const float3 v02 = v[2] - v[0];
	const float3 v04 = v[4] - v[0];
	const float3 ccw024 = cross(v02, v04);

	const float3 v01 = v[1] - v[0];
	const float3 ccw012 = cross(v01, v02);

	const float3 v23 = v[3] - v[2];
	const float3 v24 = v[4] - v[2];
	const float3 ccw234 = cross(v23, v24);

	const float3 v05 = v[5] - v[0];
	const float3 ccw045 = cross(v04, v05);

	if (ccw024.z * ccw012.z < 0)
	{
		// 輪郭線を追加。
		outStream.Append(gsIn[0]);
		outStream.Append(gsIn[2]);
		outStream.RestartStrip();
	}
	if (ccw024.z * ccw234.z < 0)
	{
		// 輪郭線を追加。
		outStream.Append(gsIn[2]);
		outStream.Append(gsIn[4]);
		outStream.RestartStrip();
	}
	if (ccw024.z * ccw045.z < 0)
	{
		// 輪郭線を追加。
		outStream.Append(gsIn[4]);
		outStream.Append(gsIn[0]);
		outStream.RestartStrip();
	}
}


#if 1
// 三角形を受け取ってラインを出力し、ファーを生成するジオメトリ シェーダー。
// イメージ ベースではなく物理ベース。
[maxvertexcount(8)]
void gsFurFunc(triangle DS_OUTPUT_PT gsIn[3], inout LineStream<VS_OUTPUT_PCT> outStream)
{
	VS_OUTPUT_PCT gsOut = (VS_OUTPUT_PCT)0;

	// 現状では、面頂点と面中心に毛を生やしているにすぎないが、デバッグ用途には使える。
	// HACK: 三角形の面積を計算して、単位面積当たりの本数（密度）を乗じることで毛の量を統一的に調整できないか？
	// ただしジオメトリ シェーダー1段では出力できる頂点の数に制限がある。1ポリが占める画面内面積の変動で負荷がばらつく。
	// HACK: それともファーマップ テクスチャ フェッチを使ってイメージ ベースにする？
	// HACK: 前段で、シェーダー 5.0 のハードウェア テッセレーションを使ってポリを分割しておくと楽になるか？
	// HACK: 乱数（乱数テーブル or noise() 関数）を使って風ベクトルをリアルタイムに変更し、毛の先端を動かしたい。
	// ジオメトリ シェーダーでは noise() 関数が使えない？
	// コンピュート シェーダーでも noise() 関数が使えない？
	// noise() 関数はテクスチャ シェーダーのみでの利用が可能らしい。
	// Direct3D 9 では D3DXFillTextureTX() とテクスチャ シェーダーを組み合わせて利用するらしいが、
	// D3D 10/11 では利用不可能？
	// GLM には perlin ノイズを生成するヘルパー関数が用意されているので、
	// そちらを利用して CPU 側で乱数テーブル テクスチャを用意したほうがよさげ。
	// もしくは Xorshift を使ってコンピュート シェーダーで生成する。

	//
	// Calculate the face normal
	//
	//const float furLength = 8.0;
	const float furLength = 100.0;
	const float3 faceEdgeA = gsIn[1].pos.xyz - gsIn[0].pos.xyz;
	const float3 faceEdgeB = gsIn[2].pos.xyz - gsIn[0].pos.xyz;
	const float3 faceNormal = normalize(cross(faceEdgeA, faceEdgeB));
	const float3 vExtrudeAmt = faceNormal * furLength;

	//
	// Calculate the face center
	//
	const float3 centerPos = (gsIn[0].pos.xyz + gsIn[1].pos.xyz + gsIn[2].pos.xyz) / 3.0;
	const float2 centerTex = (gsIn[0].tex + gsIn[1].tex + gsIn[2].tex) / 3.0;
	// 中心というか重心座標。cf. 三角形の五心。
	// マテリアルを使うだけであれば、テクスチャ座標は要らない。

	//
	// Output the fur
	//
#if 0
	// 面頂点に毛を生やす。
	for (int i = 0; i < 3; ++i)
	{
		// 根元。
        gsOut.pos = float4(gsIn[i].pos.xyz, 1);
        gsOut.pos = mul(gsOut.pos, UniView);
        gsOut.pos = mul(gsOut.pos, UniProjection);
		gsOut.color = float4(0.5, 0.2, 0.2, 1);
		gsOut.tex = gsIn[i].tex;
		outStream.Append(gsOut);

		// 先端。
		gsOut.pos = float4(gsIn[i].pos.xyz, 1) + float4(vExtrudeAmt, 0);
		gsOut.pos = mul(gsOut.pos, UniView);
		gsOut.pos = mul(gsOut.pos, UniProjection);
		gsOut.color = float4(1.0, 0.5, 0.5, 0);
		gsOut.tex = gsIn[i].tex;
		outStream.Append(gsOut);

		outStream.RestartStrip();
	}
#endif
	// 面中心に毛を生やす。
	{
		// 根元。
        gsOut.pos = float4(centerPos, 1);
        gsOut.pos = mul(gsOut.pos, UniView);
        gsOut.pos = mul(gsOut.pos, UniProjection);
		gsOut.color = float4(0.2, 0.5, 0.2, 1);
		gsOut.tex = centerTex;
		outStream.Append(gsOut);

		// 先端。
        gsOut.pos = float4(centerPos, 1) + float4(vExtrudeAmt, 0);
        gsOut.pos = mul(gsOut.pos, UniView);
        gsOut.pos = mul(gsOut.pos, UniProjection);
		gsOut.color = float4(0.5, 1.0, 0.5, 0);
		gsOut.tex = centerTex;
		outStream.Append(gsOut);

		outStream.RestartStrip();
	}
	// ジオメトリ シェーダー側でマテリアルとライトを使ったディフューズ ライティングをしておき、
	// ピクセル シェーダーでテクスチャと合成するようにするか？
	// ファーの根元はアンビエント オクルージョンよろしく暗くしたほうがよい。
	// ただし乗算合成でなく加算合成する場合は明るさ設定に検討の余地あり。
}
#endif

// VTF を使ってファージオメトリ マップを参照し、ファー方向を表す法線ベクトルと始点 Z を取得する頂点シェーダー。
#if 1
VS_OUTPUT_PCN vsFetchFurMapPCTtoPCN(in VS_INPUT_PCT vsIn)
#else
VS_OUTPUT_PCN vsFetchFurMapPCTtoPCN(in VS_INPUT_PCT vsIn, uint vid : SV_VertexID, uint iid : SV_InstanceID)
#endif
{
	VS_OUTPUT_PCN vsOut = (VS_OUTPUT_PCN)0;
	//vsOut.color = vsIn.color;
	// HACK: 頂点カラーでファーの色を表すが、メッシュ等のレンダリング後のレンダーターゲット カラーマップから値を取得する？
	// ブラーやブルームなどのポスト エフェクト用に作成する中間生成物（非 MSAA テクスチャ）を利用するか、
	// もしくはピクセル シェーダーに渡した UV 座標をもとに MSAA テクスチャからフェッチすればよいが、
	// 地の色と異なる色のファーを生やすことはできない。さらにもうひとつのレンダーターゲットとして、ファーのカラーマップを作るか？
	vsOut.color = float4(1,1,1,1);

	//const int width = UniScreenSize.x;
	//const int height = UniScreenSize.y;
#if 1
	const int x = vsIn.tex.x * UniScreenSize.x;
	const int y = vsIn.tex.y * UniScreenSize.y;
#else
	const int x = iid % UniScreenSize.x;
	const int y = iid / UniScreenSize.x;
#endif
	//const int3 sampleCoord = int3(vsIn.tex.x * width, vsIn.tex.y * height, 0);
	const int3 sampleCoord = int3(x, y, 0);
	const float4 fur = FurGeoMapTex.Load(sampleCoord);
	// 頂点シェーダーおよびジオメトリ シェーダーからは Texture2DMS.Load() メソッドが使えない。というかマルチサンプル テクスチャ全般が扱えない。
	// Texture2DMS が使えるのは、ピクセル シェーダーおよびコンピュート シェーダー。
	// シェーダーモデル 4.1 以上であれば、頂点シェーダーやジオメトリ シェーダーからでも扱える？
	// G-Buffer に MSAA テクスチャを使うとメモリ帯域幅を食いそうだが……

	vsOut.normal = fur.xyz;
#if 1
	vsOut.pos = vsIn.pos;
#else
	vsOut.pos.x = (+2.0f * x) / (UniScreenSize.x - 1) - 1.0f;
	vsOut.pos.y = (-2.0f * y) / (UniScreenSize.y - 1) + 1.0f;
#endif
	vsOut.pos.z = fur.w;

	return vsOut;
}

// イメージ ベースのファー（ライン プリミティブ）を生成するジオメトリ シェーダー。
[maxvertexcount(2)]
void gsImageBasedFur(point VS_OUTPUT_PCN gsIn[1], inout LineStream<VS_OUTPUT_PC> outStream)
{
	// 法線ベクトルがゼロベクトルの場合は、プリミティブの生成を中止して速度を上げる。
	if (length(gsIn[0].normal) > 1.0e-10f)
	{
		VS_OUTPUT_PC gsOut = (VS_OUTPUT_PC)0;
		gsOut.pos = gsIn[0].pos;
		gsOut.color = float4(gsIn[0].color.rgb, 1); // 根元。
		outStream.Append(gsOut);
		//gsOut.pos += 0.1f * float4(gsIn[0].normal, 0); // 法線方向に伸ばす。
		const float furLength = 0.1f;
		//float4 rand;
		// TODO: 乱数テーブル テクスチャを参照して、変動を加える。
		gsOut.pos += furLength * float4(normalize(gsIn[0].normal + 0.3f * sin(gsIn[0].pos.xyz)), 0); // 法線方向に伸ばす。
		//gsOut.pos += furLength * float4(normalize(gsIn[0].normal + 0.3f * sin(rand.xyz)), 0); // 法線方向に伸ばす。
		gsOut.color = float4(gsIn[0].color.rgb, 0); // 先端。
		outStream.Append(gsOut);
	}
	outStream.RestartStrip();
}

struct VS_OUTPUT_FLAKE
{
	float4 Position : SV_Position;
	float4x4 RotMatrix : ROTMATRIX;
};

VS_OUTPUT_FLAKE vsFlakeParticle(in VS_INPUT_P vsInDummy, uint instanceId : SV_InstanceID)
{
	MyFlakeVertex vsIn = SrvFlakeParticleBuffer[instanceId];
	VS_OUTPUT_FLAKE vsOut = (VS_OUTPUT_FLAKE)0;
	//vsOut.Position = float4(vsIn.Position, 1);
	vsOut.RotMatrix = CreateMatrixFromRotationQuaternion(CreateRotationQuaternion(0, 0, 1, vsIn.Attitude.w));
	vsOut.Position = float4(0, 0, 0, 1);
	//vsOut.RotMatrix = IdentityMatrix4x4F;
	return vsOut;
}

[maxvertexcount(4)]
void gsFlakeParticle(point VS_OUTPUT_FLAKE gsIn[1], inout TriangleStream<VS_OUTPUT_PC> outStream)
{
#if 0
	const float UniClipPlaneNearZ = 0.1;
	const float UniClipPlaneFarZ = 2500.0;
	if ((UniClipPlaneNearZ <= -gsIn[0].Position.z) && (-gsIn[0].Position.z <= UniClipPlaneFarZ))
#endif
	{
		VS_OUTPUT_PC gsOut;

		static const float2 texCoords[4] =
		{
			{ 0.0, 0.0 }, // LT
			{ 0.0, 1.0 }, // LB
			{ 1.0, 0.0 }, // RT
			{ 1.0, 1.0 }, // RB
		};

		static const float4 colors[2][2] =
		{
			{ 1.0, 0.0, 0.0, 1.0 },
			{ 0.0, 1.0, 0.0, 1.0 },
			{ 0.0, 0.0, 1.0, 1.0 },
			{ 1.0, 1.0, 1.0, 1.0 },
		};

		[unroll]
		for (int v = 0; v < 4; ++v)
		{
			// TODO: UniProjection を適用する。

			float2 uv = texCoords[v];
			gsOut.color = colors[(int)uv.y][(int)uv.x];
			uv -= float2(0.5, 0.5);
			//uv *= float2(0.5, -0.5);
			uv *= float2(0.1, -0.1);
			const float3 pos = mul(float3(uv.x, uv.y, 1.0), (float3x3)gsIn[0].RotMatrix);
			gsOut.pos = float4(pos.x, pos.y, 0.0, 1.0);

			outStream.Append(gsOut);
		}
		outStream.RestartStrip();
	}
}

/////////////////////////////////////////////////////////////////////

#include "MyRenderStates.hlsli"

/////////////////////////////////////////////////////////////////////
// エフェクト テクニック。

#if 0
// シェーディングなし。
technique11 TechShadingLessPC
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsShadingLessPCtoPC()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psSimplePC()));
	}
}
#endif

// ライトの方向を表すラインを描画する。
technique11 TechRenderLightDirLine
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetDepthStencilState(DSS_DontWriteDepth, 0); // 深度バッファは参照するが書き込まない。
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePtoP()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_4_0, gsCreateLightDirLine()));
		SetPixelShader(CompileShader(ps_4_0, psSimplePC()));
	}
}

// テッセレーション ビルボードのテスト。
technique11 TechTessBillboard
{
	pass p0
	{
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0, 0, 0, 0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_5_0, vsSimpleQuadBillboard()));
		SetHullShader(CompileShader(hs_5_0, hsSimpleQuadBillboard()));
		SetDomainShader(CompileShader(ds_5_0, dsSimpleQuadBillboard()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psSimpleQuadBillboard()));
	}
}

technique11 TechGeomBillboard
{
	pass p0
	{
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0, 0, 0, 0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_5_0, vsSimpleQuadBillboard()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_5_0, gsSimpleQuadBillboard()));
		SetPixelShader(CompileShader(ps_5_0, psSimpleQuadBillboard()));
	}
}

technique11 TechFlakeParticle
{
	pass p0
	{
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0, 0, 0, 0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_5_0, vsFlakeParticle()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_5_0, gsFlakeParticle()));
		SetPixelShader(CompileShader(ps_5_0, psSimpleQuadBillboard()));
	}
}

// 座標軸を表すラインを描画する。
technique11 TechRenderCoordAxisLines
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetDepthStencilState(DSS_DontWriteDepth, 0); // 深度バッファは参照するが書き込まない。
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsShadingLessPCtoPC()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psSimplePC()));
	}
}

// シャドウマップをテスト用に HUD として描画する。
technique11 TechShadowHudTest
{
	pass p0
	{
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsShadingLessPCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
#if 1
		SetGeometryShader(CompileShader(gs_4_0, gsCreateOffsetVerticesForCascadedShadowTest()));
		SetPixelShader(CompileShader(ps_4_0, psFetchCascadedShadowMap()));
#else
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psFetchShadowMap()));
#endif
	}
}

// フォント。
technique11 TechFont
{
	pass p0
	{
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetDepthStencilState(DSS_DontWriteDepth, 0);
		SetDepthStencilState(DSS_NoDepthStencil, 0);
		//SetRasterizerState(RS_Solid);

		//SetVertexShader(CompileShader(vs_4_0, vsScreenPCT()));
		SetVertexShader(CompileShader(vs_4_0, vsShadingLessPCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psFetchAlphaMap()));
	}
}

technique11 TechAddDownSampledTex
{
	pass p0
	{
		//SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetBlendState(BS_SrcAlphaBlendingAdd, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);
		//SetRasterizerState(RS_Solid);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psFetchDownSampledTex()));
	}
}

technique11 TechEdgeDetectColorSketch
{
	pass p0
	{
		//SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_SrcAlphaBlendingAdd, float4(0,0,0,0), 0xFFFFFFFF);
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_NoDepthStencil, 0);
		//SetRasterizerState(RS_Solid);

		SetVertexShader(CompileShader(vs_4_0, vsSimplePCTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psEdgeDetectColorSketch()));
	}
}

technique11 TechDisplaySimpleComputingTest
{
	pass p0
	{
		//SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);
		//SetDepthStencilState(DSS_NoDepthStencil, 0);
		//SetRasterizerState(RS_Solid);

		SetVertexShader(CompileShader(vs_4_0, vsShowWaveSimResult()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psShowWaveSimResult()));
	}
}

// ランバート。
technique11 TechLambertPCNT
{
	pass p0
	{
		SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		//SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsLambertPCNTtoPCT()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, psSimplePCT()));
	}
}


// ホスト側で取得して、ローレベル API を使って明示的に適用するシェーダー。
// Dynamic Shader Linkage を必要とするシェーダーはテクニックから分離してコンパイル＆生成しておく。
// 予備のエフェクト パスにプールを作成することも考えたが、冗長なので却下。
//PixelShader g_psPhongAndCreateFurMap = CompileShader(ps_5_0, psPhongAndCreateFurMap());


// スキニング（フォン／トゥーン＋インク＋ファー）。
technique11 TechSkinning
{
	// 本体を描画する。
	// 最初は Dynamic Shader Linkage を必要とするシェーダーは含めないようにするため、ダミーシェーダーをバインドしておく。
	// 後でレンダリング時に C++ コードを使って差し替える。
	pass passBody
	{
		// TODO: マテリアル特性に応じて、ブレンディングを制御したほうがよい。
		// （加算合成／乗算合成／ブレンディングなし／etc.）
		//SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_WireFrame);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_5_0, vsSkinningFunc()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psSkinningDummyNoIF()));
	}

	pass passInk
	{
		//SetBlendState(BS_NoBlending, float4(0,0,0,0), 0xFFFFFFFF);
		SetBlendState(BS_NormalAlphaBlendingOn, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_WireFrame);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_DefaultDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, vsSkinningFunc()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_4_0, gsDetectEdge()));
		SetPixelShader(CompileShader(ps_4_0, psToonInk()));
	}

	// ファーをテッセレーションにより生成＆描画する。
	// Input Assembler のトポロジータイプの違いに注意。
	pass passFur
	{
		SetBlendState(BS_SrcAlphaBlendingAdd, float4(0,0,0,0), 0xFFFFFFFF);
		//SetRasterizerState(RS_Solid);
		SetDepthStencilState(DSS_DontWriteDepth, 0);
		// ファーを描画する際は、Z バッファへの書き込みは行なわない。

		SetVertexShader(CompileShader(vs_5_0, vsSkinningWorldOnlyPT()));
		SetHullShader(CompileShader(hs_5_0, hsSimpleTriPT()));
		SetDomainShader(CompileShader(ds_5_0, dsSimpleTriPT()));
		SetGeometryShader(CompileShader(gs_5_0, gsFurFunc()));
		SetPixelShader(CompileShader(ps_5_0, psSimplePCT()));
	}
}


// イメージ ベース（スクリーン ベース、ポスト エフェクト）のファーシェーダー。
technique11 TechImageBasedFur
{
	pass p0
	{
		SetBlendState(BS_SrcAlphaBlendingAdd, float4(0,0,0,0), 0xFFFFFFFF);
		SetDepthStencilState(DSS_DontWriteDepth, 0);

		SetVertexShader(CompileShader(vs_4_0, vsFetchFurMapPCTtoPCN()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(CompileShader(gs_4_0, gsImageBasedFur()));
		SetPixelShader(CompileShader(ps_4_0, psSimplePC()));
	}
}
