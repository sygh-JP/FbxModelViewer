#ifndef MY_MSAA_ALG_TEX_SAMPLE_COUNT
//#define MY_MSAA_ALG_TEX_SAMPLE_COUNT 1
#define MY_MSAA_ALG_TEX_SAMPLE_COUNT 8
// マクロを使ってテンプレート的に複製する。GLSL コンパイラに渡す前に、マクロ定義行を文字列結合すればよい。
#endif
//#version 440
//#version 330
//#version 140
//#version 120


in vec4 mygl_Color;
in vec2 mygl_TexCoord0;

layout(location = 0) out vec4 FsOutColor;

// MSAA なしの場合でも、FP16 テクスチャから 8bit RGBA バックバッファに転送する場合はシェーダーを使う必要がある。
#if (MY_MSAA_ALG_TEX_SAMPLE_COUNT == 1)

uniform sampler2D UniMainTexture;

void main(void)
{
	// OpenGL の UV は左下原点。Render Texture 経由で画面座標への転送に使う場合は注意が必要。
	vec2 texCoord = vec2(mygl_TexCoord0.x, 1.0 - mygl_TexCoord0.y);
	FsOutColor = texture(UniMainTexture, texCoord) * mygl_Color;
}

#else

// マルチサンプル テクスチャは OpenGL 3.2, GLSL 1.5 以降でのみ使用可能。
// また、読み出しには texture2D() や texture() でなく texelFetch() を使う。
// OpenGL ES では 3.0 以降で標準化されているらしい。
// HLSL での Load() メソッドに相当する。
// ちなみに NVIDIA Cg には tex1Dfetch(), tex2Dfetch(), tex3Dfetch() があるが、
// これらはサンプラーを使わないわけではなく整数インデックスが指定できるバージョンらしい。
// また、NVIDIA CUDA には線形メモリ用の tex1Dfetch() がある。
uniform sampler2DMS UniMainTexture;

void main()
{
	const int samples = MY_MSAA_ALG_TEX_SAMPLE_COUNT;
	FsOutColor = vec4(0.0);
	vec2 texCoord = vec2(mygl_TexCoord0.x, 1.0 - mygl_TexCoord0.y);
	ivec2 texCoord2 = ivec2(textureSize(UniMainTexture) * texCoord);
	// HACK: ループアンロール属性を付けたいところ。
	for (int i = 0; i < samples; ++i)
	{
		FsOutColor += texelFetch(UniMainTexture, texCoord2, i);
	}
	FsOutColor *= mygl_Color;
	FsOutColor /= float(samples);
}

#endif
