// Only UTF-8 or ASCII is available.

#if 0
float4 SampleLinearClampStructuredBuffer(in StructuredBuffer<float4> inBuffer, uint2 size, float2 tex)
{
	const uint2 index = (tex * size) % size;

	const float4 t0 = inBuffer[index.y * size.x + index.x];
	const float4 t1 = (index.x + 1 < size.x)
		? inBuffer[index.y * size.x + (index.x + 1)]
		: (float4)0;
	const float4 t2 = (index.y + 1 < size.y)
		? inBuffer[(index.y + 1) * size.x + index.x]
		: (float4)0;
	const float4 t3 = ((index.x + 1 < size.x) && (index.y + 1 < size.y))
		? inBuffer[(index.y + 1) * size.x + (index.x + 1)]
		: (float4)0;

	const float2 f = frac(tex * size);
	const float4 t = lerp(lerp(t0, t1, f.x), lerp(t2, t3, f.x), f.y);
	return t;
}
#endif

MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE
	MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_NAME(
	in StructuredBuffer<MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE> inBuffer,
	uint2 size, float2 tex)
{
	const uint2 index = (tex * size) % size;

	const MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE t0 = inBuffer[index.y * size.x + index.x];
	const MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE t1 = (index.x + 1 < size.x)
		? inBuffer[index.y * size.x + (index.x + 1)]
		: (MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE)0;
	const MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE t2 = (index.y + 1 < size.y)
		? inBuffer[(index.y + 1) * size.x + index.x]
		: (MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE)0;
	const MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE t3 = ((index.x + 1 < size.x) && (index.y + 1 < size.y))
		? inBuffer[(index.y + 1) * size.x + (index.x + 1)]
		: (MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE)0;

	// Bilinear filtering.
	const float2 f = frac(tex * size);
	// The following code can handle all of the float1 - float4, int1 - int4, and uint1 - uint4.
	// However, it will be required to apply linear interpolation individually for the other types (such as a structure of some types).
	// By the way, it is better to use 1D/2D/3D texture rather than to use structured buffer when using float1 - float4 as an element type.
	const MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE t = lerp(lerp(t0, t1, f.x), lerp(t2, t3, f.x), f.y);
	return t;
}

// Undef by itself.
#undef MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_NAME
#undef MY_SAMPLE_LINEAR_CLAMP_STRUCTURED_BUFFER_FUNC_ELEM_TYPE
