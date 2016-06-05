//#version 440
//#version 330
//#version 140
//#version 120

layout(location = 0) in vec3 VsInPosition;
layout(location = 1) in vec4 VsInColor;
layout(location = 2) in vec2 VsInTexCoord;

// GLSL の組み込み変数に似せて、ユーザー定義の out には mygl_ プレフィックスを付けることにする。
out vec4 mygl_Color;
out vec2 mygl_TexCoord0;

void main(void)
{
	// NOTE: OpenGL 3.0, GLSL 1.3 以降は、gl_FrontColor などの組み込みの varying 変数の代わりにユーザー定義の out 変数に出力する。
	// gl_Position だけは例外らしい（HLSL でいう SV_Position セマンティクスでマークされた変数に相当する）。
	// ちなみに in/out は、OpenGL 2.x 世代の GPU ではサポートされないので注意。
	// なお、NVIDIA の OpenGL ドライバーでは、なぜか #version 120 を指定していても H/W とドライバーさえ OpenGL 3.x 以上に対応していれば、
	// in のほうは使える模様。NVIDIA の GLSL コンパイラは AMD や Intel に比べて全体的に緩いらしい。
	// また、out を使う際、in を定義せず組み込みの gl_Color などを使おうとするとコンパイル エラーになる。
	// 定数リテラルの暗黙変換（整数→浮動小数）は、NVIDIA ドライバーでは OK だが、
	// 他のベンダーではサポートされないものもあるらしい（特にモバイルの ESSL）。
	// つまり、vec4(0, 0, 0, 0) は NG。vec4(0.0, 0.0, 0.0, 0.0) と書く必要がある。
	// vec4(0.0) という省略構文もあるが、ドライバーによってはこの省略構文もアウトらしい。
	// ちなみに mat4(1.0) で単位行列の生成ができるらしいが、これすらアウトになるドライバーもあるのか？
	// どんだけヘボ実装なんだ……
	// GLSL 4.2 以降では vec4 x = { 0.0, 0.0, 0.0, 0.0 }; という C 言語の初期化子リスト構文が使えるらしい。
	// HLSL では最初からどちらの構文も使えていたので、GLSL も最初から対応しとけと言いたいが……
	// モバイルでは discard（Alpha Test）も失速の原因になるらしい。
	// http://dench.flatlib.jp/opengl/glsl
	// 
	// ちなみに uniform には struct を使えるが、in/out には struct を使えないらしい。OpenGL 3.2 の Interface Block で似たような対応ができるが、
	// HLSL からコードを移植する場合は注意。
	// 
	// なお、NVIDIA の OpenGL ドライバーはどうもいったん GLSL を Cg コードに変換してからコンパイルしているらしい？
	// 単に Internal Cg Compiler は GLSL もコンパイルできるということ？
	// GL_SHADING_LANGUAGE_VERSION を取得すると、
	// "4.30 NVIDIA via Cg compiler" などの文字列が返ってくる。
	// Cg は一般公開されている最終版の April 2012 Ver.3.1 時点で OpenGL 4.3 コンピュート シェーダーのプロファイルをサポートしていないが、
	// NVIDIA ドライバー内部で使われている Internal Cg の最新版ではコンピュート シェーダーをサポートしているということか？

	gl_Position = vec4(VsInPosition, 1.0);
#if 0
	gl_FrontColor = VsInColor;
	gl_TexCoord[0] = vec4(VsInTexCoord.x, VsInTexCoord.y, 0.0, 0.0);
#else
	mygl_Color = VsInColor;
	mygl_TexCoord0 = VsInTexCoord;
#endif
}
