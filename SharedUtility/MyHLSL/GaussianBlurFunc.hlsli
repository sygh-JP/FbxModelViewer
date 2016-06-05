// Only UTF-8 or ASCII is available.

// ガウスぼかしの関数テンプレートもどき。
// インクルード直前にぼかしレベルごとに異なる名前をマクロで与えることで複数の関数定義を行なう。
// 重み配列は事前計算して、あらかじめ定数配列として定義しておく。

//#define MY_GAUSSIAN_SAMPLING_SRC_TEX_ELEM_TYPE float4

#ifndef MY_GAUSSIAN_BLUR_FUNC_HEADER_INCLUDED
#define MY_GAUSSIAN_BLUR_FUNC_HEADER_INCLUDED

//typedef float4 MY_GAUSSIAN_SAMPLING_SRC_TEX_ELEM_TYPE;
// Texture2D/Texture2DMS などの Type に、typedef された型を使うと、X3017 のコンパイル エラーになってしまう。
// Texture2D<Type> など自体を typedef する必要がありそう。

#endif

// バイリニア補間する場合は常に半テクセルのずれを考慮する必要がある。
// 入力テクスチャと出力テクスチャのサイズがまったく同じであれば（スケーリングが一切なければ）、
// バイリニア サンプラーでなくニアレストネイバーでもよさげ。
// 1対1でマッピングできない場合は、うかつにニアレストネイバーを使ってはいけない。
// http://msdn.microsoft.com/ja-jp/library/bb147229.aspx
// http://msdn.microsoft.com/ja-jp/library/bb219690.aspx


float4 MY_GAUSSIAN_BLUR_HORIZONTAL_FUNC_NAME(
	float texSizeX, float2 tex, SamplerState linearClamp,
	Texture2D srcTex)
{
	// 隣接ではなく1つ飛ばし。
	const float stepSize = 2.0;
	float4 color = MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[0] * srcTex.Sample(linearClamp, tex);
	[unroll]
	for (int i = 1; i < MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT; ++i)
	{
#if 1
		const float r = stepSize * float(i) / texSizeX;
		const float2 offset = float2(r, 0);
		color += MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[i] * (
			srcTex.Sample(linearClamp, tex + offset) +
			srcTex.Sample(linearClamp, tex - offset));
#else
		// 引数指定による offset は -8～7 の範囲にないとダメらしい。カーネルが大きい場合は使えない。
		color += MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[i] * (
			srcTex.Sample(linearClamp, tex, int2(+i, 0)) +
			srcTex.Sample(linearClamp, tex, int2(-i, 0)));
#endif
	}
	return color;
}

float4 MY_GAUSSIAN_BLUR_VERTICAL_FUNC_NAME(
	float texSizeY, float2 tex, SamplerState linearClamp,
	Texture2D srcTex)
{
	// 隣接ではなく1つ飛ばし。
	const float stepSize = 2.0;
	float4 color = MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[0] * srcTex.Sample(linearClamp, tex);
	[unroll]
	for (int i = 1; i < MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT; ++i)
	{
#if 1
		const float r = stepSize * float(i) / texSizeY;
		const float2 offset = float2(0, r);
		color += MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[i] * (
			srcTex.Sample(linearClamp, tex + offset) +
			srcTex.Sample(linearClamp, tex - offset));
#else
		// 引数指定による offset は -8～7 の範囲にないとダメらしい。カーネルが大きい場合は使えない。
		color += MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[i] * (
			srcTex.Sample(linearClamp, tex, int2(0, +i)) +
			srcTex.Sample(linearClamp, tex, int2(0, -i)));
#endif
	}
	return color;
}

// Undef by itself.
#undef MY_GAUSSIAN_BLUR_HORIZONTAL_FUNC_NAME
#undef MY_GAUSSIAN_BLUR_VERTICAL_FUNC_NAME
#undef MY_GAUSSIAN_WEIGHTS_ARRAY_NAME
#undef MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT
//#undef MY_GAUSSIAN_SAMPLING_SRC_TEX_ELEM_TYPE
