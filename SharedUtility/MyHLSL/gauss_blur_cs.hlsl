// http://developer.amd.com/wordpress/media/2012/10/Shader%20Model%205-0%20and%20Compute%20Shader.pps
// cs_5_0 is required.

groupshared float4 HorizontalLine[WIDTH]; // TLS
Texture2D txInput; // Input texture to read from
RWTexture2D<float4> OutputTexture; // Tmp output

// Invoke by Dispatch(1, HEIGHT, 1)
[numthreads(WIDTH, 1, 1)]
void GausBlurHoriz(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID)
{
	// Fetch color from input texture
	float4 vColor = txInput[int2(groupThreadID.x, groupID.y)];
	// Store it into TLS
	HorizontalLine[groupThreadID.x] = vColor;
	// Synchronize threads
	GroupMemoryBarrierWithGroupSync();

	// Compute horizontal Gaussian blur for each pixel
	vColor = float4(0,0,0,0);
	[unroll]
	for (int i = -GS2; i <= GS2; ++i)
	{
		// Determine offset of pixel to fetch
		int nOffset = groupThreadID.x + i;
		// Clamp offset
		nOffset = clamp(nOffset, 0, WIDTH - 1);
		// Add color for pixels within horizontal filter
		vColor += G[GS2 + i] * HorizontalLine[nOffset];
	}
	// Store result
	OutputTexture[int2(groupThreadID.x, groupID.y)] = vColor;
}
