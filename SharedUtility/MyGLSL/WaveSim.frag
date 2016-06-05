//#version 440
//#version 330
//#include "MyUniformBlocks.glsl"

#if 0
uniform mat4 UniWorldView;

uniform vec4 UniMaterialColorDiffuse;
uniform vec4 UniMaterialColorAmbient;
uniform vec4 UniMaterialColorSpecular;
uniform vec4 UniMaterialColorEmissive;

uniform float UniMaterialOpacityAlpha;
uniform float UniMaterialSpecularPower;
uniform float UniMaterialReflectivity;
uniform float UniMaterialIndexOfRefraction;

uniform vec3 UniLightDir;
uniform vec3 UniLightPos;
uniform vec4 UniLightColor;
uniform vec4 UniAmbientLight;
#endif

uniform sampler2D UniWaveSimResultTexture;

in VS_OUTPUT_WAVE
{
	vec4 color;
	vec3 normal;
	vec2 tex;
	vec3 wvpos;
} vsOut;

layout(location = 0) out vec4 FsOutColor;

//const vec4 WaveDiffuse = { 0.6, 0.8, 1.0, 0.7 }; // TODO: cbuffer 化。
//const vec3 WaveSpecular = { 1.6, 1.8, 2.0 }; // TODO: cbuffer 化。
//const float WaveSpecularPower = 4.0; // TODO: cbuffer 化。
//const vec3 WaveTestLightPos = { 100.0, 300.0, 100.0 }; // TODO: cbuffer 化。

void main(void)
{
	vec4 t = texture(UniWaveSimResultTexture, vsOut.tex);
	vec3 normal = normalize(cross(vec3(0.0, t.g, 1.0), vec3(1.0, t.r, 0.0)));

	//vec4 lightWVPos = UniWorldView * vec4(WaveTestLightPos, 1.0);
	vec4 lightWVPos = UniWorldView * vec4(UniLightPos, 1.0);

	vec3 lp = normalize(lightWVPos.xyz - vsOut.wvpos);
	vec3 wvNormal = normalize((UniWorldView * vec4(normal, 0.0)).xyz);
	float hl = dot(lp, normalize(reflect(vsOut.wvpos, wvNormal)));
	float diffuse = max(0.0, dot(wvNormal, lp));

	// HACK: 頂点カラーおよび頂点法線は GLSL 最適化の影響で頂点属性ロケーションの取得が失敗するのを防ぐため、仕方なく乗じている。
	vec3 color =
		vsOut.color.rgb * vsOut.normal.y *
		UniMaterialColorAmbient.rgb * UniAmbientLight.rgb +
		diffuse * UniMaterialColorDiffuse.rgb +
		pow(abs(hl), UniMaterialSpecularPower) * UniMaterialColorSpecular.rgb;

	FsOutColor = vec4(color * UniLightColor.rgb, UniMaterialOpacityAlpha);

	// テスト用のダミーコード。
	//FsOutColor = vsOut.color * vec4(vsOut.tex.x, vsOut.tex.y, vsOut.wvpos.z, vsOut.normal.y);
}
