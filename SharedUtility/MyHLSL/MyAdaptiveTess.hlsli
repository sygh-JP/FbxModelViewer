// Only UTF-8 or ASCII is available.

// ハル シェーダーの partitioning を integer としたとき、
// テッセレーション因数 T は切り捨てられ、TTT-T による分割後の三角形の個数 N は、比較的単純な漸化式で求まる？
// 111-1 :               = (6*0+1)  = 1 [face]
// 222-2 :               = (6*1)    = 6 [faces]
// 333-3 : (6-0+1+6)     = (6*2+1)  = 13 [faces]
// 444-4 : (13-1+6+6)    = (6*4)    = 24 [faces]
// 555-5 : (24-6+13+6)   = (6*6+1)  = 37 [faces]
// 666-6 : (37-1+24+6)   = (6*11)   = 66 [faces]
// 777-7 : (66-6+37+6)   = (6*17+1) = 103 [faces]
// 888-8 : (103-1+66+6)  = (6*29)   = 174 [faces]
// 999-9 : (174-6+103+6) = (6*46+1) = 277 [faces]
// ...となるはず。
// fractional_even や fractional_odd を使うと、連続的に分割量を遷移させられるが、
// fractional_even を使うと、1は2に丸められてしまうので、「分割なし」とすることはできなくなる。

#include "CommonFuncs.hlsli"

struct QuadTessFactorData
{
	float edges[4] : SV_TessFactor;
	float insides[2] : SV_InsideTessFactor;
};

struct TriTessFactorData
{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

static const QuadTessFactorData NoDivQuadTessFactorData = { 1, 1, 1, 1, 1, 1 };
static const TriTessFactorData NoDivTriTessFactorData = { 1, 1, 1, 1 };

#if 0
struct HS_INPUT_P
{
	float3 pos : POSITION0; // float4 ではない。頂点シェーダーでトランスフォームした後の値が渡される。
};

struct HS_INPUT_PC
{
	float3 pos : POSITION0; // float4 ではない。頂点シェーダーでトランスフォームした後の値が渡される。
	float4 color : COLOR0;
};

struct HS_INPUT_PT
{
	float3 pos : POSITION0; // float4 ではない。頂点シェーダーでトランスフォームした後の値が渡される。
	float2 tex : TEXCOORD0;
};
#endif

// 行列や配列を頂点構造体に使う場合、
// セマンティクスのインデックス（TEXCOORDn などにおける n）は自動的に複数割り当てられることになるので注意。
// たとえば float[3] や float3x3 ではともに3スロット分消費するので、インデックスが3つ分割り当てられる。
// http://forum.devmaster.net/t/shader-compiler-bug-dx10/15112

struct VS_OUTPUT_PARTICLE
{
	float4 Position : SV_Position;
	float4 TexCoord : TEXCOORD0; // 左上座標とサイズもしくは左上座標と右下座標。
	//float AngleZ : TEXCOORD1;
	//float2 CosSin : TEXCOORD1;
	float3x3 RotMatrix : ROTMATRIX;
};

//typedef HS_INPUT_P HS_OUTPUT_P;
//typedef HS_INPUT_PC HS_OUTPUT_PC;
//typedef HS_INPUT_PT HS_OUTPUT_PT;

//typedef VS_OUTPUT_P DS_OUTPUT_P;
typedef VS_OUTPUT_PC DS_OUTPUT_PC;
typedef VS_OUTPUT_PT DS_OUTPUT_PT;


float CalcApploximatedTessFactor(float divNum)
{
	// テッセレーション因数 T、三角形分割数 N に対して、
	// N = a * exp(b * T) すなわち T = log(N / a) / b で近似し、最小自乗法により a と b を求めている。
	static const float a = 2.22811232;
	static const float b = 0.51683330;
	return log(divNum / a) / b;
}

// ファーの色は頂点カラー、マテリアルもしくはテクスチャで決定する。
// ドメイン シェーダーの後段、ジオメトリ シェーダーが出力するライン プリミティブにテクスチャ UV 情報を付加し、
// テクスチャの地の色に左右されるようにすれば、ヒョウ柄のファーなどがプログラマブルに実現できる。

// ハル シェーダーのパッチ定数関数。
// Adaptive Tessellation Fur 用。
TriTessFactorData HSPatchConstantFuncATFurTriPT(const InputPatch<VS_OUTPUT_PT, 3> patch, uint pid : SV_PrimitiveID)
{
	// テッセレーション因数が1であれば分割なし。0であればポリゴンが消滅し、Discard されるらしい。
	// 分割後に法線方向に押し出すなどの操作を加える場合はポリゴン チャンク（メッシュ）単位で同一のテッセレーション係数を与えないと、隣接ポリゴン間で頂点ずれが発生してしまう。
	// カメラからの距離に応じて因数を制御する距離ベースの適応型テッセレーションと、ピクセル面積に応じて因数を制御するスクリーン スペースの適応型テッセレーションがある。
	// 距離ベースはメイン レンダリング エンジン（CPU）側から因数（の指標）を定数バッファ経由で与えることも可能だが、
	// ピクセル ベースは GPU 側での独立判断が重要になりそう。
	// 他にも、一般的に疎メッシュだとアラが目立ちやすい部分である輪郭に近い部分のテッセレーションを豊かにし、そうでない部分は乏しくする最適化手法もある。
	// http://game.watch.impress.co.jp/docs/series/3dcg/20100310_353969.html

	// 行列の「数学的乗算」を HLSL で行なう場合、必ず mul() 関数を使う必要がある。
	// * 演算子を使った乗算は、単純に成分ごとの重ね合わせ乗算が行なわれるだけなので注意。GLSL とは挙動が異なる。
	// ともあれ、定数バッファには余裕があるが命令数を削減する必要がある場合、シェーダーでの行列同士の乗算は可能な限り避けたほうがいいかも。
	// CPU 側での事前計算1回で済ませられる場合はそのほうがよいかも。
	const float3 worldFaceEdgeA = (patch[1].pos - patch[0].pos).xyz;
	const float3 worldFaceEdgeB = (patch[2].pos - patch[0].pos).xyz;
	const float worldArea = CalcTriangleArea(worldFaceEdgeA, worldFaceEdgeB);
	// 今回は正規化座標系やスクリーン座標系に射影した後の頂点位置が必要なのではなく、あくまで三角形の面積が求まればよい。
	const float4 faceEdgeA = mul(float4(worldFaceEdgeA, 1), UniViewProj);
	const float4 faceEdgeB = mul(float4(worldFaceEdgeB, 1), UniViewProj);
	//const float4 pos0 = mul(float4(patch[0].pos, 1), UniViewProj);
	//const float4 pos1 = mul(float4(patch[1].pos, 1), UniViewProj);
	//const float4 pos2 = mul(float4(patch[2].pos, 1), UniViewProj);
	// 正規化座標系をスクリーン座標系にスケーリングする。
	const float2 scaling = float2(UniScreenSize.x * 0.5f, UniScreenSize.y * 0.5f);
	//const float2 screenFaceEdgeA = scaling * (pos1.xy / pos1.w - pos0.xy / pos0.w);
	//const float2 screenFaceEdgeB = scaling * (pos2.xy / pos2.w - pos0.xy / pos0.w);
	const float2 screenFaceEdgeA = scaling * (faceEdgeA.xy / faceEdgeA.w);
	const float2 screenFaceEdgeB = scaling * (faceEdgeB.xy / faceEdgeB.w);
	const float screenArea = CalcTriangleArea(screenFaceEdgeA, screenFaceEdgeB); // 外積の大きさは平行四辺形の面積になる。
	const float worldFurDensity = 0.20f; // 希望する物理単位面積あたりのファーの数（ワールド座標系）。
	// TODO: 密度を定数バッファで外部指定する。

	//const float screenFurDensity = 1;
	// 分割数を N とすると、N / S = d となる。
	// N を実現するための適切なテッセレーション因数 T を求めることが最終目標となる。
	//const float minAreaPix = 10 * 10; // [pixels^2]
	const float minAreaPix = 2 * 2; // [pixels^2]
	const float worldDesiredDivNum = worldFurDensity * worldArea;
	//const float screenDesiredDivNum = screenFurDensity * screenArea;

	TriTessFactorData output = (TriTessFactorData)0;
	// 分割後の重心分布をできるだけ均一にしたい場合、4つあるテッセレーション因数はすべて同じ値にしてしまったほうがよさげ。

	// TODO: 頂点シェーダーの出力結果（ワールド座標系）を使ってワールド座標系における物理面積を求め、
	// さらに正規化座標変換後の三角形とスクリーン サイズを使ってピクセル面積を割り出し、適切な分割数を決める複合式適応型テッセレーションとする。
	// HACK: 単純に分割するだけだったら Screen Space での Adaptive Tessellation も可能だが、
	// 分割後に大きく形状を変えたり、プリミティブ種類を変えたりする場合は厳しい。
	//if (screenArea < minAreaPix)
	//if (true)
	if (false)
	{
		const float tessFactor = 1, insideTessFactor = 1;
		// 分割なし。
		output.edges[0] = output.edges[1] = output.edges[2] = tessFactor;
		output.inside = insideTessFactor;
	}
	else
	{
		const float tessFactor = clamp(CalcApploximatedTessFactor(worldDesiredDivNum), 1, 9);
		//const float tessFactor = 4, insideTessFactor = 4;
		output.edges[0] = output.edges[1] = output.edges[2] = tessFactor;
		output.inside = tessFactor;
	}

	return output;
}

// ハル シェーダー。
[domain("tri")]
//[partitioning("integer")]
//[partitioning("pow2")]
//[partitioning("fractional_even")]
[partitioning("fractional_odd")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSPatchConstantFuncATFurTriPT")]
VS_OUTPUT_PT hsSimpleTriPT(const InputPatch<VS_OUTPUT_PT, 3> patch, uint ocpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)
{
#if 0
	HS_OUTPUT_PT output = (HS_OUTPUT_PT)0;

	output.pos = patch[ocpid].pos;
	output.tex = patch[ocpid].tex;
	return output;
#else
	return patch[ocpid];
#endif
}

// ドメイン シェーダー。
[domain("tri")]
DS_OUTPUT_PT dsSimpleTriPT(TriTessFactorData factor, float3 uvw : SV_DomainLocation, const OutputPatch<VS_OUTPUT_PT, 3> patch)
{
	DS_OUTPUT_PT dsOut = (DS_OUTPUT_PT)0;

	dsOut.pos.xyz = (patch[2].pos * uvw.x + patch[1].pos * uvw.y + patch[0].pos * uvw.z).xyz;
	dsOut.pos.w = 1;
	dsOut.tex = patch[2].tex * uvw.x + patch[1].tex * uvw.y + patch[0].tex * uvw.z;

	return dsOut;
}

// CAPCOM の Panta Rhei エンジンでは、テッセレータを利用したビルボード生成が実装されているらしい。
// 普通にジオメトリ シェーダーを使うより高速になる？
// http://game.watch.impress.co.jp/docs/series/3dcg/20130731_609420.html

VS_OUTPUT_PARTICLE vsSimpleQuadBillboard(in VS_INPUT_P vsInDummy, uint instanceId : SV_InstanceID)
{
	VS_OUTPUT_PARTICLE vsOut = (VS_OUTPUT_PARTICLE)0;
	vsOut.Position = float4(0, 0, 0, 1);
	// TODO: 構造化バッファに仕込まれた回転角と頂点位置をもとに、回転の同次変換行列を頂点シェーダーで生成して、ハル シェーダー・ドメイン シェーダーに渡す。
	const float rotAngle = radians(30.0);
	vsOut.RotMatrix = CreateRotationMatrix3x3Z(rotAngle, vsOut.Position.xy);
	return vsOut;
}

QuadTessFactorData HSPatchConstantFuncSimpleQuadBillboard(const InputPatch<VS_OUTPUT_PARTICLE, 1> patch, uint pid : SV_PrimitiveID)
{
	return NoDivQuadTessFactorData;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HSPatchConstantFuncSimpleQuadBillboard")]
VS_OUTPUT_PARTICLE hsSimpleQuadBillboard(const InputPatch<VS_OUTPUT_PARTICLE, 1> patch, uint ocpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)
{
	return patch[ocpid];
}

[domain("quad")]
DS_OUTPUT_PC dsSimpleQuadBillboard(QuadTessFactorData factor, float2 uv : SV_DomainLocation, const OutputPatch<VS_OUTPUT_PARTICLE, 4> patch)
{
	DS_OUTPUT_PC dsOut = (DS_OUTPUT_PC)0;

	static const float4 colors[2][2] =
	{
		{ 1.0, 0.0, 0.0, 1.0 },
		{ 0.0, 1.0, 0.0, 1.0 },
		{ 0.0, 0.0, 1.0, 1.0 },
		{ 1.0, 1.0, 1.0, 1.0 },
	};
	//dsOut.color = float4(uv.x, uv.y, 1, 1);
	dsOut.color = colors[(int)uv.y][(int)uv.x];

	// 正規化座標系での値を直接指定してみる。UV は左上原点だが、正規化座標系は左下原点。
	// http://wlog.flatlib.jp/archive/1/2008-11-14

	// TODO: UniProjection を適用する。

	uv -= float2(0.5, 0.5);
	//uv *= float2(0.5, -0.5);
	uv *= float2(0.1, -0.1);
	const float3 pos = mul(float3(uv.x, uv.y, 1.0), patch[0].RotMatrix);
	dsOut.pos = float4(pos.x, pos.y, 0.0, 1.0);
	// TODO: ビルボードのテクスチャ UV 座標はテッセレーション前の頂点バッファ（Quad ではなく Point）に格納されている
	// 情報（左上座標とサイズもしくは左上座標と右下座標）から生成する。
	// 頂点カラー情報も生成しておくとデバッグしやすい。
	// ドメイン シェーダーで頂点番号が必要であれば、ハル シェーダーから頂点情報として SV_OutputControlPointID を渡せばいい？
	// ドメイン シェーダーに渡されるのは結局 UV とパッチ配列なので、UV から判断しないとダメ。
	// すべてのハル シェーダーは、各パッチで (SV_PrimitiveID により識別される)
	// 出力コントロール ポイントごとに (SV_OutputControlPointID により識別される) 1 回呼び出される。

	return dsOut;
}

[maxvertexcount(4)]
void gsSimpleQuadBillboard(point VS_OUTPUT_PARTICLE gsIn[1], inout TriangleStream<VS_OUTPUT_PC> outStream)
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
			const float3 pos = mul(float3(uv.x, uv.y, 1.0), gsIn[0].RotMatrix);
			gsOut.pos = float4(pos.x, pos.y, 0.0, 1.0);

			outStream.Append(gsOut);
		}
		outStream.RestartStrip();
	}
}

float4 psSimpleQuadBillboard(VS_OUTPUT_PC psIn) : SV_Target0
{
	return psIn.color;
}
