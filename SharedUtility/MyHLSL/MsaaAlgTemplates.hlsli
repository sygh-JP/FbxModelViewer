// Only UTF-8 or ASCII is available.

// MSAA 関連アルゴリズムの関数テンプレートもどき。
// インクルード直前に MSAA レベルごとに異なる名前をマクロで与えることで複数の関数定義を行なう。

#define MY_MSAA_ALG_TEX_ELEM_TYPE float4

#ifndef MY_MSAA_ALG_TEMPLATES_HEADER_INCLUDED
#define MY_MSAA_ALG_TEMPLATES_HEADER_INCLUDED

//typedef float4 MY_MSAA_ALG_TEX_ELEM_TYPE;

// 非 MSAA 版の関数は、この領域内で一度だけ定義されるようにする。
// C++ とは違い、inline 指定しただけでは関数再定義のエラーとなるので注意。
// typedef も複数回記述すると再定義エラー X3003 になってしまう（たとえ同じ型の typedef だったとしても）。

float4 TransportFromNonMSAATex(
	int2 size, float2 tex,
	Texture2D<MY_MSAA_ALG_TEX_ELEM_TYPE> srcTex)
{
	const int3 sampleCoord = int3(tex.x * size.x, tex.y * size.y, 0);
	return srcTex.Load(sampleCoord);
}

#endif

// H/W が D3D 10.1 (シェーダーモデル 4.1) に対応していない場合でも使えるダウンサンプル手法。
// ちなみに Texture2DMS には Sample() メソッドがない。
float4 MY_MSAA_DOWN_SAMPLE_FUNC_NAME(
	int2 size, float2 tex,
	Texture2DMS<MY_MSAA_ALG_TEX_ELEM_TYPE, MY_MSAA_ALG_TEX_SAMPLE_COUNT> srcTex)
{
	// サイズの取得には GetDimensions() を使う方法もある。
	const int2 sampleCoord = int2(tex.x * size.x, tex.y * size.y);
	float4 color = { 0, 0, 0, 0 };

	[unroll] // warning X3582 対策の属性。
	for (int i = 0; i < MY_MSAA_ALG_TEX_SAMPLE_COUNT; ++i)
	{
		color += srcTex.Load(sampleCoord, i);
	}
	// 平均値を求める。
	return color / MY_MSAA_ALG_TEX_SAMPLE_COUNT;
}

// SM 4.1 向けのオプション。4.1 非対応のハードウェア向けコードと比べて、どちらが高速に動作する？
#ifdef MY_SM41_MSAA_DOWN_SAMPLE_FUNC_NAME
float4 MY_SM41_MSAA_DOWN_SAMPLE_FUNC_NAME(
	Texture2DMS<MY_MSAA_ALG_TEX_ELEM_TYPE, MY_MSAA_ALG_TEX_SAMPLE_COUNT> srcTex)
{
	// H/W が D3D 10.1 (シェーダーモデル 4.1) 以上に対応している場合。
	int2 sampleCoord;
	float4 color = { 0, 0, 0, 0 };

	[unroll]
	for (int i = 0; i < MY_MSAA_ALG_TEX_SAMPLE_COUNT; ++i)
	{
		// GetSamplePosition() は ps_4_1 (D3D 10.1) 以降でサポートされている。
		// http://game.watch.impress.co.jp/docs/20080219/d3d.htm
		sampleCoord = srcTex.GetSamplePosition(i);
		color += srcTex.Load(sampleCoord, i);
	}
	return color / MY_MSAA_ALG_TEX_SAMPLE_COUNT;
}
#undef MY_SM41_MSAA_DOWN_SAMPLE_FUNC_NAME
#endif

// Undef by itself.
#undef MY_MSAA_DOWN_SAMPLE_FUNC_NAME
#undef MY_MSAA_ALG_TEX_ELEM_TYPE
#undef MY_MSAA_ALG_TEX_SAMPLE_COUNT
