// Only UTF-8 or ASCII is available.

// ランバート ディフューズ係数。
// ライト ベクトル light は、各頂点から光源へ向かうベクトル。
// ワールド座標系における頂点法線との内積でディフューズ係数が決まる。
float CalcLambertDiffuse(float3 wnormal, float3 light)
{
	return max(dot(wnormal, light), 0);
	//return saturate(dot(light, wnormal));
}

// ハーフ ランバートは、従来のランバートの欠点を少しだけ改良し、やや疑似 GI ライクな陰影になる。Valve Software によって開発されたフェイク技法。
float CalcHalfLambertDiffuse(float3 wnormal, float3 light)
{
	// Half Lambert.

	const float lambert = CalcLambertDiffuse(wnormal, light) * 0.5f + 0.5f;
	return lambert * lambert;
}

// スペキュラー反射をモデル化するには、光の進行方向だけでなく、視点からの視線の方向もレンダリング システムに知らせる必要がある。
// Phong スペキュラーにハーフ ベクトル（ハーフウェイ ベクトル）を組み合わせたシェーダーは、Blinn-Phong と呼ばれる。
float CalcBlinnPhongSpecular(float3 wnormal, float3 halfway, float specularPower)
{
	// HLSL の pow() 関数は x と y の両方にゼロが与えられた場合、動作未定義となるらしい。
	// シェーダー側で条件分岐するのはコストが高いので、C++ 側で対処する。
	return pow(max(dot(wnormal, halfway), 0), specularPower);
	//return pow(saturate(dot(wnormal, halfway)), specularPower);
}

// https://spphire9.wordpress.com/2013/03/13/webgl%E3%81%A7cook-torrance/
// Beckmann 分布。
float CalcBeckmannDistribution(float microfacet, float dotNH)
{
	const float d2 = dotNH * dotNH;
	const float m2 = microfacet * microfacet;
	return exp((d2 - 1.0) / (d2 * m2)) / (m2 * d2 * d2);
}

float CalcBeckmannDistribution(float microfacet, float3 wnormal, float3 halfway)
{
	return CalcBeckmannDistribution(microfacet, dot(wnormal, halfway));
}

// Unreal Engine 4 では Schlick 近似ではなくガウシアン球を使った近似を使っているらしい？
// http://d.hatena.ne.jp/hanecci/20130727/p2
// http://blog.selfshadow.com/publications/s2013-shading-course/#course_content

float CalcSchlickFresnelTerm(float fresnel0, float dotLH)
{
	return fresnel0 + (1.0 - fresnel0) * pow(1.0 - dotLH, 5.0);
}

float CalcSchlickFresnelTerm(float fresnel0, float3 light, float3 halfway)
{
	return CalcSchlickFresnelTerm(fresnel0, dot(light, halfway));
}

float3 CalcSchlickFresnelTerm(float3 fresnel0, float dotLH)
{
	// HLSL ではスカラー演算式をベクトル演算式にそのまま拡張できる（スカラーリテラルはベクトルに暗黙変換される）。
	// C++ のように関数オーバーロードも定義可能。
	// GLSL/GLSL ES でも一応サポートされている模様。
	// これでテンプレートもサポートしてくれれば完璧なんだが……
#if 0
	return float3(
		CalcSchlickFresnelTerm(fresnel0.r, dotLH),
		CalcSchlickFresnelTerm(fresnel0.g, dotLH),
		CalcSchlickFresnelTerm(fresnel0.b, dotLH));
#else
	return fresnel0 + (1.0 - fresnel0) * pow(1.0 - dotLH, 5.0);
#endif
}

float3 CalcSchlickFresnelTerm(float3 fresnel0, float3 light, float3 halfway)
{
	return CalcSchlickFresnelTerm(fresnel0, dot(light, halfway));
}

float3 CalcCookTorranceSpecular(float3 wnormal, float3 light, float3 eye, float3 halfway, float3 specularColor, float microfacet)
{
	// TODO: microfacet は、マテリアルパラメータ名としては roughness にする？

	const float dotLH = dot(light, halfway);
#if 0
	const float dotNH = dot(wnormal, halfway);
	const float dotNL = dot(wnormal, light);
	const float dotNV = dot(wnormal, eye);
	const float dotVH = dot(eye, halfway);
#else
	const float dotNH = saturate(dot(wnormal, halfway));
	const float dotNL = saturate(dot(wnormal, light));
	const float dotNV = saturate(dot(wnormal, eye));
	const float dotVH = saturate(dot(eye, halfway));
#endif

	// Cook-Torrance
	const float3 fresnelTerm = CalcSchlickFresnelTerm(specularColor, dotLH);
	//const float3 fresnelTerm = CalcSchlickFresnelTerm(specularColor, dotVH);
	const float dist = CalcBeckmannDistribution(microfacet, dotNH);
	const float temp = 2.0 * dotNH / dotVH;
	const float geometricAttenuation = min(1.0, min(temp * dotNV, temp * dotNL));
	const float3 spe = max(fresnelTerm * dist * geometricAttenuation / (F_PI * dotNV * dotNL), 0.0);
	return spe;
}

// TODO: 他、オーレン・ネイヤー反射などの実装も検討する。
