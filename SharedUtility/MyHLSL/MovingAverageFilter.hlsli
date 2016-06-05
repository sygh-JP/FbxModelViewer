// Only UTF-8 or ASCII is available.

float2 MY_MOVING_AVERAGE_FILTER_HORIZONTAL_FUNC_NAME(
	float texSizeX, float3 tex, SamplerState linearClamp,
	Texture2DArray<float2> srcTex)
{
	float2 color = srcTex.Sample(linearClamp, tex);
	[unroll]
	for (int i = 1; i <= MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT / 2; ++i)
	{
		// HACK: Use "alternate" texels instead of "neighbor"?
#if 1
		const float r = float(i) / texSizeX;
		const float3 offset = float3(r, 0, 0);
		color +=
			srcTex.Sample(linearClamp, tex + offset) +
			srcTex.Sample(linearClamp, tex - offset);
#else
		// 引数指定による offset は -8～7 の範囲にないとダメらしい。カーネルが大きい場合は使えない。
		color +=
			srcTex.Sample(linearClamp, tex, int2(+i, 0)) +
			srcTex.Sample(linearClamp, tex, int2(-i, 0));
#endif
	}
	return color / float(MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT);
}

float2 MY_MOVING_AVERAGE_FILTER_VERTICAL_FUNC_NAME(
	float texSizeY, float3 tex, SamplerState linearClamp,
	Texture2DArray<float2> srcTex)
{
	float2 color = srcTex.Sample(linearClamp, tex);
	[unroll]
	for (int i = 1; i <= MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT / 2; ++i)
	{
#if 1
		const float r = float(i) / texSizeY;
		const float3 offset = float3(0, r, 0);
		color +=
			srcTex.Sample(linearClamp, tex + offset) +
			srcTex.Sample(linearClamp, tex - offset);
#else
		color +=
			srcTex.Sample(linearClamp, tex, int2(0, +i)) +
			srcTex.Sample(linearClamp, tex, int2(0, -i));
#endif
	}
	return color / float(MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT);
}

#undef MY_MOVING_AVERAGE_FILTER_HORIZONTAL_FUNC_NAME
#undef MY_MOVING_AVERAGE_FILTER_VERTICAL_FUNC_NAME
#undef MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT
