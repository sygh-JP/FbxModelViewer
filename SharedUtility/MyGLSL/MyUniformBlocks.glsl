#extension GL_ARB_enhanced_layouts : enable

// GLSL 4.4 ではバイト単位でレイアウト指定できるが、
// GPU のレジスタは float4 単位なので、
// HLSL と互換性を持たせる場合はこの3つのオフセット用基本単位があれば十分。
const int FP32SIZE = 4;
const int VEC4SIZE = 16;
const int MAT4SIZE = 64;

// GLSL では配列サイズに const int を使うことはできない。
#define MAX_ANIM_BONE_NUM  (120)

// Uniform blocks では std430 は使えない仕様らしい。
// GeForce GTX 760 + 337.88 ドライバーではコンパイル エラーになる。
// しかし、Quadro 600/2000 + 332.21 ドライバーでは成功する。おそらくドライバーのバグだった。
// Quadro 600/2000 + 340.52 ではちゃんとコンパイル エラーになる。
// Shader storage blocks では std430 も std140 も使える？
// https://www.opengl.org/registry/specs/ARB/shader_storage_buffer_object.txt

layout(std140, binding = 0) uniform CBufferBoneMatrixPalettePack
{
	layout(offset = 0) mat4 UniBoneMatrixPalette[MAX_ANIM_BONE_NUM];
};

layout(std140, binding = 1) uniform CBufferViewParamsPack
{
	layout(offset = MAT4SIZE * 0 + VEC4SIZE * 0 + FP32SIZE * 0) mat4 UniWorld;
	layout(offset = MAT4SIZE * 1 + VEC4SIZE * 0 + FP32SIZE * 0) mat4 UniView;
	layout(offset = MAT4SIZE * 2 + VEC4SIZE * 0 + FP32SIZE * 0) mat4 UniProjection;
	layout(offset = MAT4SIZE * 3 + VEC4SIZE * 0 + FP32SIZE * 0) mat4 UniWorldView;
	layout(offset = MAT4SIZE * 4 + VEC4SIZE * 0 + FP32SIZE * 0) mat4 UniViewProj;
	layout(offset = MAT4SIZE * 5 + VEC4SIZE * 0 + FP32SIZE * 0) mat4 UniWorldViewProj;

	layout(offset = MAT4SIZE * 6 + VEC4SIZE * 0 + FP32SIZE * 0) vec3 UniEyePosition;

	layout(offset = MAT4SIZE * 6 + VEC4SIZE * 1 + FP32SIZE * 0) ivec2 UniScreenSize;
};

layout(std140, binding = 2) uniform CBufferMeshPartAttributePack
{
	layout(offset = VEC4SIZE * 0 + FP32SIZE * 0) vec4 UniMaterialColorDiffuse;
	layout(offset = VEC4SIZE * 1 + FP32SIZE * 0) vec4 UniMaterialColorAmbient;
	layout(offset = VEC4SIZE * 2 + FP32SIZE * 0) vec4 UniMaterialColorSpecular;
	layout(offset = VEC4SIZE * 3 + FP32SIZE * 0) vec4 UniMaterialColorEmissive;

	layout(offset = VEC4SIZE * 4 + FP32SIZE * 0) float UniMaterialOpacityAlpha;
	layout(offset = VEC4SIZE * 4 + FP32SIZE * 1) float UniMaterialSpecularPower;
	layout(offset = VEC4SIZE * 4 + FP32SIZE * 2) float UniMaterialReflectivity;
	layout(offset = VEC4SIZE * 4 + FP32SIZE * 3) float UniMaterialIndexOfRefraction;

	layout(offset = VEC4SIZE * 4 + FP32SIZE * 4) float UniToonShadingRefTexV;
	layout(offset = VEC4SIZE * 4 + FP32SIZE * 5) float UniMaterialRoughness;

	//layout(offset = VEC4SIZE * 4 + FP32SIZE * 4) int hoge;
	// GLSL 4.4 の GL_ARB_enhanced_layouts でもメンバーオフセットのオーバーラップは許可されないらしい。
	// コンパイル エラーになる。
	// 仕様書の「4.4.5 Uniform and Shader Storage Block Layout Qualifiers」にもその旨記載があるが、若干分かりにくい。
	// HLSL の More Aggressive Packing を実現するにはどうすればいい？
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632.aspx
	// http://msdn.microsoft.com/ja-jp/library/ee418340.aspx
};

layout(std140, binding = 3) uniform CBufferLightParamsPack
{
	layout(offset = VEC4SIZE * 0) vec3 UniLightDir;
	layout(offset = VEC4SIZE * 1) vec3 UniLightPos;
	layout(offset = VEC4SIZE * 2) vec4 UniLightColor;
	layout(offset = VEC4SIZE * 3) vec4 UniAmbientLight;
};


const mat4 IdentityMatrixF = mat4(
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0);

const vec4 ZeroVector4F = vec4(0.0, 0.0, 0.0, 0.0);
