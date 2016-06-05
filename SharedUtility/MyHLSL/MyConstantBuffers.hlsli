// Only UTF-8 or ASCII is available.

// ボーン最大数。ホスト プログラム側と同じ値にする。
//#define MAX_ANIM_BONE_NUM  (56)
//static const uint MAX_ANIM_BONE_NUM = 56;
static const uint MAX_ANIM_BONE_NUM = 120;

static const uint MAX_SHADOW_CASCADES_NUM = 8;


//static const uint TOON_SHADING_REF_TEX_SIZE = 128;
static const uint TOON_SHADING_REF_TEX_SIZE = 256;
// とりあえず最大 8bit 階調のグラデーションを表現できるようにしておく。
// 正方形テクスチャを使う場合、1枚あたりで表現可能なトゥーン シェーディング対象マテリアル数上限も 256 となる。
// ゲームの場合、1シーンで 256 マテリアルあれば十分のはず。足らなければテクスチャを複数枚用意するか、非正方形テクスチャを使う。
// また、マテリアルの名前と書込ライン番号（0～255）をマップにしておく必要がある。

// トゥーン シェーディング（Toon shading）、セル シェーディング（Cel shading）という用語があるが、
// このソースコードでは主に Toon のほうを使うことにする。

// テクスチャ1ライン中の階調をフェッチするが、
// バイリニア フィルタリングすると UV 座標が半テクセルずれる。そのための補正係数を定義。

//static const float ToonShadingRefTexIniX = 0.0f / TOON_SHADING_REF_TEX_SIZE;
//static const float ToonShadingRefTexIniY = 0.0f / TOON_SHADING_REF_TEX_SIZE;
static const float ToonShadingRefTexIniX = 0.5f / TOON_SHADING_REF_TEX_SIZE;
//static const float ToonShadingRefTexIniY = 0.5f / TOON_SHADING_REF_TEX_SIZE;

//static const float ToonShadingRefTexV = 0.5f / TOON_SHADING_REF_TEX_SIZE; // Material#0.
//static const float ToonShadingRefTexV = 1.0f / TOON_SHADING_REF_TEX_SIZE; // NG.
//static const float ToonShadingRefTexV = (1.0f + 0.5f) / TOON_SHADING_REF_TEX_SIZE; // Material#1.
// TODO: マテリアルごとに値が異なるので、変数にする。

// マテリアルごとに参照する色調（テクスチャのライン）を変える。
// 肌は赤もしくは紫っぽい影、白い布は青っぽい影のほうがアニメ的な生き生きした色になる。
// 参照する色をテクスチャ側（デザイナー側）で決定してコンテンツ化するか、
// それとも適当な色補正アルゴリズムを考えてシェーダーで実装するか、要検討。

// HLSL シェーダー側では自動的に 16 バイト（float4）境界にアライメントされる。
// packoffset を使うと明示的に配置を制御できる。
// 定数バッファ cbuffer はテクスチャ バッファ tbuffer と比較して頻繁な更新に強いが、
// 効率向上のためには更新頻度別に cbuffer を分けて定義するとよい。
// Direct3D 10/11 の cbuffer は 4096 ベクトルまで許容されるので、float4x4 だと 1024 個まで格納できる。

// GLSL の Uniform Block とは違い、cbuffer のインスタンス名を指定することはできないらしい。


cbuffer CBufferBoneMatrixPalettePack : register(b0)
{
	// ボーン行列。
	row_major float4x4 UniBoneMatrixPalette[MAX_ANIM_BONE_NUM] : packoffset(c0);
};

#if 0
struct QuatTransform
{
	float4 RotationQ;
	float4 TranslationV;
};

cbuffer CBufferBoneQuatPalettePack : register(b0)
{
	// ボーン変換情報。
	QuatTransform UniBoneQuatPalette[MAX_ANIM_BONE_NUM] : packoffset(c0);
};
#endif

cbuffer CBufferViewParamsPack : register(b1)
{
	// トランスフォーム行列。
	float4x4 UniWorld : packoffset(c0);
	float4x4 UniView : packoffset(c4);
	float4x4 UniProjection : packoffset(c8);
	float4x4 UniWorldView : packoffset(c12);
	float4x4 UniViewProj : packoffset(c16);
	// ワールド・ビュー・射影変換行列。
	float4x4 UniWorldViewProj : packoffset(c20);

	float3 UniEyePosition : packoffset(c24);

	int2 UniScreenSize : packoffset(c25);
};

// RGB は単純なシェーディング アルゴリズムの場合ディフューズにあれば十分だが、一応すべてに用意しておく。
// 定数バッファに余裕がない Direct3D 9 レベル環境ではオミットしたほうがよい。
// RGB も Level も [0, 1] に正規化された値。
cbuffer CBufferMeshPartAttributePack : register(b2)
{
	// メッシュのカレント マテリアルのディフューズ RGB と Level。
	float4 MaterialColorDiffuse : packoffset(c0);
	// メッシュのカレント マテリアルのアンビエント RGB と Level。
	float4 MaterialColorAmbient : packoffset(c1);
	// メッシュのカレント マテリアルのスペキュラー RGB と Level。
	float4 MaterialColorSpecular : packoffset(c2);
	// メッシュのカレント マテリアルのエミッシブ RGB と Level。
	float4 MaterialColorEmissive : packoffset(c3);

	// 不透明度。0＝透明、1＝不透明。
	float MaterialOpacityAlpha : packoffset(c4.x);
	// スペキュラー強度。
	float MaterialSpecularPower : packoffset(c4.y);
	// 反射率。
	float MaterialReflectivity : packoffset(c4.z);
	// 屈折率。
	float MaterialIndexOfRefraction : packoffset(c4.w);

	float ToonShadingRefTexV : packoffset(c5.x);

	float UniMaterialRoughness : packoffset(c5.y);
};

// ライティング情報。
cbuffer CBufferLightParamsPack : register(b3)
{
	// ライトの向き。
	float3 LightDir : packoffset(c0);
	// ライトのワールド位置。
	float3 LightPos : packoffset(c1);
	// ライトの色。
	float4 LightColor : packoffset(c2);
	// 環境光の色。
	float4 AmbientLight : packoffset(c3);
};

// b4 レジスターは動的シェーダーリンクのテーブルに使っている。
// HACK: シャドウマップのレンダリングにジオメトリ シェーダーを使って1キックにするならば、
// サンプリングに使用する定数バッファ b6 と統合してしまってもよい。

cbuffer CBufferShadowRenderingInfoPack : register(b5)
{
	matrix UniCascadedLightViewProj[MAX_SHADOW_CASCADES_NUM] : packoffset(c0);
	int UniAvailableCascadeCount : packoffset(c32.x);
};


#if 0
// ガウスぼかしのカーネル最大サイズ。ホスト プログラム側と同じ値にする。
//#define MAX_GAUSSIAN_WEIGHTS_KERNEL_HALF  (7)
static const int MAX_GAUSSIAN_WEIGHTS_KERNEL_HALF = 7;

// ガウスぼかしの重み配列。
float GaussianWeightsArray[MAX_GAUSSIAN_WEIGHTS_KERNEL_HALF + 1];
// なお、HLSL の float, float2, float3 の配列は、内部的にはすべて float4 の配列になってしまい、無駄が発生するので、
// 特に定数バッファが貧弱な Direct3D 9 世代の環境では可能な限りパッキングを考慮したほうがよい。
// ガウスぼかしのぼかし量を複数段使い分ける場合、
// 最大サイズを持つひとつの配列を毎回書き換えて使いまわすのではなく、
// レベルごとに複数の固定長配列を用意しておいたほうが良さげ。
// アップデートは起動時に1回行なうだけでよくなる。
// ……となると、いちいち実行時に初期化するのではなく、
// いっそシェーダーのコンパイル時に定数として与えてしまったほうが良さげ。
#endif

#if 0
// テクスチャ座標オフセット ベクトルの生成を静的に行なって最適化する。
// ただしダウンサンプル テクスチャのサイズが変動し、さらに実行時コンパイルが使えない場合、CPU 側で設定する必要がある。
// --> ループ アンロールによる最適化に期待して、手動最適化は行なわないことにする。
static const float2 GaussianOffsetsArray[MAX_GAUSSIAN_WEIGHTS_KERNEL_HALF] =
{
	float2( 2.0f / DOWN_SAMPLED_TEX_SIZE, 0),
	float2( 4.0f / DOWN_SAMPLED_TEX_SIZE, 0),
	float2( 6.0f / DOWN_SAMPLED_TEX_SIZE, 0),
	float2( 8.0f / DOWN_SAMPLED_TEX_SIZE, 0),
	float2(10.0f / DOWN_SAMPLED_TEX_SIZE, 0),
	float2(12.0f / DOWN_SAMPLED_TEX_SIZE, 0),
	float2(14.0f / DOWN_SAMPLED_TEX_SIZE, 0),
};
#endif

#if 0
// カーネル サイズを複数段用意する場合は不要。
static const int GaussianIndexedOffsetsArray[MAX_GAUSSIAN_WEIGHTS_KERNEL_HALF] =
{
	2,
	4,
	6,
	8,
	10,
	12,
	14,
};
#endif
