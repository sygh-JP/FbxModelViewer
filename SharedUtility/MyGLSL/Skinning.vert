//#version 440
//#version 430
//#version 420
//#version 330
//#version 120

// GLSL 4.0 から input/output が予約語になったらしい（4.4 時点ではまだ実際に使われてはいない模様）。
// OpenSubdiv の開発でも指摘されている。
// https://github.com/PixarAnimationStudios/OpenSubdiv/pull/164
// http://stackoverflow.com/questions/6232153/what-keywords-glsl-introduce-to-c
// http://www.opengl.org/registry/doc/GLSLangSpec.4.00.8.clean.pdf
// output は NVIDIA の OpenGL 4.4 対応ドライバー 332.21 で #version 430 を指定すると
// 確かにコンパイル エラーになるが、input はまだ使えるらしい？
// #version 420 までは output を変数名に使ってもコンパイル エラーにならない。

// Direct3D 10 のシェーダーモデル 4.0 以降では頂点シェーダーの定数レジスタ数（定数バッファの総サイズ）にかなり余裕があるが、
// OpenGL 2.x 世代のビデオ カードでは Direct3D 9 同様の制約が発生する。
// シェーダーモデル 2.0 や 3.0 では 256 本が保証されるだけ。
// また、Intel オンボードなどのように、たとえ Direct3D 10 をサポートしていても OpenGL は 2.x までしかサポートしなかったり、
// Direct3D 11 をサポートしていても OpenGL は 3.x までしかサポートしなかったりするものがあるが、
// OpenGL API を使う場合は OpenGL のバージョンに即した制約となる可能性がある。要調査。
// ※例え OpenGL 4.0 以上をサポートしていても、OpenGL Uniform Block サイズの上限は GPU ごとに異なるらしい。
// Direct3D 11 同等の仕様すら満たしていないものもある。
// http://wlog.flatlib.jp/?itemid=1634
// ちなみに MSAA 能力は Direct3D Feature Level とは直接の関係がないらしい（特に Direct3D 10.0 およびそれ以前において、実装は必須ではない）。
// http://msdn.microsoft.com/ja-jp/library/ee422084.aspx
// Intel オンボードは MSAA をサポートしない、もしくは MSAA レベルが低いものが多い。


//#define MAX_ANIM_BONE_NUM  (56)
//#define MAX_ANIM_BONE_NUM  (120)

// GLSL は typedef が使えないらしい……最低だ……
// static const も構文として使えない。const のみ。
// また、比較的 C++ に近い HLSL と違い、const int を配列要素数として使えないらしい。
// なお、単精度浮動小数点リテラルの f ポストフィックスはサポートしないが、
// 符号なし整数リテラルの u ポストフィックスは必須。
// 初期化は初期化子リストではなくコンストラクタ構文しか使えないのに、
// その他は C++ よりはむしろ C に近いという非常に中途半端な言語仕様。
// 結局 GLSL 4.2 で初期化子リストもサポートするようになった。

#if 0
uniform mat4 UniWorld;
uniform mat4 UniWorldViewProj;

uniform mat4 UniBoneMatrixPalette[MAX_ANIM_BONE_NUM];
#endif

// 以前のバージョンの GLSL では attribute/varing/in/out に ivec4 などの整数は直接使えなかったらしい。
// glVertexAttribPointer() の normalized パラメータに GL_TRUE を渡し、整数を正規化して浮動小数点数の vec4 として渡す必要があった。
// つまり、整数値を取り出すときはシェーダーでスケーリングして復元していたので、精度にも限界があった。
// OpenGL 3.3 以降では、glVertexAttribIPointer() を使えば正規化＆復元の必要はないらしい。

layout(location = 0) in vec3 VsInPosition;
layout(location = 1) in vec3 VsInNormal;
layout(location = 2) in vec2 VsInTexCoord;
layout(location = 3) in ivec4 VsInBoneIndices0;
layout(location = 4) in ivec4 VsInBoneIndices1;
layout(location = 5) in vec4 VsInBoneWeights0;
layout(location = 6) in vec4 VsInBoneWeights1;

//in vec4 VsInBoneIndices; // 旧版。正規化されたインデックス（[0.0, 1.0] の範囲）。


struct VS_INPUT_SKIN
{
	vec4 pos;
	vec4 normal;
	//vec2 tex;
#if 1
	ivec4 indices0;
	ivec4 indices1;
#else
	vec4 indices0; // 正規化されたインデックス。
#endif
	vec4 weights0;
	vec4 weights1;
};

struct SkinningFinalOutputInfo
{
	vec4 pos;
	//vec4 color;
	//vec3 normal;
	//vec2 tex;
	vec3 wpos;
	vec3 wnormal;
};


void DoMatrixSkinning(out vec4 outPos, out vec4 outNormal, const in VS_INPUT_SKIN skinInput)
{
	outPos = ZeroVector4F;
	outNormal = ZeroVector4F;
	for (int i = 0; i < 4; ++i)
	{
		float weight0 = skinInput.weights0[i];
#if 1
		int index0 = skinInput.indices0[i];
#else
		int index0 = int(skinInput.indices0[i] * 255); // 正規数を byte としてスケーリング。
#endif
		outPos += UniBoneMatrixPalette[index0] * skinInput.pos * weight0;
		outNormal += UniBoneMatrixPalette[index0] * skinInput.normal * weight0;

		float weight1 = skinInput.weights1[i];
		int index1 = skinInput.indices1[i];
		outPos += UniBoneMatrixPalette[index1] * skinInput.pos * weight1;
		outNormal += UniBoneMatrixPalette[index1] * skinInput.normal * weight1;
	}
}


void FinalProcOfSkinning(out SkinningFinalOutputInfo finalOutput, in vec4 skinnedPos, in vec4 skinnedNormal)
{
	finalOutput.pos = UniWorldViewProj * skinnedPos;

	// 今のところデバイス座標系での法線は必要ないのでコメントアウト。
	//finalOutput.normal = normalize((UniWorldViewProj * skinnedNormal).xyz);

	vec4 wpos = UniWorld * skinnedPos;
	finalOutput.wpos = wpos.xyz / wpos.w; // Project to homogeneous coordinate w = 1
	// HACK: 法線はピクセル シェーダー側で改めて正規化するので、頂点シェーダー側での正規化は不要のはず。
	finalOutput.wnormal = normalize((UniWorld * skinnedNormal).xyz);
}


// ジオメトリ シェーダーを使わない場合はインターフェイス ブロックよりも個別の out 変数を使ったほうが楽ではある。
// HLSL のセマンティクスとは違って GLSL は予約名（組み込み変数）形式になっているのがいただけない。

//out vec2 mygl_TexCoord0;
//out vec3 mygl_WPos;
//out vec3 mygl_WNormal;

out VS_OUTPUT_WORLD_PNT
{
	vec2 tex;
	vec3 wpos;
	vec3 wnormal;
} vsOut;

void main(void)
{
	VS_INPUT_SKIN vsIn = VS_INPUT_SKIN(
		vec4(VsInPosition, 1.0),
		vec4(VsInNormal, 0.0),
		VsInBoneIndices0,
		VsInBoneIndices1,
		VsInBoneWeights0,
		VsInBoneWeights1
		);

	vec4 skinnedPos;
	vec4 skinnedNormal;
	DoMatrixSkinning(skinnedPos, skinnedNormal, vsIn);
	SkinningFinalOutputInfo skinOut;
	FinalProcOfSkinning(skinOut, skinnedPos, skinnedNormal);

	gl_Position = skinOut.pos;
	vsOut.tex = VsInTexCoord;
	vsOut.wpos = skinOut.wpos;
	vsOut.wnormal = skinOut.wnormal;

	//mygl_TexCoord0 = VsInTexCoord;
	//mygl_WPos = vsOut.wpos;
	//mygl_WNormal = vsOut.wnormal;

	//gl_TexCoord[0] = vec4(VsInTexCoord.xy, 0.0, 0.0);

#if 1
	//gl_Position = vsOut.pos;
#else
	gl_Position = UniWorldViewProj * vsIn.pos; // ダミーコード。

	vec3 norm = normalize((UniWorldViewProj * vsIn.normal).xyz); // ダミーコード。
	CalcLighting(vsOut.color, norm); // ダミーコード。
#endif

#if 1
	//gl_FrontColor = vsOut.color;
#else
	gl_FrontColor = vsOut.color + VsInBoneIndices0 * VsInBoneWeights0 * 0.000001; // ダミーコード。
#endif
}
