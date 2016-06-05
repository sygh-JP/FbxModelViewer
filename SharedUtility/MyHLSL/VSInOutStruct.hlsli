// Only UTF-8 or ASCII is available.

// 頂点シェーダーの入出力パラメータ構造体。
// C++ 側の POSITION 入力が float3 (DXGI_FORMAT_R32G32B32_FLOAT) でも、HLSL 側では float4 で受ける。
// SV_Position セマンティクスを含む出力は、ハル シェーダー、ジオメトリ シェーダーあるいはピクセル シェーダーへの入力パラメータにもなる。
// ピクセル シェーダーへの入力においては、SV_Position は必須。
// ハル シェーダーへの入力においては、SV_Position は必須ではない。
// 英語のコメントは、参考にした XNA ACL のハードウェア スキニング コードの名残。

struct VS_INPUT_T
{
	float2 TexCoord : TEXCOORD;
};

struct VS_INPUT_P
{
	float4 Position : POSITION;
};

struct VS_INPUT_PC
{
	float4 pos : POSITION;
	float4 color : COLOR; // Vertex color.
};

struct VS_INPUT_PT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD;
};

struct VS_INPUT_PCT
{
	float4 pos : POSITION;
	float4 color : COLOR; // Vertex color.
	float2 tex : TEXCOORD;
};

struct VS_INPUT_PCNT
{
	float4 pos : POSITION;
	float4 color : COLOR; // Vertex color.
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
};

struct VS_INPUT_SKIN
{
	float4 pos      : POSITION;
	float3 normal   : NORMAL;
	float2 tex      : TEXCOORD0;

	// These are the indices (4 of them) that index the bones that affect this vertex.
	// The indices refer to the MatrixPalette.
	//uint4 indices  : BLENDINDICES0;
	// These are the weights (4 of them) that determine how much each bone affects this vertex.
	//float4 weights : BLENDWEIGHT0;

	//half4 indices  : BLENDINDICES0; // NG.
	//float4 indices  : BLENDINDICES0; // NG.
	// HLSL には byte, short, half が存在しない（D3D 10/11 用 HLSL では half は float 扱い）。
	// 頂点シェーダーへの入力時は int や float で受けることになる。
	// BYTE[4] から uint4 へのマッピングは入力レイアウトやセマンティクスを適切に定義しておけば Direct3D が自動でやってくれるらしい。
	// R8G8B8A8 や R16G16B16A16（IEEE 754 FP16）で記録されたテクスチャからデータを読み取る場合も同様。

	// 合計8つまでのボーン インデックス。
	uint4 indices0  : BLENDINDICES0;
	uint4 indices1  : BLENDINDICES1;

	// 合計8つまでのボーン ウエイト。
	float4 weights0 : BLENDWEIGHT0;
	float4 weights1 : BLENDWEIGHT1;
};


#if 0
// This is the output from our skinning method
struct SKIN_VS_OUTPUT
{
	float4 pos : SV_Position;
	float3 normal : NORMAL;
};
#endif

struct VS_OUTPUT_P
{
	float4 pos : SV_Position;
};

struct VS_OUTPUT_PC
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
};

struct VS_OUTPUT_PCT
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
};

// シャドウマップのテスト描画用に、カスケード インデックスを保持する。
struct VS_OUTPUT_PCTI
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	int index : TEXCOORD1;
};

struct VS_OUTPUT_PT
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT_PCN
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
	float3 normal : NORMAL;
};

struct VS_OUTPUT_PNT
{
	float4 pos : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT_WORLD_PNT
{
	float4 pos : SV_Position; // WVP 変換後の正規化デバイス座標系。
	float3 normal : NORMAL; // WVP 変換後の正規化デバイス座標系。
	float2 tex : TEXCOORD0;
	float3 wpos : TEXCOORD1; // ワールド座標系。
	float3 wnormal : TEXCOORD2; // ワールド座標系。
	float4 LightViewPos : TEXCOORD3; // シャドウマップ サンプリング用。
	float WVDepth : TEXCOORD4; // シャドウマップ サンプリング用。
	//float4 WVPos : TEXCOORD5; // シャドウマップ サンプリング用。
};

struct VS_OUTPUT_PCNT
{
	float4 pos : SV_Position;
	float4 color : COLOR0;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};


// 頂点入力レイアウト作成に必要となる頂点シェーダー BLOB の作成用に、頂点入力構造体を使用するだけの関数を用意しておく。
// 実際のパイプライン構成用途には使わない。
// なお、vs_4_0_level_9_x を使う場合、SV_Position は必ず出力に含める必要がある（vs_4_0 や vs_5_0 ではジオメトリ シェーダーなどがあるので必須ではない）。
// ちなみにメンバーの定義がまったく同じでセマンティクスだけが異なる構造体は、暗黙の変換が可能になるらしい。
// セマンティクスは大文字・小文字を区別しないようだが、定義済みの値を使う場合はできるだけ MSDN の HLSL ヘルプに書いてある形式をそのまま使っておいたほうがよい。
// http://msdn.microsoft.com/ja-jp/library/ee418355.aspx

static const float4 DummyColor4F = { 0, 0, 0, 1 };

inline float4 vsLayoutGenDummyT(in VS_INPUT_T vsIn) : SV_Position
{
	//return vsIn;
	return DummyColor4F;
}

inline float4 vsLayoutGenDummyP(in VS_INPUT_P vsIn) : SV_Position
{
	//return vsIn;
	return DummyColor4F;
}

inline float4 vsLayoutGenDummyPC(in VS_INPUT_PC vsIn) : SV_Position
{
	//return vsIn;
	return DummyColor4F;
}

inline float4 vsLayoutGenDummyPCT(in VS_INPUT_PCT vsIn) : SV_Position
{
	//return vsIn;
	return DummyColor4F;
}

inline float4 vsLayoutGenDummyPCNT(in VS_INPUT_PCNT vsIn) : SV_Position
{
	//return vsIn;
	return DummyColor4F;
}

inline float4 vsLayoutGenDummyPNTIW(in VS_INPUT_SKIN vsIn) : SV_Position
{
	return DummyColor4F;
}


VS_OUTPUT_PCT vsSimplePCTtoPCT(in VS_INPUT_PCT vsIn)
{
#if 0
	VS_OUTPUT_PCT vsOut = (VS_OUTPUT_PCT)0;
	vsOut.pos = vsIn.pos;
	vsOut.tex = vsIn.tex;
	vsOut.color = vsIn.color;

	return vsOut;
#else
	// 構造体のフィールド定義に互換性がある場合、そのまま暗黙変換できるらしい。
	return vsIn;
#endif
}

VS_OUTPUT_PT vsSimplePTtoPT(in VS_INPUT_PT vsIn)
{
	return vsIn;
}

VS_OUTPUT_P vsSimplePtoP(in VS_INPUT_P vsIn)
{
	return vsIn;
}
