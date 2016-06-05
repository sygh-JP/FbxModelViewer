#pragma once


namespace MyAppCommonConsts
{
	const uint32_t MY_CS_REDUCTION_TILE_SIZE_X = 16;
	const uint32_t MY_CS_REDUCTION_TILE_SIZE_Y = 16;
	const uint32_t MY_CS_REDUCTION_TILE_LOCAL_GROUP_SIZE = (MY_CS_REDUCTION_TILE_SIZE_X * MY_CS_REDUCTION_TILE_SIZE_Y);

#if 0
	const uint32_t DOWN_SAMPLED_TEX_SIZE = 256;
#else
	const uint32_t DOWN_SAMPLED_TEX_SIZE = 128;
#endif

	const uint32_t RANDOM_NUM_TABLE_DATA_COUNT = 256 * 256;

	const uint32_t COMPUTING_TEMP_WORK_SIZE = 256;
	const uint32_t ComputingTempDataCount = COMPUTING_TEMP_WORK_SIZE * COMPUTING_TEMP_WORK_SIZE;

	const int ComputingThreadLocalGroupSizeX = 256;
	const int ComputingThreadLocalGroupSizeY = 1;

	// HLSL コードの numthreads 属性、GLSL コードの layout に関係する。
	const int ComputingThreadGroupCount = ComputingThreadLocalGroupSizeX * ComputingThreadLocalGroupSizeY;
#if 0
	// データ総数（全スレッド数）がグループ総数で割り切れない場合。
	// 端数を処理する際はいったん余分に Dispatch して、コンピュート シェーダー側で動的分岐を使って不要な処理を省くとよいらしい。
	// http://wlog.flatlib.jp/item/1425
	const int ComputeDispatchCountX = (ComputingTempDataCount + ComputingThreadGroupCount - 1) / ComputingThreadGroupCount;
#else
	static_assert(ComputingTempDataCount % ComputingThreadGroupCount == 0, "Not aligned!!");
	const int ComputeDispatchCountX = ComputingTempDataCount / ComputingThreadGroupCount;
#endif
}
