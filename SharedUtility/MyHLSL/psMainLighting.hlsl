// Only UTF-8 or ASCII is available.

#include "VSInOutStruct.hlsli"
#include "CommonConst.hlsli"

#include "MyConstantBuffers.hlsli"
#include "MyTexSamplers.hlsli"

//#include "Skinning.hlsli"
#include "ShadowSampling.hlsli"

#include "DiffuseSpecularFunc.hlsli"

///////////////////////////////////////////////////////////////////////////////

float4 CalcLightingPhotoReal(float shadowingPercentLit, float diffuse, float3 specular, float2 tex)
{
	// フォト リアルではハーフ ランバートを使う。
	// ランバートにシャドウマップから得たライト率を乗じた後、改めてハーフ ランバートを計算する。
	// ライトの方向と、ライティング対象の面の方向が平行に近づくと、
	// カスケード シャドウを使ったとしてもどうしてもエイリアスが発生するので、
	// シャドウイング＋シェーディングで解決する。
	// なお、ランバート シェーディングの結果にライト率を乗じるのはよいが、ハーフ ランバートの結果にライト率を乗じてはいけない。

	const float diffuse1 = 0.5 * (shadowingPercentLit * diffuse) + 0.5;
	const float diffuse2 = diffuse1 * diffuse1;

	// UNDONE: エミッシブ（自己発光）は、ポスト エフェクトである高輝度抽出ブルーム エフェクト バッファへの出力の有効／無効を切り替えられるようにする。
	// 出力する場合は飽和させないで高輝度領域を HDR レンダリングしてしまったほうがよい。
	const float4 texDiffuseColor = SrvDiffuseTex.Sample(SS_LinearWrap, tex);
	float4 color = float4(
		(MaterialColorDiffuse.w * MaterialColorDiffuse * LightColor * diffuse2).rgb,
		MaterialOpacityAlpha);

	// Metasequoia はマテリアルの色（Vector3F）とディフューズ強度（Float）は別物。
	// また、ディフューズ カラーやアンビエント カラー、スペキュラーカラーといった概念はない。
	// FBX Exporter でコンバートする際、DirectX RM の X フォーマットや 3ds Max 同様に、
	// DiffuseColor3F = DiffuseIntensityF * MaterialColor3F と TransparencyF = 1 - MaterialOpacityF が出力される模様。
	// つまり、DiffuseIntensityF が分からないかぎり、MaterialColor3F は復元できない。
	// なお、AmbientColor3F や EmissiveColor3F はそれぞれ AmbientIntensityF と EmissiveIntensityF を3つ並べたベクトルになる。
	// MQO を FBX にエクスポートする場合、マテリアル カラーは完全にホワイトにして、
	// テクスチャ色だけで制御するようにしたほうがよさげ。
	// --> FBX Exporter Ver.1.2.10 で ColorRGB と Factor が分離されて、MQO の情報が完全に再現できるようになったので、マテリアル カラーを使える。
	const float4 ambems = texDiffuseColor * (MaterialColorAmbient.w * AmbientLight + MaterialColorEmissive.w);
	// スペキュラー色（ハイライト色）はディフューズ テクスチャ カラーに左右されない。
	// HACK: 金や銅などの金属では、ハイライトの色は材質の色に左右される。
	// これがディフューズ カラーの他にスペキュラーカラーをマテリアル属性として別途用意しておいたほうがいい理由。
	// 金属は専用のシェーダープログラムを用意して、マテリアル特性（Conductor/Dielectric/etc.）に応じてシェーダー自体を変えるという手もある。
	// セル シェーダー系でも金属パーツだけはリアル調にする、というのも良いかも。
	const float4 spc = float4(MaterialColorSpecular.w * LightColor.rgb * specular, 0);
	// アンビエントやエミッシブのアルファは無視。
#if 0
	color.rgb += (ambems + spc).rgb;
	return saturate(color) * texDiffuseColor;
#else
	// Maya, LightWave 寄りのアルゴリズム。スペキュラーは透明度の影響を受けない。
	color.rgb += ambems.rgb;
	//return spc + saturate(color) * texDiffuseColor;
	float4 outColor = spc + saturate(color) * texDiffuseColor;
		// アルファ チャンネルは必ず0～1にクランプする必要がある。
		// ダイナミック レンジでレンダリングする場合、アルファが1を超えると意図した通りにアルファ合成できない。
		outColor.a = saturate(outColor.a);
	return outColor;
#endif

	// アンビエントは直接ライトをすべて切ったとしても残る係数。
	// エミッシブは外部ライトをすべて切ったとしても残る係数。
	// ただし Metasequoia では、それらはスケーリング／下駄用のスカラー値にすぎず、
	// 出力色はマテリアル カラーとディフューズ テクスチャ カラーを乗じるようになっている。
	// セル シェーダー系やイラスト調の明るい肌表現として、強いアンビエントやエミッシブにより下駄を履かせるのは GI 的に邪道。
	// なお、アンビエントやエミッシブを単純に加算するとすぐに飽和して白飛びするので、それを防止するために saturate() を使用する。
	// HDR レンダリングした結果をポスト エフェクトでトーン マッピングする場合は余計な処理なので、クランプ処理は必要に応じて実施する必要がある。

	// ピクセル単位のライティングを行なうため、ディフューズやスペキュラーに関してはピクセル シェーダー側で計算する必要があるが、
	// もしシェーダーモデル 2.0 のような命令数の制約の厳しい条件では、
	// 頂点シェーダーで計算できるもの（アンビエントやエミッシブの係数合成など）は頂点シェーダー側で計算しておいたほうがよい。
}

float4 CalcLightingToon(float shadowingPercentLit, float diffuse, float3 specular, float2 tex)
{
	// トゥーン シェーディングではハーフ ランバートを使わない。
	// トゥーン シェーディング グラデーション参照テクスチャのフィルタはクランプ モード。
	const float4 vDiffuse = SrvToonShadingGradRefTex.Sample(
		SS_LinearClamp,
		//SS_LinearWrap,
		float2(ToonShadingRefTexIniX + shadowingPercentLit * diffuse, ToonShadingRefTexV));
	// HACK: これだと影の輪郭に VSM ブラーがかからなくなる。
	// トゥーン グラデーションが影色を制御するだけの線形階調であればブラーが見えるが、
	// 非線形階調であれば場合によってはブラーが見えない。

	// セル シェーダーは従来のフォト リアリスティック シェーダーとはマテリアルの使い方を変える。
	// ディフューズ強度・アンビエント・エミッシブを入れない。MQO 用リアルタイム セルシェーダー、CelsView の実装もそうなっている模様。
	// ただし CelsView は MQO からマテリアル カラーを取得して、カラーテクスチャと乗算しているが、
	// FBX を使う場合は DiffuseFactor 乗算前の Diffuse を使うなりしないといけない。
	// ファイル フォーマットによらず確実に色再現を求めるならば、色はすべてカラーテクスチャと直接ライトで制御する。
	// アンビエントを入れるかどうかはシーンやオブジェクトに応じて動的に制御できたほうがよい。もちろん下駄を履かせるだけでなく、
	// きっちりした GI 的アプローチ（フォトン マッピングやレイトレース）をとったほうがよいことに変わりはない。
	const float4 texDiffuseColor = SrvDiffuseTex.Sample(SS_LinearWrap, tex);
	float4 color = float4(
		//(MaterialColorDiffuse.w * MaterialColorDiffuse * LightColor * vDiffuse).rgb,
		(MaterialColorDiffuse * LightColor * vDiffuse).rgb,
		MaterialOpacityAlpha);

	const float4 spc = float4(MaterialColorSpecular.w * LightColor.rgb * specular, 0);

	float4 outColor = spc + saturate(color) * texDiffuseColor;
		outColor.a = saturate(outColor.a);
	return outColor;
}

///////////////////////////////////////////////////////////////////////////////
interface IHogeBase
{
	float4 GetColor();
};
// 実体（インスタンス）ではなくポインタのようなもの。実体は定数バッファ上のテーブルに確保される。
IHogeBase AbstractHoge;
class CHogeA : IHogeBase
{
	float4 m_vDummy;
	float4 GetColor()
	{
		return m_vDummy + float4(1, 0, 0, 1);
	}
};
class CHogeB : IHogeBase
{
	float4 m_vDummy;
	float4 GetColor()
	{
		return m_vDummy + float4(0, 1, 0, 1);
	}
};
class CHogeC : IHogeBase
{
	float4 m_vDummy;
	float4 GetColor()
	{
		return m_vDummy + float4(0, 0, 1, 1);
	}
};

///////////////////////////////////////////////////////////////////////////////
interface IMainLightingShaderBase
{
	float4 CalcLighting(float shadowingPercentLit, float diffuse, float3 specular, float2 tex);
};
// 実体（インスタンス）ではなくポインタのようなもの。実体は定数バッファ上のテーブルに確保される。
IMainLightingShaderBase AbstractMainLightingShader;
class CMainLightingShaderPhotoReal : IMainLightingShaderBase
{
	float4 m_vDummy;
	float4 CalcLighting(float shadowingPercentLit, float diffuse, float3 specular, float2 tex)
	{
		return CalcLightingPhotoReal(shadowingPercentLit, diffuse, specular, tex);
	}
};
class CMainLightingShaderToon : IMainLightingShaderBase
{
	float4 m_vDummy;
	float4 CalcLighting(float shadowingPercentLit, float diffuse, float3 specular, float2 tex)
	{
		return CalcLightingToon(shadowingPercentLit, diffuse, specular, tex);
	}
};

///////////////////////////////////////////////////////////////////////////////
interface ISpecularBase
{
	float3 CalcSpecular(float3 wnormal, float3 light, float3 eye, float3 halfway);
};
// 実体（インスタンス）ではなくポインタのようなもの。実体は定数バッファ上のテーブルに確保される。
ISpecularBase AbstractMainLightingSpecular;
class CMainLightingSpecularDummy : ISpecularBase
{
	float4 m_vDummy;
	float3 CalcSpecular(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		return float3(0, 0, 0);
	}
};
class CMainLightingSpecularBlinnPhong : ISpecularBase
{
	float4 m_vDummy;
	float3 CalcSpecular(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		return CalcBlinnPhongSpecular(wnormal, halfway, MaterialSpecularPower);
	}
};
class CMainLightingSpecularCookTorrance : ISpecularBase
{
	float4 m_vDummy;
	float3 CalcSpecular(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		return CalcCookTorranceSpecular(wnormal, light, eye, halfway, MaterialColorSpecular.rgb, UniMaterialRoughness);
	}
};

///////////////////////////////////////////////////////////////////////////////
interface IEnvMapColorPickerBase
{
	float4 GetEnvMapColor(float3 wnormal, float3 light, float3 eye, float3 halfway);
};
// 実体（インスタンス）ではなくポインタのようなもの。実体は定数バッファ上のテーブルに確保される。
IEnvMapColorPickerBase AbstractEnvMapColorPicker;
class CEnvMapColorPickerDummy : IEnvMapColorPickerBase
{
	float4 m_vDummy;
	float4 GetEnvMapColor(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		return float4(0, 0, 0, 0);
	}
};
class CEnvMapColorPickerReflect : IEnvMapColorPickerBase
{
	float4 m_vDummy;
	float4 GetEnvMapColor(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		return SrvEnvMapTex.Sample(SS_LinearClamp, reflect(eye, wnormal));
	}
};
class CEnvMapColorPickerRefract : IEnvMapColorPickerBase
{
	float4 m_vDummy;
	float4 GetEnvMapColor(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		const float eta = 0.67; // 屈折率の比。
		return SrvEnvMapTex.Sample(SS_LinearClamp, refract(eye, wnormal, eta));
	}
};
class CEnvMapColorPickerFresnel : IEnvMapColorPickerBase
{
	float4 m_vDummy;
	float4 GetEnvMapColor(float3 wnormal, float3 light, float3 eye, float3 halfway)
	{
		const float eta = 0.67; // 屈折率の比。
		const float fresnel0 = (1.0 - eta) * (1.0 - eta) / ((1.0 + eta) * (1.0 + eta)); // フレネル反射係数 F(0°) の計算。
		// TODO: フレネル反射。
		// フレネル反射の計算は全部ピクセル シェーダーで行なわずに、反射・屈折ベクトルの計算までは頂点シェーダーにオフロードしたほうが良いか？
		// 頂点単位の法線だと金属表面の映り込みとかのクオリティが下がりそう。ディフューズやスペキュラーほどの劣化は起こらないと思うが……
		// Schlick 近似に関しては諸式あるようなのだが、実際のところどれが正しいのかよく分からない。
		// ビューベクトルとハーフ ベクトルを使っているものもある。
		// ライト ベクトルとハーフ ベクトルを使うのが本物？
		// http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20081220
		// http://d.hatena.ne.jp/hanecci/20130525/p3
		// http://kanamori.cs.tsukuba.ac.jp/jikken/inner/reflection_refraction.pdf
		// http://content.gpwiki.org/D3DBook:%28Lighting%29_Cook-Torrance
		// https://spphire9.wordpress.com/2013/03/13/webgl%E3%81%A7cook-torrance/
		const float border = CalcSchlickFresnelTerm(fresnel0, light, halfway);
		const float4 reflectVal = SrvEnvMapTex.Sample(SS_LinearClamp, reflect(eye, wnormal));
		const float4 refractVal = SrvEnvMapTex.Sample(SS_LinearClamp, refract(eye, wnormal, eta));
		return lerp(refractVal, reflectVal, border);
	}
};

///////////////////////////////////////////////////////////////////////////////
// たとえ必要ない場合でも、ID3D11ClassLinkage::GetClassInstance() で取得できるようにするために
// インターフェイス実装クラスにはダミーの float4 メンバー変数を1つ持たせる。
// これは実際に使われなくても（GLSL のような過剰な最適化で）消滅することはないらしい。
// なお、複数のインターフェイスのインスタンスを切り替える場合でも、1つの定数バッファでいける模様。
// Dynamic Shader Linkage を使って分岐する場合、
// [1]
//   1つのシェーダーあたりインターフェイスは1つまでにして、
//   組み合わせは継承のパターン分岐で実現する方法。
// [2]
//   複数のインターフェイスを用意して、組み合わせをホスト側で動的に制御する方法。
// の2つがあるが、[1]は結局 Dynamic Shader Linkage を使う意味があまりなくなるので、できれば[2]のほうが望ましい。
// DirectX SDK June 2010 には DynamicShaderLinkageFX11 という Effects11 を使ったサンプルがあり、
// これは複数のインターフェイスを同時に使って動的な組み合わせを実現している。
// D3DCompiler ランタイムのシェーダーリフレクションが使える場合は非常に分かりやすくなる（OpenGL Shader Subroutine と同じくらい分かりやすい）が、
// そうでない場合はかなりトリッキーな工夫が必要になる。
cbuffer CBufferClassInstanceSelectorTable : register(b4)
{
	CMainLightingSpecularDummy UniInstanceSpecularDummy;
	CMainLightingSpecularBlinnPhong UniInstanceSpecularBlinnPhong;
	CMainLightingSpecularCookTorrance UniInstanceSpecularCookTorrance;

	CMainLightingShaderPhotoReal UniInstanceMainLightingShaderPhotoReal;
	CMainLightingShaderToon UniInstanceMainLightingShaderToon;

	CEnvMapColorPickerDummy UniInstanceEnvMapColorPickerDummy;
	CEnvMapColorPickerReflect UniInstanceEnvMapColorPickerReflect;
	CEnvMapColorPickerRefract UniInstanceEnvMapColorPickerRefract;
	CEnvMapColorPickerFresnel UniInstanceEnvMapColorPickerFresnel;

	CHogeA UniInstanceHogeA;
	CHogeB UniInstanceHogeB;
	CHogeC UniInstanceHogeC;
};
// http://msdn.microsoft.com/ja-jp/library/ee419541.aspx
// ランタイムで実行する ID3D11ClassLinkage::GetClassInstance で取得できるためには、
// 1 つの以上のデータ メンバーを持つクラス インスタンスであることが必要です。
// メンバーのないインスタンスは、サイズが 0 のオブジェクトとして、コンパイルされたシェーダー BLOB から最適化されます。
// クラスにデータ メンバーがない場合は、代わりに ID3D11ClassLinkage::CreateClassInstance を使用してください。
///////////////////////////////////////////////////////////////////////////////

// ファーマップを同時に作成するピクセル シェーダー（フォン シェーディング）。
void main(in VS_OUTPUT_WORLD_PNT psIn, out float4 outColor0 : SV_Target0, out float4 outColor1 : SV_Target1, out float4 outColor2 : SV_Target2)
{
	float shadowingPercentLit = 1;
	int shadowingCascadeIndex = 0;
	psIn.LightViewPos /= psIn.LightViewPos.w;
	// 深度値はピクセル シェーダー側で w=1 に射影する必要はないのか？
	// 頂点シェーダー側で射影したものを補間する方法でもよいのか？
	// ちなみに DirectX SDK のサンプルでは w=1 に射影すらしていなかった。
	//const float wvdepth = psIn.WVPos.z / psIn.WVPos.w;
	const float wvdepth = psIn.WVDepth;
	CalcShadowLightPercent(wvdepth, psIn.LightViewPos, shadowingCascadeIndex, shadowingPercentLit);

	// Per-Pixel Lighting のため、法線ベクトルはピクセル シェーダー側で改めて正規化する（ピクセル シェーダーに渡ってくる法線ベクトルは線形補間されている）。
	const float3 wnormal = normalize(psIn.wnormal);
	const float3 light = normalize(LightPos - psIn.wpos); // ライト ベクトル（LightToPos）。
	//const float3 light = -LightDir;
	const float3 eye = normalize(UniEyePosition - psIn.wpos); // 視線ベクトル（EyeToPos, ViewToPos）。
	const float3 halfway = normalize(light + eye); // ハーフ ベクトル。
	const float diffuse = CalcLambertDiffuse(wnormal, light);
#if 0
	const float specular = CalcBlinnPhongSpecular(wnormal, halfway, MaterialSpecularPower);
#else
	const float3 specular = AbstractMainLightingSpecular.CalcSpecular(wnormal, light, eye, halfway);
#endif

#if 1
	outColor0 = AbstractMainLightingShader.CalcLighting(shadowingPercentLit, diffuse, specular, psIn.tex);
#else
	// カスケード シャドウのデバッグ用。
	outColor0 = CascadeColorMultipliers[shadowingCascadeIndex] * AbstractMainLightingShader.CalcLighting(1, diffuse, specular, psIn.tex);
#endif

#if 1
	// Direct3D ではテクスチャが SRV スロットにバインドされていない場合、フェッチするとゼロベクトルが返る仕様らしい。
	// OpenGL とは違ってデバッグ レイヤーは何も言ってこないので、ハードウェア依存の未定義動作ではなく仕様のはず。要確認。
	// 仕様かどうかはっきりしない場合はダミーのキューブマップ テクスチャを明示的に作成して、環境マッピングしない場合はダミーをバインドしておく。
	// HACK: ただブレンドするのではなく、スペキュラーハイライトを独立処理する。
	// HACK: アルファは環境マップとブレンドするべきでない。カラー RGB のみとするべき。
	// HACK: 反射マッピングや屈折マッピングが必要な場合とそうでない場合とで、ピクセル シェーダーを分けるか、Dynamic Shader Linkage を使う。
	float4 envMapColor = AbstractEnvMapColorPicker.GetEnvMapColor(wnormal, light, eye, halfway);
		//float4 envMapColor = AbstractEnvMapColorPicker.GetEnvMapColor(wnormal, light, eye, halfway) * AbstractHoge.GetColor();
		outColor0 = lerp(outColor0, envMapColor, MaterialReflectivity);
#endif

	// もうひとつのレンダーターゲットであるファーマップには、デバイス座標系におけるピクセル単位の法線ベクトルと、ファー始点の深度値を書き込む。
	// ファーを適用しないマテリアルの場合、ファーマップに書き込みを行なわないピクセル シェーダーを使う。
	// 読み込み側で必要に応じてファー長さを調整する。
	// MRT を使うと、同じ MSAA フォーマットにしないといけないという制約があるので、
	// 別パスでファーが必要なマテリアル メッシュのみを使ってファーマップを作成したほうがよい？
	const float3 devNorm = normalize(psIn.normal);
	outColor1 = float4(devNorm, psIn.pos.z);

	// ポスト プロセスでのエッジ検出用に、ワールド座標系の法線・深度マップに出力。
	// 法線は -1～+1 を 0～1 に直す（smoothstep(-1, +1, x) で直してもいいかも）。
	// これにより、一応 MSAA FP16 から非 MSAA の R8G8B8A8 バッファにも転送できる形式になるが、
	// 精度が落ちるので転送先も FP16 にしておいたほうがよいかも。
	outColor2 = float4((wnormal + 1.0f) * 0.5f, psIn.pos.z / psIn.pos.w);
	//outColor2 = float4((devNorm + 1.0f) * 0.5f, psIn.pos.z / psIn.pos.w);
}
