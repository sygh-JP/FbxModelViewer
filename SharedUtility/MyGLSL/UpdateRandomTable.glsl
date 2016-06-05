#version 430
#extension GL_ARB_compute_variable_group_size : enable

// OpenGL コンピュート シェーダーにおいて、HLSL StructuredBuffer/RWStructuredBuffer に相当するのは、
// Shader Storage Buffer Object（SSBO）と呼ばれるものらしい。
// 対応するインターフェイス ブロックつまり Shader Storage Block の作成は Uniform Buffer Object（UBO）の Uniform Block によく似ていて、
// uniform の代わりに buffer キーワードを使う（これは分かりにくい……storage のほうがいいのでは……）。
// ちなみに下記の Wiki にはなぜかゼロ幅の空白（UCS-2 U+200B）が多数使われている。
// Visual Studio IDE のエディターにコードをコピー＆ペーストするときは要注意。
// http://www.opengl.org/wiki/Shader_Storage_Buffer_Object
// http://www.opengl.org/wiki/Interface_Block_(GLSL)
// GLSL コンピュート シェーダー用組み込み変数と HLSL コンピュート シェーダー（DirectCompute）用システム値セマンティクスとの対比は下記に記載がある。
// OpenGL の場合はローカル グループ サイズをホスト側で実行時に指定することもできるため、HLSL には対応するものが存在しない組み込み変数もある。
// http://www.opengl.org/registry/specs/ARB/compute_shader.txt
//  OpenGL Compute             DirectCompute
//  --------------------------------------------------
//  gl_NumWorkGroups           --
//  gl_WorkGroupSize           --
//  gl_WorkGroupID             SV_GroupID
//  gl_LocalInvocationID       SV_GroupThreadID
//  gl_GlobalInvocationID      SV_DispatchThreadID
//  gl_LocalInvocationIndex    SV_GroupIndex

// なお、RWTexture2D に相当するのは image2D らしい。

layout(std430, binding = 0) buffer SBufferRandomNumTable
{
	uvec4 SsboRandomNumTable[];
};

#if 0
layout(std140) buffer foo
{
	int ary[16];
} bar;
#endif

uvec4 Xorshift128RandomCreateNext(uvec4 random)
{
	uint t = (random.x ^ (random.x << 11));
	random.x = random.y;
	random.y = random.z;
	random.z = random.w;
	random.w = (random.w = (random.w ^ (random.w >> 19)) ^ (t ^ (t >> 8)));
	return random;
}

#if 0
#define ComputingThreadLocalGroupSizeX  (256)
#define ComputingThreadLocalGroupSizeY  (1)
#elif 0
const uint ComputingThreadLocalGroupSizeX = 256;
const uint ComputingThreadLocalGroupSizeY = 1;
#endif

// NVIDIA の OpenGL 4.3 対応ドライバー 311.35 だとコンピュート シェーダーの layout 修飾子パラメータ右辺値に
// マクロ定義した定数シンボルが使えないというバグがある。4.4 対応ドライバー 331.82 では修正されている模様。
// ちなみに const 定数は相変わらず属性値として使えないという GLSL のダメ仕様は健在。

// ローカル サイズの指定は省略すると1になる。ただし local_size_x だけは省略できないらしい。
//layout(local_size_x = ComputingThreadLocalGroupSizeX, local_size_y = ComputingThreadLocalGroupSizeY) in;
layout(local_size_variable) in;
void main()
{
	uint index = gl_GlobalInvocationID.x;
	uvec4 random = SsboRandomNumTable[index];
	SsboRandomNumTable[index] = Xorshift128RandomCreateNext(random);
}
