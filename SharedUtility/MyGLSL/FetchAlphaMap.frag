//#version 440
//#version 330
//#version 140
//#version 120

// HACK: テクスチャ サンプラーの uniform 変数には layout:binding によるインデックス固定が効かない？
uniform sampler2D UniMainTexture;

in vec4 mygl_Color;
in vec2 mygl_TexCoord0;

layout(location = 0) out vec4 FsOutColor;

void main(void)
{
	// 頂点シェーダーから受け取ったカラー RGBA と、アルファ マップをモジュレートする。
#if 0
	// OpenGL 2.x / ES 2.0 用。
	gl_FragColor = vec4(gl_Color.rgb, gl_Color.a * texture2D(UniMainTexture, vec2(gl_TexCoord[0])).a);
#else
	FsOutColor = vec4(mygl_Color.rgb, mygl_Color.a * texture(UniMainTexture, mygl_TexCoord0).a);
	//FsOutColor = vec4(min(FsOutColor.r, 0.0), min(FsOutColor.g, 0.0), min(FsOutColor.b, 0.0), max(FsOutColor.a, 1.0)); // ラスター化ルールのテストコード。
#endif
}
