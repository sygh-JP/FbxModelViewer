#version 430
#extension GL_ARB_compute_variable_group_size : enable

#if 0
layout(std430, binding = 0) buffer StorageInputBuffer0F4
{
	readonly vec4 InputBuffer0F4[];
};

layout(std430, binding = 1) buffer StorageOutputBuffer0F4
{
	writeonly vec4 OutputBuffer0F4[];
};

layout(local_size_variable) in;
void main()
{
	uint index = gl_GlobalInvocationID.x;
	OutputBuffer0F4[index] = InputBuffer0F4[index];
}
#endif

#define COMPUTING_TEMP_WORK_SIZE  (256)
// 定数バッファで渡す？
// GLSL 4.3 の imageSize() を使ってシェーダー側でサイズ取得する？
// ちなみに sampler2D などの場合は textureSize() になる。

const float Spring = 0.03;

const vec2 AddWavePos = { 0.6, 0.3 }; // TODO: クリック位置にする。
const float AddWaveRad = 0.01; // TODO: [0, 1] の乱数を乗じる。
//const float AddWaveVelocity = 1.0; // TODO: [-0.5, +0.5] の乱数を乗じる。
const float AddWaveVelocity = 2.0; // TODO: [-0.5, +0.5] の乱数を乗じる。

const vec4 ZeroVector4F = { 0.0, 0.0, 0.0, 0.0 };

void MyDeclTestFunc(in sampler2D inTex) {}

// HLSL で RWTexture2D を Texture2D<float4> として関数パラメータに直接渡せないのと同様に、
// 実引数に応じた qualifier を仮引数側でも詳細に指定する必要があるらしい。
// 2014年1月時点で GLSL コンピュート シェーダーのサンプルが非常に少なく、
// OpenGL 仕様のドキュメントも分かりづらいので、
// GLSL コンパイラーのエラーメッセージを頼りに手探りで修正していくほうがまだ楽。
vec4 FetchHeightVelocityNormalMap(layout(rgba32f) readonly image2D inTex, ivec2 index, ivec2 size)
{
	// 中心点と隣接する上下左右4点をサンプリング。
	vec4 t0 = imageLoad(inTex, index);
	vec4 t1 = (index.x - 1 >= 0) ?
		imageLoad(inTex, ivec2(index.x - 1, index.y)) : ZeroVector4F;
	vec4 t2 = (index.x + 1 < size.x) ?
		imageLoad(inTex, ivec2(index.x + 1, index.y)) : ZeroVector4F;
	vec4 t3 = (index.y - 1 >= 0) ?
		imageLoad(inTex, ivec2(index.x, index.y - 1)) : ZeroVector4F;
	vec4 t4 = (index.y + 1 < size.y) ?
		imageLoad(inTex, ivec2(index.x, index.y + 1)) : ZeroVector4F;

	// 速度の計算。
	float velocity = t0.y + Spring * (t1.x + t2.x + t3.x + t4.x - t0.x * 4.0);

	// 位置の計算。
	float height = t0.x + velocity;

	// 波の追加。
	vec2 pos = { float(index.x) / (size.x - 1), float(index.y) / (size.y - 1) };
	if (distance(AddWavePos, pos) <= AddWaveRad)
	{
		velocity += AddWaveVelocity;
	}

	// 新しい位置と速度、傾きの出力。
	return vec4(height, velocity, (t2.x - t1.x) * 0.5, (t4.x - t3.x) * 0.5);
}

// http://github.prideout.net/modern-opengl-prezo/

layout(binding = 0, rgba32f) uniform readonly image2D UniWaveSimInputImage;
layout(binding = 1, rgba32f) uniform writeonly image2D UniWaveSimOutputImage;
// HLSL の RWTexture2D や CUDA の Surface に相当するのは readwrite 修飾子。
// ちなみに OpenCL は 2.0 でようやく __read_write が実装されたらしい。
// OpenCL 2.0 対応の GPU は2014年5月時点ではまだ存在しない？　AMD Kaveri 世代の APU は対応しているらしいが……

layout(local_size_variable) in;
void main()
{
	ivec2 index = { int(gl_WorkGroupID.x), int(gl_LocalInvocationID.x) };
	//ivec2 size = imageSize(UniWaveSimOutputImage); // writeonly の場合 imageSize() が使えないらしい。なんだそりゃ……
	ivec2 size = imageSize(UniWaveSimInputImage);
	if (index.x < size.x && index.y < size.y)
	{
		imageStore(UniWaveSimOutputImage, index,
			FetchHeightVelocityNormalMap(UniWaveSimInputImage, index, size));
	}

	// テスト用のダミーコード。
	//vec4 input = imageLoad(UniWaveSimInputImage, ivec2(0, 0));
	//imageStore(UniWaveSimOutputImage, ivec2(0, 0), input);
}
