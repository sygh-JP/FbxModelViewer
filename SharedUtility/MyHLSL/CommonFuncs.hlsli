// Only UTF-8 or ASCII is available.

// HLSL の cross() 関数は float3 専用。
// シェーダーモデル 5.0 以降では HLSL で double を使えるが、Windows SDK 8.0, 8.1 に含まれる
// 現時点の fxc.exe では cross() 関数などに double 用のオーバーロードはない模様。
// double3 に対して cross() を使うと、float3 への暗黙の型変換による X3205 の警告が出る。
// GLSL の cross() 関数も vec3 専用だが、GLSL 4.0 以降は dvec3 もサポートする。

inline float CrossCCW(float2 a, float2 b)
{
	return a.x * b.y - a.y * b.x;
}

inline float CalcTriangleArea(float2 edgeA, float2 edgeB)
{
	return 0.5f * abs(CrossCCW(edgeA, edgeB)); // 外積の大きさは平行四辺形の面積になる。
}

inline float CalcTriangleArea(float3 edgeA, float3 edgeB)
{
	return 0.5f * length(cross(edgeA, edgeB)); // 外積の大きさは平行四辺形の面積になる。
}


inline float3x3 CreateRotationMatrix3x3Z(float theta)
{
	float sint, cost;
	sincos(theta, sint, cost);
	const float3x3 retVal =
	{
		{ cost, sint, 0 },
		{ -sint, cost, 0 },
		{ 0, 0, 1 },
	};
	return retVal;
}

inline float3x3 CreateRotationMatrix3x3Z(float theta, float2 center)
{
	float sint, cost;
	sincos(theta, sint, cost);
	const float3x3 retVal =
	{
		{ cost, sint, 0 },
		{ -sint, cost, 0 },
		{ center.x * (1 - cost) + center.y * sint, -center.x * sint + center.y * (1 - cost), 1 },
	};
	return retVal;
}

float4 CreateRotationQuaternion(float ax, float ay, float az, float theta)
{
	float sint, cost;
	sincos(0.5 * theta, sint, cost);
	return float4(sint * ax, sint * ay, sint * az, cost);
}

// HACK: 列優先？行優先？
float4x4 CreateMatrixFromRotationQuaternion(const in float4 quat)
{
	const float4 quat2 = 2.0 * quat;

	const float xx = quat.x * quat2.x;
	const float xy = quat.x * quat2.y;
	const float xz = quat.x * quat2.z;
	const float yy = quat.y * quat2.y;
	const float yz = quat.y * quat2.z;
	const float zz = quat.z * quat2.z;
	const float wx = quat.w * quat2.x;
	const float wy = quat.w * quat2.y;
	const float wz = quat.w * quat2.z;

	float4x4 dest;
	dest[0][0] = 1.0 - (yy + zz);
	dest[1][0] = xy + wz;
	dest[2][0] = xz - wy;
	dest[3][0] = 0.0;

	dest[0][1] = xy - wz;
	dest[1][1] = 1.0 - (xx + zz);
	dest[2][1] = yz + wx;
	dest[3][1] = 0.0;

	dest[0][2] = xz + wy;
	dest[1][2] = yz - wx;
	dest[2][2] = 1.0 - (xx + yy);
	dest[3][2] = 0.0;

	dest[0][3] = 0.0;
	dest[1][3] = 0.0;
	dest[2][3] = 0.0;
	dest[3][3] = 1.0;

	return dest;
}

float4 JointQuaternion(const in float4 q1, const in float4 q2)
{
	// XMQuaternionMultiply() を参照のこと。
	return float4(
		(q2.w * q1.x) + (q2.x * q1.w) + (q2.y * q1.z) - (q2.z * q1.y),
		(q2.w * q1.y) - (q2.x * q1.z) + (q2.y * q1.w) + (q2.z * q1.x),
		(q2.w * q1.z) + (q2.x * q1.y) - (q2.y * q1.x) + (q2.z * q1.w),
		(q2.w * q1.w) - (q2.x * q1.x) - (q2.y * q1.y) - (q2.z * q1.z));
}

// クォータニオンの球面線形補間。
// 2つのクォータニオンが正規化されていることが前提。
// クォータニオンの正規化は、普通に float4 を正規化するだけ。
float4 SlerpQuaternion(const in float4 q1, const in float4 q2, const in float t)
{
	const float cosOmega = dot(q1, q2);
	const float sinOmega = sqrt(1.0 - cosOmega * cosOmega);

	static const float eps = 1.0e-5;

	// HACK: シェーダーで動的分岐するのはパフォーマンス上問題があるので、DirectXMath の XMQuaternionSlerpV() を参考にして分岐をなくすべき。
	// ロジック上どうしても必要な場面以外は分岐は避けるべき。単純な算術演算ごときに分岐は使わない。
	// もしかしたら動的分岐ではなく、まず両方実行して最後に線形補間するコードを生成してくれるのかもしれない。
	// http://msdn.microsoft.com/ja-jp/library/bb944006.aspx
	// ちなみに浮動小数のゼロ除算は必ずしも未定義動作ではなく、分子が非ゼロであれば発散するだけだが、分子もゼロだと NaN を引き起こす。
	if (abs(sinOmega) < eps)
	{
		return q1; // 2つのクォータニオンがなす角度がゼロの場合。正規化されているならば、q1 or q2 のどちらでもよいはず。
	}

	const float omega = atan2(sinOmega, cosOmega);
	const float ratio1 = sin((1.0 - t) * omega);
	const float ratio2 = sin(t * omega);

	return (q1 * ratio1 + q2 * ratio2) / sinOmega;
}
