// Only UTF-8 or ASCII is available.

#include "GaussianWeights.hlsli"
#include "CommonConst.hlsli"
#include "CommonDefs.hlsli"
#include "VSInOutStruct.hlsli"

#include "MyTexSamplers.hlsli"


// コンピュート シェーダーによるポスト エフェクトは groupshared を伴うため、
// 他のシェーダーと一緒にまとめてコンパイルすると警告 X3579 が出る。
// エフェクト自体を分けてしまったほうがよさげ。

#define MY_GAUSSIAN_BLUR_CS_TEMP_LINE_MAX_SIZE  DOWN_SAMPLED_TEX_SIZE

// カーネルサイズは 9 * 2 + 1 = 19 になる。
#define MY_GAUSSIAN_BLUR_CS_FUNC_NAME           GaussianBlurFuncCS10
#define MY_GAUSSIAN_WEIGHTS_ARRAY_NAME          GaussianWeights10
#define MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT    10
#include "GaussianBlurFuncCS.hlsli"

#if 0
// カーネルサイズは 7 * 2 + 1 = 15 になる。
#define MY_GAUSSIAN_BLUR_CS_FUNC_NAME           GaussianBlurFuncCS8
#define MY_GAUSSIAN_WEIGHTS_ARRAY_NAME          GaussianWeights8
#define MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT    8
#include "GaussianBlurFuncCS.hlsli"
#endif

#if 0
// カーネルサイズは 5 * 2 + 1 = 11 になる。
#define MY_GAUSSIAN_BLUR_CS_FUNC_NAME           GaussianBlurFuncCS6
#define MY_GAUSSIAN_WEIGHTS_ARRAY_NAME          GaussianWeights6
#define MY_GAUSSIAN_WEIGHTS_ARRAY_ELEM_COUNT    6
#include "GaussianBlurFuncCS.hlsli"
#endif

#if 0
[numthreads(1, 1, 1)]
void csEmptyTest(
	uint3 globalGroupId : SV_GroupID,
	uint3 localGroupThreadId : SV_GroupThreadID,
	uint3 globalDispatchThreadId : SV_DispatchThreadID,
	uint localGroupIndex : SV_GroupIndex)
{
}
#endif

// Invoke by Dispatch(HEIGHT, 1, 1)
//[numthreads(WIDTH, 1, 1)]
[numthreads(DOWN_SAMPLED_TEX_SIZE, 1, 1)]
void csApplyGaussianBlur19ToDownSampledTex(uint3 globalGroupId : SV_GroupID, uint3 localGroupThreadId : SV_GroupThreadID)
{
	//GaussianBlurFuncCS8(localGroupThreadId.x, globalGroupId.x, DOWN_SAMPLED_TEX_SIZE, DownSampledTex, UniRWGaussianBlurOutTex);
	GaussianBlurFuncCS10(localGroupThreadId.x, globalGroupId.x, DOWN_SAMPLED_TEX_SIZE, DownSampledTex, UniRWGaussianBlurOutTex);
}


/////////////////////////////////////////////////////////////////////

#define MY_MOVING_AVERAGE_CS_TEMP_LINE_MAX_SIZE   SHADOW_MAP_TEXTURE_SIZE
#define MY_MOVING_AVERAGE_FILTER_CS_FUNC_NAME     MovingAverageFilterCS3
#define MY_MOVING_AVERAGE_SOURCE_DATA_ELEM_COUNT  3
#include "MovingAverageFilterCS.hlsli"

#if 0
[numthreads(SHADOW_MAP_TEXTURE_SIZE, 1, 1)]
//[numthreads(1024, 1, 1)]
void csApplyMovingAverageFilter3ToShadowTex(uint3 globalGroupId : SV_GroupID, uint3 localGroupThreadId : SV_GroupThreadID)
{
	// スライス番号は Dispatch() 時に与える。
	// なお、すべてのスライスで作業用（水平ぼかし出力用）の一時バッファを使いまわすとしても、
	// RWTexture2D ではなく RWTexture2DArray を使うことになる。
	MovingAverageFilterCS3(localGroupThreadId.x, globalGroupId.x, globalGroupId.y, SHADOW_MAP_TEXTURE_SIZE, SrvCascadedShadowMapsTex, UniRWShadowSmoothingOutTex);
}
#else
static const int ShadowBlurComputingWorkSize = 1024;
//static const int ShadowBlurComputingWorkSize = 256;
[numthreads(ShadowBlurComputingWorkSize, 1, 1)]
void csApplyMovingAverageFilter3ToShadowTex(uint3 globalGroupId : SV_GroupID, uint3 localGroupThreadId : SV_GroupThreadID)
{
	MovingAverageFilterCS3(
		localGroupThreadId.x + ShadowBlurComputingWorkSize * globalGroupId.x,
		globalGroupId.y,
		globalGroupId.z,
		SHADOW_MAP_TEXTURE_SIZE, SrvCascadedShadowMapsTex, UniRWShadowSmoothingOutTex);
}
#endif

/////////////////////////////////////////////////////////////////////

// Fermi でも Kepler でも Occupancy を最大化できるローカル グループ サイズを選ぶ。
// AMD 向けには別のチューニング パラメータが適しているかも。
// NVIDIA では Warp 単位すなわち 32 の倍数、AMD では Wavefront 単位すなわち 64 の倍数がよいらしい。
// NVIDIA の場合は CUDA におけるチューニング知識がほぼそのままコンピュート シェーダーにも通用するはず。
// http://news.mynavi.jp/series/kepler_gpu/011/

#define MY_CS_REDUCTION_TILE_SIZE_X  (16)
#define MY_CS_REDUCTION_TILE_SIZE_Y  (16)
#define MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE  (MY_CS_REDUCTION_TILE_SIZE_X * MY_CS_REDUCTION_TILE_SIZE_Y)
groupshared float ReductionTileAccum[MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE];

// HACK: 任意サイズに対応する場合、定数バッファで画像サイズとグループ数（グリッド サイズ）を渡す。
static const uint4 UniComputeTotalTargetDataCount = { COMPUTING_TEMP_WORK_SIZE, COMPUTING_TEMP_WORK_SIZE, 1, 0 };
static const uint4 UniComputeGlobalDispatchGroupCount =
{
	// HACK: 実際には単純な除算ではなく、切り上げを行なう必要がある。
	(UniComputeTotalTargetDataCount.x / MY_CS_REDUCTION_TILE_SIZE_X),
	(UniComputeTotalTargetDataCount.y / MY_CS_REDUCTION_TILE_SIZE_Y),
	1,
	0
};

[numthreads(MY_CS_REDUCTION_TILE_SIZE_X, MY_CS_REDUCTION_TILE_SIZE_Y, 1)]
void csReductionTexture2DTo1D(
	uint3 globalGroupId : SV_GroupID,
	//uint3 localGroupThreadId : SV_GroupThreadID,
	//uint3 globalDispatchThreadId : SV_DispatchThreadID,
	uint localGroupIndex : SV_GroupIndex)
{
	const float4 src = UniReductionInputTex2D.Load(uint3(
		globalGroupId.x * MY_CS_REDUCTION_TILE_SIZE_X + globalGroupId.x,
		globalGroupId.y * MY_CS_REDUCTION_TILE_SIZE_Y + globalGroupId.y,
		0));
	ReductionTileAccum[localGroupIndex] = dot(src, RgbToYFactor4F);

#if 0
	// 下記は単純で分かりやすいが、ローカル グループ スレッドのうち先頭1つしか働いていない。並列リダクションの意味や意義が薄れる。
	if (localGroupIndex == 0)
	{
		GroupMemoryBarrierWithGroupSync();

		float sum = 0;
		[unroll]
		for (int i = 0; i < MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE; ++i)
		{
			sum += ReductionTileAccum[i];
		}
		UniReductionOutputTex1D[globalGroupId.y * UniComputeGlobalDispatchGroupCount.x + globalGroupId.x] = sum;
	}
#else
	// DirectX SDK 付属サンプルの HDRToneMappingCS11 は 8x8 = 64 サイズの各タイル内で、データを半分ずつローカル スレッド群により統合していくコードを
	// 手動でアンロールして記述していたが、分かりづらいしスレッド数のチューニングができないので、コンパイラによるアンロールを使うべき。
	// HLSL ループ アンロールに関する考察が下記にある。
	// http://wlog.flatlib.jp/item/1012
	// なお、NVIDIA は GPU による並列リダクションの最適化手順を示した「Optimizing Parallel Reduction in CUDA」という資料を公開している。
	// http://developer.download.nvidia.com/compute/cuda/1.1-Beta/x86_website/projects/reduction/doc/reduction.pdf
	// DirectCompute 版の「DirectCompute Optimizations and Best Practices」は下記。
	// http://on-demand.gputechconf.com/gtc/2010/presentations/S12312-DirectCompute-Pre-Conference-Tutorial.pdf
	// ちなみに NVIDIA の GLSL コンパイラでは #pragma optionNV (unroll all) というヒント属性を付けられるようだが、
	// GLSL ではマクロなりインライン関数なりを使って手動展開したほうがいいかもしれない。
	// OpenCL では CUDA のような #pragma unroll が使えるが、cl_nv_pragma_unroll という拡張扱いらしい（一応 AMD でも同じディレクティブが動作するらしい）。
	// また、単純な輝度総和の平均ではなく、対数平均を使ってトーン マッピングを行なう手法もある。
	// DirectX SDK 付属サンプルの HDRLighting では対数平均を使っている。

	// Parallel reduction algorithm follows
	[unroll]
	for (uint i = MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE / 2; i > 0; i /= 2)
	{
		GroupMemoryBarrierWithGroupSync();
		if (localGroupIndex < i)
		{
			ReductionTileAccum[localGroupIndex] += ReductionTileAccum[i + localGroupIndex];
		}
	}

	if (localGroupIndex == 0)
	{
		UniReductionOutputTex1D[globalGroupId.y * UniComputeGlobalDispatchGroupCount.x + globalGroupId.x] = ReductionTileAccum[0];
	}
#endif
}

[numthreads(MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE, 1, 1)]
void csReductionTexture1DTo1D(
	uint3 globalGroupId : SV_GroupID,
	//uint3 localGroupThreadId : SV_GroupThreadID,
	uint3 globalDispatchThreadId : SV_DispatchThreadID,
	uint localGroupIndex : SV_GroupIndex)
{
	ReductionTileAccum[localGroupIndex] = (globalDispatchThreadId.x < UniComputeTotalTargetDataCount.x) ? UniReductionInputTex1D[globalDispatchThreadId.x] : 0;

	[unroll]
	for (uint i = MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE / 2; i > 0; i /= 2)
	{
		GroupMemoryBarrierWithGroupSync();
		if (localGroupIndex < i)
		{
			ReductionTileAccum[localGroupIndex] += ReductionTileAccum[i + localGroupIndex];
		}
	}

	if (localGroupIndex == 0)
	{
		UniReductionOutputTex1D[globalGroupId.x] = ReductionTileAccum[0];
	}
}

/////////////////////////////////////////////////////////////////////

// HACK: 物性値は定数バッファ経由で渡す。
static const float WaveVelocity = 1.0f;
static const float DeltaT = 0.1f;
static const float Delta = 2.0f;
static const float WaveVelocityC = WaveVelocity * WaveVelocity * DeltaT * DeltaT / (Delta * Delta);

// 高速なグループ共有メモリを使わないのであれば、ピクセル シェーダーの代わりにコンピュート シェーダーを使う意義も薄れるが、
// それでも頂点シェーダーやラスタライザーをいちいち起動しないで済むのはメリット。

// 波動方程式（ステンシル計算）をコンピュート シェーダーで解く。拡散方程式（空間2階微分）も同様にして解けるはず。
// 拘束条件として、端に位置する座標の高さはゼロで固定する？
// 任意形状の水面を作る場合、同様に端に位置する箇所をを拘束してしまえばよい？
// 拘束条件をどうやって埋め込む？　波面テクスチャ（あるいは構造化バッファ）もしくは
// ディスプレースメント マッピング対象の頂点バッファ（BAB を使うか、あるいは構造化バッファ）の要素に 0 or 1 をとるメンバーを含めておく？
// 任意形状のとき、テクスチャを使わずに構造化バッファを使う場合、隣接点をどうやって求める？
// 隣接情報を頂点バッファもしくはインデックス バッファに与えて、BAB などで読み取らせる？
// テッセレーションで頂点を生成するのはパフォーマンス的に問題ありそう。前フレームの変位結果を格納しておくこともできない。
// 初期条件は CPU で与える？　Dynamic リソースは UAV を作れないので、Default リソースと分ける必要がある。
// UpdateSubresource() を使えば、Default リソースを直接更新できる。Dynamic リソースを経由する必要はない。
// しかし、Dynamic リソースを経由する方式のほうが高速になる模様。常に成り立つかどうかは不明（環境起因なのか対象起因なのかも不明）。
// 定数バッファ経由でオブジェクト（Obstacle）投入位置を指定するような初期化専用のシェーダーを作る？

#if 0
static const float Spring = 0.03f;
static const float2 AddWavePos = { 0.6f, 0.3f }; // TODO: クリック位置にする。
static const float AddWaveRad = 0.01f; // TODO: [0, 1] の乱数を乗じる。
//static const float AddWaveVelocity = 1.0f; // TODO: [-0.5, +0.5] の乱数を乗じる。
static const float AddWaveVelocity = 2.0f; // TODO: [-0.5, +0.5] の乱数を乗じる。

float4 CalcNextWave(uint2 index, uint2 size, float4 t0, float4 t1, float4 t2, float4 t3, float4 t4)
{
	// 速度の計算。
	// Y 座標には以前の速度、
	// X 座標には以前の位置が入っている。
	// もともとはエフェクトマニアックスという書籍からパクったピクセル シェーダーのコード。
	// 本来は波動方程式の解を数値計算で求めるとき、前回の情報だけでなく、前々回の情報も必要になるはずだが……
	// http://tpweb2.phys.konan-u.ac.jp/~keisan_butsuri/keisan_butsuri/pdf/taiko.pdf
	float velocity = t0.y + Spring * (t1.x + t2.x + t3.x + t4.x - t0.x * 4);

	// 位置の計算。
	const float position = t0.x + velocity;

	// 波の追加。
	const float2 newPos = { (float)index.x / (size.x - 1), (float)index.y / (size.y - 1) }; // 座標を [0, 1] に正規化。
	if (distance(AddWavePos, newPos) <= AddWaveRad)
	{
		velocity += AddWaveVelocity;
	}

	// 新しい位置と速度、傾き Tx, Tz の出力。
	// 現時点では Z と W に格納した傾きは使用していない。
	return float4(position, velocity, (t2.x - t1.x) * 0.5, (t4.x - t3.x) * 0.5);
}
#endif

float4 CalcNextWave(uint2 index, uint2 size, float4 t0, float4 t1, float4 t2, float4 t3, float4 t4, float4 prepre)
{
	const float velocity = WaveVelocityC * (t1.w + t2.w + t3.w + t4.w - t0.w * 4);
	const float position = 2.0 * t0.w - prepre.w + velocity;
#if 0
	// NOTE: 法線の計算には前フレームの位置情報を使うので、厳密には現フレームの法線ではない。
	// 前フレームの情報を使うことで、データの再読み出しコストを低減させることができる。
	// 厳密に計算する場合、カーネルをいったん終了させて別のカーネルを改めて起動する方法があるが、
	// すべての高さが求まったあとに改めて Dispatch するのは呼び出しコストがかかる。
	// メモリ バリアを使ってすべての高さが求まって UAV 経由の書込が完了するまで待機させておくとよいかも。
	// メモリ バリアであれば Dispatch しないので GPU 側で完結し、CPU との同期が不要だが、実際に比較してみないとどちらが高速なのかは分からない。
	// DeviceMemoryBarrier() で待機する。OpenCL の mem_fence(CLK_GLOBAL_MEM_FENCE) に相当。CUDA の __threadfence() に近い（完全互換ではない）。
	// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/04/09/c-amp-for-the-directcompute-programmer.aspx
	// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/04/10/c-amp-for-the-opencl-programmer.aspx
	// http://blogs.msdn.com/b/nativeconcurrency/archive/2012/04/11/c-amp-for-the-cuda-programmer.aspx
	// ちなみに下記は同期手段を持たないピクセル シェーダーにおいて UAV のデータ ハザード（RAW ハザード）が発生している典型例。
	// http://wlog.flatlib.jp/item/1412
	// HACK: グループ共有メモリ経由で一時計算結果を蓄積して使うようにするといいかも。グループをまたぐ部分をどうするのか、という問題が残るが……
	// AMD が公開している資料「Efficient Compute Shader Programming」で紹介されている手法を活用できないか？

	const float diffHx = (t2.w - t1.w); // X 方向の高さ差分。
	const float diffHz = (t4.w - t3.w); // Z 方向の高さ差分。
	const float planeLength = 300;
	const float oneGridLen = 2.0 * planeLength / COMPUTING_TEMP_WORK_SIZE;
	// 4近傍から傾きを求め、法線を算出する。
	const float3 tanvecx = float3(+oneGridLen, diffHx, 0);
	const float3 tanvecz = float3(0, diffHz, -oneGridLen);
	const float3 normal = cross(tanvecx, tanvecz);
	// 法線の正規化はピクセル シェーダー側で行なう。

	// 法線と新しい位置の出力。
	return float4(normal, position);
#else
	return float4(0, 0, 0, position);
#endif
}

// HACK: ローカル グループ サイズの組み合わせは複数パターン用意するなりして、ホスト プログラム側と共有する。
// OpenGL のコンピュート シェーダーは実行時にグループ数を指定することもできるようになっているので柔軟だが、
// DirectX コンピュート シェーダーにはその機能がない。
// 処理対象の総データ数を固定し、SV_GroupID や SV_GroupThreadID を使って
// 多次元インデックスを直接特定して最適化する場合は確かに必要ない機能だが、
// SV_DispatchThreadID をインデックスの特定に使う汎用目的ならば欲しい機能。
static const int ComputingThreadLocalGroupSizeX = 256;
static const int ComputingThreadLocalGroupSizeY = 1;

// 構造化バッファを使うバージョン。
float4 FetchHeightVelocityNormalMap(StructuredBuffer<float4> inBufferPre, StructuredBuffer<float4> inBufferPrePre, int2 index, int2 size)
{
	const float4 t0 = inBufferPre[index.y * size.x + index.x];
	const float4 t1 = (index.x - 1 >= 0) ?
		inBufferPre[index.y * size.x + (index.x - 1)] : (float4)0;
	const float4 t2 = (index.x + 1 < size.x) ?
		inBufferPre[index.y * size.x + (index.x + 1)] : (float4)0;
	const float4 t3 = (index.y - 1 >= 0) ?
		inBufferPre[(index.y - 1) * size.x + index.x] : (float4)0;
	const float4 t4 = (index.y + 1 < size.y) ?
		inBufferPre[(index.y + 1) * size.x + index.x] : (float4)0;

	const float4 prepre = inBufferPrePre[index.y * size.x + index.x];

	return CalcNextWave(index, size, t0, t1, t2, t3, t4, prepre);
}

groupshared float4 HeightMapAccum[ComputingThreadLocalGroupSizeX];

// テクスチャを使うバージョン。
float4 FetchHeightVelocityNormalMap(Texture2D<float> inTexMask, Texture2D<float4> inTexPre, Texture2D<float4> inTexPrePre, int2 index, int2 size)
{
#if 1
	// 中心点と隣接する上下左右4点を読み取る。
	// 範囲外はすべてゼロになる（固定端反射の境界条件、Dirichlet 境界条件）。
	// 
	// 防波堤や岩場の波打ち際では自由端反射（Neumann 境界条件）。境界条件をどうやって与える？
	// 京大の講義資料にヒントがある。
	// http://daigakunyuushikouryakunoheya.web.fc2.com/butsurinozatsugaku/hadou/suimennha/bouhateiyaiwabanidekirutakanami.html
	// http://d.hatena.ne.jp/inlinedivenaoki/20100115/1263572094
	// http://ocw.kyoto-u.ac.jp/ja/01-faculty-of-integrated-human-studies-jp/introduction-to-simulation/pdf/sim98-9.pdf
	// UNDONE: 自由端の場合、隣接要素のマスク値がゼロ、もしくは隣接インデックスが範囲外の場合、
	// その地点の値を t0 に等しくすればよいはず。
	// 逆に言えば、隣接インデックスが範囲内でかつ隣接要素のマスク値が正の場合、
	// Prev フレームのストレージからフェッチする。

	// 任意形状にする場合は、隣接インデックス情報を頂点バッファ＋BAB（あるいは専用の構造化バッファ）などから読み取るなりする方法がある。
	// ハイトマップと拘束条件マスクの 2D テクスチャを使ったほうがパフォーマンスは高いはず。コーディングもしやすい。
	// 物理的に2次元/3次元のシミュレーションを行なう場合、できるかぎり 2D/3D テクスチャを使ったほうがよい。
	// 各格子点に4成分以上のデータを格納する必要がある場合や、型の異なるメンバーを持つ構造体データを格納する場合に、構造化バッファや BAB の使用を検討する。

	const float4 t0 = inTexPre[index] * inTexMask[index];
#if 0
	const float4 t1 = (index.x - 1 >= 0) ?
		inTexPre[uint2(index.x - 1, index.y)] * inTexMask[uint2(index.x - 1, index.y)] : (float4)0;
	const float4 t2 = (index.x + 1 < size.x) ?
		inTexPre[uint2(index.x + 1, index.y)] * inTexMask[uint2(index.x + 1, index.y)] : (float4)0;
	const float4 t3 = (index.y - 1 >= 0) ?
		inTexPre[uint2(index.x, index.y - 1)] * inTexMask[uint2(index.x, index.y - 1)] : (float4)0;
	const float4 t4 = (index.y + 1 < size.y) ?
		inTexPre[uint2(index.x, index.y + 1)] * inTexMask[uint2(index.x, index.y + 1)] : (float4)0;
#else
	const float4 t1 = (index.x - 1 >= 0     && inTexMask[uint2(index.x - 1, index.y)] > 0) ?
		inTexPre[uint2(index.x - 1, index.y)] : t0;
	const float4 t2 = (index.x + 1 < size.x && inTexMask[uint2(index.x + 1, index.y)] > 0) ?
		inTexPre[uint2(index.x + 1, index.y)] : t0;
	const float4 t3 = (index.y - 1 >= 0     && inTexMask[uint2(index.x, index.y - 1)] > 0) ?
		inTexPre[uint2(index.x, index.y - 1)] : t0;
	const float4 t4 = (index.y + 1 < size.y && inTexMask[uint2(index.x, index.y + 1)] > 0) ?
		inTexPre[uint2(index.x, index.y + 1)] : t0;
#endif

#else
	// グループ共有メモリを使って隣接スレッドの読み込み結果を再利用すると高速化できるかも、と思ったが、ほとんど大差ない模様。
	// ハイトマップの他にマスク画像の読み込みも行なうが、あまり効かない。むしろ同期のオーバーヘッドで遅くなっているかも。
	// また、グリッドの1辺のサイズ（いわゆる系の大きさ＝システム サイズ）が、
	// ローカル グループ サイズの上限である1024を超える場合には、共有のメカニズム（アルゴリズム）に工夫が必要。
	// AMD の資料によると、カーネル半径が7以上の場合にコンピュート シェーダーのグループ共有メモリの効果がピクセル シェーダーを上回るらしいので、
	// カーネル半径1に相当する今回のコードでは、単純な水平方向の共有だけやっても威力を発揮できない。
	// 1スレッドが単一の格子点を処理するのではなく、複数の格子点を処理するようにしたほうがいいかも。レジスタに保存したデータを再利用できる。
	// あるいは256×1単位ではなく、16×16単位をグループ化して共有するとか……
	const float4 t0 = inTexPre[index] * inTexMask[index];
	const float4 t3 = (index.y - 1 >= 0) ?
		inTexPre[uint2(index.x, index.y - 1)] * inTexMask[uint2(index.x, index.y - 1)] : (float4)0;
	const float4 t4 = (index.y + 1 < size.y) ?
		inTexPre[uint2(index.x, index.y + 1)] * inTexMask[uint2(index.x, index.y + 1)] : (float4)0;
	HeightMapAccum[index.x] = t0;

	GroupMemoryBarrierWithGroupSync();

	const float4 t1 = (index.x - 1 >= 0) ?
		HeightMapAccum[index.x - 1] : (float4)0;
	const float4 t2 = (index.x + 1 < size.x) ?
		HeightMapAccum[index.x + 1] : (float4)0;
#endif

	const float4 prepre = inTexPrePre[index];

	return CalcNextWave(index, size, t0, t1, t2, t3, t4, prepre);
}


// 処理対象のデータ総数を汎用化する場合は、定数バッファなどを使う必要がある。
// また、スレッド総数（データ総数）をグループ サイズで割りきれない場合などは、オーバーランのチェックが必要になる。

[numthreads(ComputingThreadLocalGroupSizeX, ComputingThreadLocalGroupSizeY, 1)]
void csSimpleComputingTestSB(
	uint3 globalGroupId : SV_GroupID,
	uint3 localGroupThreadId : SV_GroupThreadID,
	uint3 globalDispatchThreadId : SV_DispatchThreadID)
{
	// RW 構造化バッファを使うバージョン。
	const uint dataCount = COMPUTING_TEMP_WORK_SIZE * COMPUTING_TEMP_WORK_SIZE;
	const uint indexOut = globalDispatchThreadId.x;
	//const uint2 index = { globalDispatchThreadId.x % COMPUTING_TEMP_WORK_SIZE, globalDispatchThreadId.x / COMPUTING_TEMP_WORK_SIZE };
	const int2 index = { localGroupThreadId.x, globalGroupId.x };
	const uint2 size = { COMPUTING_TEMP_WORK_SIZE, COMPUTING_TEMP_WORK_SIZE };
	if (indexOut < dataCount)
	{
		UniWaveSimOutputBuffer[indexOut] = FetchHeightVelocityNormalMap(UniWaveSimInputBuffer1, UniWaveSimInputBuffer0, index, size);
		//UniWaveSimOutputBuffer[indexOut] = float4(globalDispatchThreadId.x, globalGroupId.x, localGroupIndex, 1); // テスト。
	}
}

[numthreads(ComputingThreadLocalGroupSizeX, ComputingThreadLocalGroupSizeY, 1)]
void csSimpleComputingTestTex(uint3 globalGroupId : SV_GroupID, uint3 localGroupThreadId : SV_GroupThreadID)
{
	// RW テクスチャを使うバージョン。
	//const int2 index = { globalGroupId.x, localGroupThreadId.x };
	const int2 index = { localGroupThreadId.x, globalGroupId.x };
	const int2 size = { COMPUTING_TEMP_WORK_SIZE, COMPUTING_TEMP_WORK_SIZE };
	if (index.x < size.x && index.y < size.y)
	{
		// HACK: GetDimensions() を使ってシェーダー側でサイズ取得する？
		UniWaveSimOutputTex[index] = FetchHeightVelocityNormalMap(SrvWaveSimMaskTex, UniWaveSimInputTex1, UniWaveSimInputTex0, index, size);
	}
}


#include "RandomNumHelper.hlsli"


[numthreads(ComputingThreadLocalGroupSizeX, ComputingThreadLocalGroupSizeY, 1)]
void csUpdateRandomTable(uint3 globalDispatchThreadId : SV_DispatchThreadID)
{
	// Xorshift で生成される最終的な乱数は w 成分のみだが、乱数を一気に4つ生成して、書き換えてしまう？
	const uint index = globalDispatchThreadId.x;
	const uint4 random = UniRWRandomNumTable[index];
	UniRWRandomNumTable[index] = Xorshift128Random::CreateNext(random);
}

void UpdateFlakeParticle(uint globalIndex)
{
	MyFlakeVertex inoutData = UavRWFlakeParticleBuffer[globalIndex];

	// HACK: クォータニオンでの保持と、クォータニオンの積による回転を実装する。
	inoutData.Attitude.w += radians(0.1);

	UavRWFlakeParticleBuffer[globalIndex] = inoutData;
}

[numthreads(MY_FLAKE_CS_LOCAL_GROUP_SIZE, 1, 1)]
void csUpdateFlakeParticle(uint3 globalDispatchThreadId : SV_DispatchThreadID)
{
	[unroll]
	for (uint i = 0; i < MY_FLAKE_CS_PARTICLES_NUM_PER_THREAD; ++i)
	{
		const uint globalIndex = MY_FLAKE_CS_PARTICLES_NUM_PER_THREAD * globalDispatchThreadId.x + i;

		// Update the particle.
		UpdateFlakeParticle(globalIndex);
	}
}

/////////////////////////////////////////////////////////////////////
technique11 CSTechReductionTexture2DTo1D
{
	pass p0
	{
		SetComputeShader(CompileShader(cs_5_0, csReductionTexture2DTo1D()));
	}
}

technique11 CSTechReductionTexture1DTo1D
{
	pass p0
	{
		SetComputeShader(CompileShader(cs_5_0, csReductionTexture1DTo1D()));
	}
}

technique11 CSTechApplyGaussianBlurToDownSampledTex
{
	pass p0
	{
		SetComputeShader(CompileShader(cs_5_0, csApplyGaussianBlur19ToDownSampledTex()));
	}
}

technique11 CSTechApplyBlurToShadowTex
{
	pass p0
	{
		SetComputeShader(CompileShader(cs_5_0, csApplyMovingAverageFilter3ToShadowTex()));
	}
}

technique11 CSTechSimpleComputingTest
{
	pass p0
	{
#if 0
		SetComputeShader(CompileShader(cs_5_0, csSimpleComputingTestSB()));
#else
		SetComputeShader(CompileShader(cs_5_0, csSimpleComputingTestTex()));
#endif
	}
}

technique11 CSTechUpdateRandomTable
{
	pass p0
	{
		SetComputeShader(CompileShader(cs_4_0, csUpdateRandomTable()));
	}
}

technique11 CSTechUpdateFlakeParticle
{
	pass p0
	{
		SetComputeShader(CompileShader(cs_4_0, csUpdateFlakeParticle()));
	}
}
