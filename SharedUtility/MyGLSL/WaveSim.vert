//#version 440
//#version 330

#if 0
uniform mat4 UniWorldViewProj;
uniform mat4 UniWorldView;
#endif

// OpenGL 3.1（GLSL 1.4）では Uniform Block に、OpenGL 3.2（GLSL 1.5）では In/Out Block に対応したが、
// GLSL 頂点シェーダーでは（例え 4.4 でも）Input ブロックを作成できないという仕様になっているらしく、
// 従来のように頂点属性（頂点レイアウト）の要素ごとに in 修飾子を付けて宣言していくしかなさそう。
// 構造体も in 変数として直接使えないので、HLSL からコードを移植する場合はやや面倒。
// Output ブロックつまり頂点シェーダー→...→フラグメント シェーダーの連結部分のほうは比較的 HLSL から移植しやすい。
// http://www.opengl.org/wiki/Interface_Block_%28GLSL%29

layout(location = 0) in vec3 VsInPosition;
layout(location = 1) in vec4 VsInColor;
layout(location = 2) in vec3 VsInNormal;
layout(location = 3) in vec2 VsInTexCoord;

// HLSL で使っている構造体と同じものを定義して、パッキングし直す。
// GLSL コンパイラが必要に応じて最適化（インライン化）するので、単純かつ冗長なコピーのオーバーヘッドは取り除かれるはず。
struct VS_INPUT_PCNT
{
	vec3 pos;
	vec4 color;
	vec3 normal;
	vec2 tex;
};

out VS_OUTPUT_WAVE
{
	vec4 color;
	vec3 normal;
	vec2 tex;
	vec3 wvpos;
} vsOut;

void main()
{
	VS_INPUT_PCNT vsIn = { VsInPosition, VsInColor, VsInNormal, VsInTexCoord };

	const float planeLength = 300;
	vec4 vPosition4 = vec4(vsIn.pos.x * planeLength, vsIn.pos.y, vsIn.pos.z * planeLength, 1.0);
	gl_Position = UniWorldViewProj * vPosition4;

	vsOut.color = vsIn.color;
	vsOut.normal = vsIn.normal;
	vsOut.tex = vsIn.tex;
	vsOut.wvpos = (UniWorldView * vPosition4).xyz;
}

// GLSL 1.4 以降、すなわち OpenGL 3.1 以降では、
// HLSL でのセマンティクスによく似た、
// gl_InstanceID
// というインスタンス描画用の組み込み変数が追加になっている。
// gl_VertexID
// に関しては GLSL 1.1 すなわち OpenGL 2.0 から使えたらしい。
