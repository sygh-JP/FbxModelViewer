// Only UTF-8 or ASCII is available.

// コンピュート シェーダーによるガウスぼかしの関数テンプレートもどき。


//#define MY_GAUSSIAN_SAMPLING_SRC_TEX_ELEM_TYPE float4

#ifndef MY_GAUSSIAN_BLUR_FUNC_CS_HEADER_INCLUDED
#define MY_GAUSSIAN_BLUR_FUNC_CS_HEADER_INCLUDED

// スレッド グループ用の TLS に相当する。
// 1つのシェーダー（カーネル関数）における groupshared 変数の合計最大サイズは D3D10 では 16KB、D3D11 では 32KB らしい。
// float4 ならば 2048 要素まで確保できる。
groupshared float4 GaussianBlurCsTempLine[MY_GAUSSIAN_BLUR_CS_TEMP_LINE_MAX_SIZE]; // TLS
// --> 水平ぼかし・垂直ぼかし両方で宣言を共有するため、ダウンサンプル テクスチャの縦横のサイズが異なる場合、より大きいほうを指定する。

#endif


void MY_GAUSSIAN_BLUR_CS_FUNC_NAME(
	int x, int y,
	int texWidth,
	Texture2D inTex, RWTexture2D<float4> outTex)
{
	// 隣接ではなく1つ飛ばし。
	const int stepSize = 2;
	// カーネル サイズが小さい場合、同期のオーバーヘッドのほうが大きく、共有メモリを使わないほうが返って高速？
	// Fermi 以降ではキャッシュがあるので、それが効いている可能性がある。
	// カーネル サイズが大きくても、それほど有意な差はなさそう？
	// http://news.mynavi.jp/articles/2010/07/21/fermi_cache/
#if 1
	// Fetch color from input texture
	const float4 srcColor = inTex[int2(x, y)];
	// Store it into TLS
	GaussianBlurCsTempLine[x] = srcColor;
	// Synchronize threads
	GroupMemoryBarrierWithGroupSync();
	// グループ内のすべてのスレッドが一時作業領域である TLS にいったんデータを書き込み終えるまで待つ。
	// その後、一斉にサンプリングと UAV への出力を行なう。
	// 二度手間に見えるかもしれないが、グローバル メモリーであるテクスチャから毎回参照するよりも
	// TLS への一時コピーを活用したほうが高速になるらしい。
	// 実際にガウスぼかしをピクセル シェーダーで実装するよりもコンピュート シェーダーで実装したほうが高速になる模様。
	// なお、ピクセル シェーダーは高速な TLS が使えないのが欠点だが、その代わりリソースやレンダーターゲットのサイズ変化に対して
	// 柔軟に対応するコードを簡単に書けるという利点がある。

	float4 color = MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[0] * GaussianBlurCsTempLine[x];
	[unroll]
	for (int i = 1; i < MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT; ++i)
	{
		// Determine offset of pixel to fetch
		const int offsetP = clamp(x + stepSize * i, 0, texWidth - 1);
		const int offsetN = clamp(x - stepSize * i, 0, texWidth - 1);
		// HLSL で整数を使うときは型に注意。
		// 符号なし型と符号付き型を加減算すると符号なし型になり、
		// また符号なし型を clamp() の第1引数に使うと、残りの型も符号なしとして推論される模様。

		color += MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[i] * (
			GaussianBlurCsTempLine[offsetP] +
			GaussianBlurCsTempLine[offsetN]);
	}

	// Store result
	outTex[int2(y, x)] = float4(color.rgb, 1);
	// 垂直方向にぼかすときにもまったく同じアルゴリズムが使えるよう、転置する。
	// 入力画像バッファ列方向へのアクセス（メモリ上では飛び飛びになる）がなくなるので、
	// ピクセル シェーダーと比べてキャッシュ ヒット的にも有利になる。
	// NOTE: ただし水平・垂直サイズに注意。
	// 水平・垂直ぼかしをペアで実行する（2回転置すれば向きが元に戻る）ことが前提。
	// 水平ぼかしの結果を書き込む一時バッファのサイズが、元の転置サイズになっていればこのコードのまま対応できるはず。
	// http://www.isus.jp/article/avx/iir-gaussian-blur-filter-implementation/
#else
	float4 color = MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[0] * inTex[int2(x, y)];
	[unroll]
	for (int i = 1; i < MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT; ++i)
	{
		const int offsetP = clamp(x + stepSize * i, 0, texWidth - 1);
		const int offsetN = clamp(x - stepSize * i, 0, texWidth - 1);

		color += MY_GAUSSIAN_WEIGHTS_ARRAY_NAME[i] * (
			inTex[int2(offsetP, y)] +
			inTex[int2(offsetN, y)]);
	}
	outTex[int2(y, x)] = float4(color.rgb, 1);
#endif
}

// Undef by itself.
#undef MY_GAUSSIAN_BLUR_CS_FUNC_NAME
#undef MY_GAUSSIAN_WEIGHTS_ARRAY_NAME
#undef MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT
//#undef MY_GAUSSIAN_SAMPLING_SRC_TEX_ELEM_TYPE
