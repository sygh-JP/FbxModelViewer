// Only UTF-8 or ASCII is available.

#if 0
struct SkinningProcInfo
{
	float4 pos;
	float3 normal;
};

// XNA ACL の名残。
// This method takes in a vertex and applies the bone transforms to it.
SkinningProcInfo Skin4(const MySkinVertex skinInput)
{
	SkinningProcInfo skinOutput = (SkinningProcInfo)0;

	// Since the weights need to add up to one, store 1.0 - (sum of the weights)
	float lastWeight = 1.0;
	float weight = 0.0;
	// Apply the transforms for the first 3 weights
	for (int i = 0; i < 3; ++i)
	{
		weight = skinInput.weights[i];
		lastWeight -= weight;
		skinOutput.pos += mul(skinInput.pos, MatrixPalette[skinInput.indices[i]]) * weight;
		skinOutput.normal += mul(skinInput.normal, (float3x3)MatrixPalette[skinInput.indices[i]]) * weight;
	}
	// Apply the transform for the last weight
	skinOutput.pos += mul(skinInput.pos, MatrixPalette[skinInput.indices[3]]) * lastWeight;
	skinOutput.normal += mul(skinInput.normal, (float3x3)MatrixPalette[skinInput.indices[3]]) * lastWeight;
	return skinOutput;
}
#endif

// スキニング処理（モデル原点に対するローカル変換）。
void DoMatrixSkinning(out float4 outPos, out float3 outNormal, const VS_INPUT_SKIN skinInput)
{
	outPos = (float4)0;
	outNormal = (float3)0;

	// スキニング頂点ストリームに4次元ベクトル1個を使う場合、1頂点あたりのボーン影響度の最大個数は4とする。
	// また、影響度の総和は1であることが前提。
	// ベクトル2つを使う場合、最大個数は8まで拡張できる。
	// HLSL の mul() 関数は GLSL の乗算演算子オーバーロードとは違って float3 と float3x3 を直接演算できるようになっているが、
	// float4 と float4x4 の乗算結果の w 成分を無視するだけで演算コスト的には変わらないはず。
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		const float weight0 = skinInput.weights0[i];
		const int index0 = skinInput.indices0[i];
		outPos += mul(skinInput.pos, UniBoneMatrixPalette[index0]) * weight0;
		outNormal += mul(skinInput.normal, (float3x3)UniBoneMatrixPalette[index0]) * weight0;

		const float weight1 = skinInput.weights1[i];
		const int index1 = skinInput.indices1[i];
		outPos += mul(skinInput.pos, UniBoneMatrixPalette[index1]) * weight1;
		outNormal += mul(skinInput.normal, (float3x3)UniBoneMatrixPalette[index1]) * weight1;
	}
}

// 行列ではなくクォータニオンを使う場合は下記を参考に。
// http://blogs.msdn.com/b/ito/archive/2009/05/05/more-bones-05.aspx
