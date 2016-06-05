//#version 330
//#include "DiffuseSpecularFunc.glsl"
//#include "MyUniformBlocks.glsl"

// 手軽にフォトリアルなローカル イルミネーションを実現できるシェーダーの代表例として、Blinn-Phong を使う。

//const uint TOON_SHADING_REF_TEX_SIZE = 128U;
const uint TOON_SHADING_REF_TEX_SIZE = 256U;

const float ToonShadingRefTexIniX = 0.5 / float(TOON_SHADING_REF_TEX_SIZE);

uniform sampler2D UniMainTexture;
uniform samplerCube UniEnvMapTexture;
uniform sampler2D UniToonShadingRefTexture;

// HACK: キューブマップなし（デフォルトの無名テクスチャをダミーとして使用、glBindTexture(GL_TEXTURE_CUBE_MAP, 0)）で
// samplerCube を使ってレンダリング（このときは texture() でゼロベクトルが返る模様）した後、
// 動的にキューブマップをロード＆バインドしてから再度同じシェーダーを使ってレンダリングすると、
// glDrawElements() のタイミングで ID=131218 の
// "Program/shader state performance warning: Fragment Shader is going to be recompiled because the shader key based on GL state mismatches."
// という警告メッセージが出力される。
// 初回だけ警告が表示された後は問題なく描画されるようになる模様だが、気持ち悪い。
// キューブマップの有無でシェーダーを完全に分けたほうがパフォーマンス的にも良いのは分かるが……
// 有効なテクスチャがバインドされてないとき（デフォルトの無名テクスチャが使用されるとき）に texture() で
// ゼロベクトルが返るという動作はベンダー依存かもしれないし、そもそも無名テクスチャは何もしないと Incomplete なものなので、
// やはりシェーダーを分けたほうがいいかも。
// もしくはダミーのキューブマップを用意しておく（6面用意するのは骨が折れるしメモリ的にもムダな感じだが……）。
// ダミーデータ方式は動的分岐ではないので、if 文などを駆使する Uber Shader よりはマシ。
// 屈折／反射のアルゴリズムを動的に切り替える場合のことも考えたら、動的シェーダーリンクや Shader Subroutine を使ったほうがいいかも。
// ただし、Shader Subroutine で分岐しても結局 samplerCube のバインドは実行されるので上記の警告は出る。
// http://stackoverflow.com/questions/15273674/binding-a-zero-texture-in-opengl
// http://stackoverflow.com/questions/17706153/do-glsl-4-x-subroutine-variables-cause-any-performance-overhead
// http://www.opengl.org/discussion_boards/showthread.php/182266-Do-GLSL-subroutines-cause-any-performance-overhead?p=1252779&viewfull=1#post1252779
// http://wlog.flatlib.jp/?itemid=1630
// なお、texture2D(), textureCube() は OpenGL 3.2 (GLSL 1.5) および OpenGL ES 3.0 で削除されたらしい。
// オーバーロードされた texture() として統合されている。
// NVIDIA の場合デスクトップ版の互換プロファイルでは #version 440 でも texture2D() はまだ使える模様だが、textureCube() は使えなくなる。
// ちなみに無名のオブジェクトは OpenGL 3.0 で非推奨になったらしい。
// http://www.asahi-net.or.jp/~YW3T-TRNS/opengl/version/index.htm

vec4 CalcLightingPhotoReal(float shadowingPercentLit, float diffuse, float specular, vec2 tex)
{
	float diffuse1 = 0.5 * (shadowingPercentLit * diffuse) + 0.5;
	float diffuse2 = diffuse1 * diffuse1;

	vec4 texDiffuseColor = texture(UniMainTexture, tex);

	vec4 color = vec4(
		(UniMaterialColorDiffuse.w * UniMaterialColorDiffuse * UniLightColor * diffuse2).rgb,
		UniMaterialOpacityAlpha);
	vec4 ambems = (UniMaterialColorAmbient.w * UniAmbientLight + UniMaterialColorEmissive.w);
	vec4 spc = UniMaterialColorSpecular.w * UniLightColor * specular;

	color.rgb += ambems.rgb;
	vec4 outColor = spc + saturate(color) * texDiffuseColor;
	outColor.a = saturate(outColor.a);
	return outColor;
}

vec4 CalcLightingToon(float shadowingPercentLit, float diffuse, float specular, vec2 tex)
{
	vec4 vDiffuse = texture(UniToonShadingRefTexture,
		vec2(ToonShadingRefTexIniX + shadowingPercentLit * diffuse, UniToonShadingRefTexV));

	vec4 texDiffuseColor = texture(UniMainTexture, tex);

	vec4 color = vec4(
		//(UniMaterialColorDiffuse.w * UniMaterialColorDiffuse * UniLightColor * vDiffuse).rgb,
		(UniMaterialColorDiffuse * UniLightColor * vDiffuse).rgb,
		UniMaterialOpacityAlpha);
	vec4 spc = UniMaterialColorSpecular.w * UniLightColor * specular;

	vec4 outColor = spc + saturate(color) * texDiffuseColor;
	outColor.a = saturate(outColor.a);
	return outColor;
}

// OpenGL 4.0 の Shader Subroutine を使って動的分岐してみる。
subroutine float4 TSubCalcMainLightingBase(float shadowingPercentLit, float diffuse, float specular, float2 tex); // インターフェイス宣言。
layout(location = 0) subroutine uniform TSubCalcMainLightingBase SubCalcMainLighting; // 関数の実体ではなくポインタのようなもの。

subroutine float4 TSubGetEnvMapColorBase(vec3 wnormal, vec3 light, vec3 eye, vec3 halfway); // インターフェイス宣言。
layout(location = 1) subroutine uniform TSubGetEnvMapColorBase SubGetEnvMapColor; // 関数の実体ではなくポインタのようなもの。
// TODO: reflect と refract を切り替えるのに使う。フレネル反射も実装する。

// サブルーチンのインデックスは種別ごとのインデックスではなく、すべてを通してのインデックスらしい。
layout(index = 0) subroutine(TSubCalcMainLightingBase) float4 SubCalcMainLightingPhotoReal(float shadowingPercentLit, float diffuse, float specular, float2 tex)
{
	return CalcLightingPhotoReal(shadowingPercentLit, diffuse, specular, tex);
}

layout(index = 1) subroutine(TSubCalcMainLightingBase) float4 SubCalcMainLightingToon(float shadowingPercentLit, float diffuse, float specular, float2 tex)
{
	return CalcLightingToon(shadowingPercentLit, diffuse, specular, tex);
}

layout(index = 2) subroutine(TSubGetEnvMapColorBase) float4 SubGetEnvMapColorDummy(vec3 wnormal, vec3 light, vec3 eye, vec3 halfway)
{
	return ZeroVector4F;
}

layout(index = 3) subroutine(TSubGetEnvMapColorBase) float4 SubGetEnvMapColorReflect(vec3 wnormal, vec3 light, vec3 eye, vec3 halfway)
{
	return texture(UniEnvMapTexture, reflect(eye, wnormal));
}

layout(index = 4) subroutine(TSubGetEnvMapColorBase) float4 SubGetEnvMapColorRefract(vec3 wnormal, vec3 light, vec3 eye, vec3 halfway)
{
	const float eta = 0.67; // 屈折率の比。
	return texture(UniEnvMapTexture, refract(eye, wnormal, eta));
}

layout(index = 5) subroutine(TSubGetEnvMapColorBase) float4 SubGetEnvMapColorFresnel(vec3 wnormal, vec3 light, vec3 eye, vec3 halfway)
{
	const float eta = 0.67; // 屈折率の比。
	const float fresnel0 = (1.0 - eta) * (1.0 - eta) / ((1.0 + eta) * (1.0 + eta)); // フレネル反射係数 F(0°) の計算。
	//const float border = fresnel0 + (1.0 - fresnel0) * pow(1.0 - dot(light, halfway), 5.0);
	const float border = CalcSchlickFresnelTerm(fresnel0, light, halfway);
	const float4 reflectVal = texture(UniEnvMapTexture, reflect(eye, wnormal));
	const float4 refractVal = texture(UniEnvMapTexture, refract(eye, wnormal, eta));
	return lerp(refractVal, reflectVal, border);
}

vec4 CalcLighting(vec3 wpos, vec3 wnormal, vec2 tex)
{
	vec3 light = normalize(UniLightPos - wpos);
	vec3 eye = normalize(UniEyePosition - wpos);
	vec3 halfway = normalize(light + eye);

	float diffuse = CalcLambertDiffuse(wnormal, light);
	float specular = CalcBlinnPhongSpecular(wnormal, halfway, UniMaterialSpecularPower);

	vec4 outColor0 = SubCalcMainLighting(1.0, diffuse, specular, tex);

	vec4 envMapColor = SubGetEnvMapColor(wnormal, light, eye, halfway);
	return lerp(outColor0, envMapColor, UniMaterialReflectivity);
}


//in vec2 mygl_TexCoord0;
//in vec3 mygl_WPos;
//in vec3 mygl_WNormal;

in VS_OUTPUT_WORLD_PNT
{
	vec2 tex;
	vec3 wpos;
	vec3 wnormal;
} vsOut;

layout(location = 0) out vec4 FsOutColor;

void main(void)
{
	// 頂点シェーダーではなくフラグメント シェーダー側で法線を正規化することで、ピクセル単位のライティングを実現できる。
	//FsOutColor = CalcLighting(mygl_WPos, normalize(mygl_WNormal), mygl_TexCoord0);
	FsOutColor = CalcLighting(vsOut.wpos, normalize(vsOut.wnormal), vsOut.tex);
}
