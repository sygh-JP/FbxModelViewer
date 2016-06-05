#version 440
//#extension GL_ARB_compute_variable_group_size : enable

// HLSL から移植しやすいように、型のシノニムを定義したいが、GLSL には typedef がない。仕方がないのでマクロを使う。

#define float4 vec4
#define float3 vec3
#define float2 vec2

// 組み込み関数に関しても、HLSL 同等シンボルを定義しておく。
#define saturate(x)  clamp((x), 0.0, 1.0)
#define lerp(x, y, s)  mix((x), (y), (s))
#define rsqrt(x)  inversesqrt(x)
