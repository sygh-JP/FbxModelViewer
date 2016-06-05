// Only UTF-8 or ASCII is available.

// コンピュートシェーダーによる移動平均フィルター。

#ifndef MY_MOVING_AVERAGE_FILTER_CS_HEADER_INCLUDED
#define MY_MOVING_AVERAGE_FILTER_CS_HEADER_INCLUDED

// 32KB（float2 の場合は要素数 4096）までであれば、タイリングは不要。
groupshared float2 MovingAverageCsTempLine[MY_MOVING_AVERAGE_CS_TEMP_LINE_MAX_SIZE]; // TLS

#endif

// Texture2DArray 中の1スライスだけ Texture2D としてシェーダー側でオブジェクト的に取り出すことができればコードの再利用がはかどるのだが……

void MY_MOVING_AVERAGE_FILTER_CS_FUNC_NAME(
	int x, int y, int slice,
	int texWidth,
	Texture2DArray<float2> inTex, RWTexture2DArray<float2> outTex)
{
#if 0
	const float2 srcColor = inTex[int3(x, y, slice)];
	MovingAverageCsTempLine[x] = srcColor;
	GroupMemoryBarrierWithGroupSync();

	float2 color = MovingAverageCsTempLine[x];
	[unroll]
	for (int i = 1; i <= MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT / 2; ++i)
	{
		// HACK: 隣接ではなく1つ飛ばしにする？
		const int offsetP = clamp(x + i, 0, texWidth - 1);
		const int offsetN = clamp(x - i, 0, texWidth - 1);
		color +=
			MovingAverageCsTempLine[offsetP] +
			MovingAverageCsTempLine[offsetN];
	}
	outTex[int3(y, x, slice)] = color / float(MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT);
#else
	float2 color = inTex[int3(x, y, slice)];
	[unroll]
	for (int i = 1; i <= MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT / 2; ++i)
	{
		// HACK: 隣接ではなく1つ飛ばしにする？
		const int offsetP = clamp(x + i, 0, texWidth - 1);
		const int offsetN = clamp(x - i, 0, texWidth - 1);
		color +=
			inTex[int3(offsetP, y, slice)] +
			inTex[int3(offsetN, y, slice)];
	}
	outTex[int3(y, x, slice)] = color / float(MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT);
#endif
}

#undef MY_MOVING_AVERAGE_FILTER_CS_FUNC_NAME
#undef MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT
