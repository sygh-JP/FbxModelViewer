#include "stdafx.h"

#include "MyCascadedShadowHelpers.hpp"

// 旧 DirectX SDK June 2010 のサンプル、CascadedShadowsManager/VarianceShadowsManager から拝借。
// 旧サンプルのクラスに _DECLSPEC_ALIGN_16_ 修飾が付いていたのは、XMVECTOR, XMMATRIX をフィールドに含むクラスを
// ヒープ上に作成できるようにするためだったらしい。
// 一般的なストレージ用途には XMFLOAT4, XMFLOAT4X4 を使うのが吉。
// CBuffer 用の構造体などは、XMVECTOR と XMMATRIX を使って、さらにアライメント修飾しておく方法でもよいが、
// やはり取り回しのしやすさでは XMFLOAT4 や XMFLOAT4X4 に軍配が挙がる。
// サンプルにて定義されていた XNA::Frustum は DirectXMath にて DirectX::BoundingFrustum という代替クラスが用意されている。
// なお、サンプルは DXUT を使っていることもあり、右手系ではなく左手系のコードとなっているので、特に視錐台やビュー・射影変換に注意。
// ちなみに、サンプルではソフトシャドウ用のシャドウマップのぼかしに単純な 3x3 平滑化フィルター（移動平均）が使われていた。
// ガウスぼかしは確かに重いので、等倍で複数枚ぼかしが必要なカスケード シャドウマップでは妥当な判断だろう。


using namespace DirectX;


namespace
{
	// 直方体や視錐台の頂点数は絶対不変の定数で、何があろうとも 8 のままだが、分かりやすさのために名前付き定数を定義しておく。
	// カスケード シャドウの潜在的最大レベルも 8 なので、即値埋め込みだとまぎらわしい。

	static const int BOX_CORNER_POINTS_COUNT_8 = int(BoundingBox::CORNER_COUNT);
	static const int FRUSTUM_CORNER_POINTS_COUNT_8 = int(BoundingFrustum::CORNER_COUNT);

	//static const XMVECTORF32 g_vFLTMAX = { +FLT_MAX, +FLT_MAX, +FLT_MAX, +FLT_MAX };
	//static const XMVECTORF32 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	//static const XMVECTORF32 g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const XMVECTORF32 g_vMultiplySetzwToZero = { 1.0f, 1.0f, 0.0f, 0.0f };
	//static const XMVECTORF32 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };
	// XMVECTOR 用のグローバル定数は、DirectXMath にて g_XMFltMin, g_XMFltMax, g_XMOneHalf, g_XMZero などが用意されている。


	//--------------------------------------------------------------------------------------
	// This function takes the camera's projection matrix and returns the 8
	// points that make up a view frustum.
	// The frustum is scaled to fit within the Begin and End interval parameters.
	//--------------------------------------------------------------------------------------
	static void CreateFrustumPointsFromCascadeInterval(
		float fCascadeIntervalBegin,
		float fCascadeIntervalEnd,
		const XMMATRIX& vProjection,
		XMVECTOR pvCornerPointsWorld[FRUSTUM_CORNER_POINTS_COUNT_8])
	{

		BoundingFrustum vViewFrust;
		BoundingFrustum::CreateFromMatrix(vViewFrust, vProjection);
#ifdef USE_LEFT_HAND_COORD_SYS
		vViewFrust.Near = fCascadeIntervalBegin;
		vViewFrust.Far = fCascadeIntervalEnd;
#else
		vViewFrust.Near = -fCascadeIntervalBegin;
		vViewFrust.Far = -fCascadeIntervalEnd;
#endif

#if 0
		static const XMVECTORU32 vGrabY = {0x00000000,0xFFFFFFFF,0x00000000,0x00000000};
		static const XMVECTORU32 vGrabX = {0xFFFFFFFF,0x00000000,0x00000000,0x00000000};
#else
		static const XMVECTORU32 vGrabY = { XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0 };
		static const XMVECTORU32 vGrabX = { XM_SELECT_1, XM_SELECT_0, XM_SELECT_0, XM_SELECT_0 };
#endif

		const XMVECTORF32 vRightTop = { vViewFrust.RightSlope, vViewFrust.TopSlope, 1.0f, 1.0f };
		const XMVECTORF32 vLeftBottom = { vViewFrust.LeftSlope, vViewFrust.BottomSlope, 1.0f, 1.0f };
		const XMVECTORF32 vNear = { vViewFrust.Near, vViewFrust.Near, vViewFrust.Near, 1.0f };
		const XMVECTORF32 vFar = { vViewFrust.Far, vViewFrust.Far, vViewFrust.Far, 1.0f };
		const XMVECTOR vRightTopNear = XMVectorMultiply( vRightTop, vNear );
		const XMVECTOR vRightTopFar = XMVectorMultiply( vRightTop, vFar );
		const XMVECTOR vLeftBottomNear = XMVectorMultiply( vLeftBottom, vNear );
		const XMVECTOR vLeftBottomFar = XMVectorMultiply( vLeftBottom, vFar );

		pvCornerPointsWorld[0] = vRightTopNear;
		pvCornerPointsWorld[1] = XMVectorSelect( vRightTopNear, vLeftBottomNear, vGrabX ); // LeftTopNear
		pvCornerPointsWorld[2] = vLeftBottomNear;
		pvCornerPointsWorld[3] = XMVectorSelect( vRightTopNear, vLeftBottomNear, vGrabY ); // RightBottomNear

		pvCornerPointsWorld[4] = vRightTopFar;
		pvCornerPointsWorld[5] = XMVectorSelect( vRightTopFar, vLeftBottomFar, vGrabX ); // LeftTopFar
		pvCornerPointsWorld[6] = vLeftBottomFar;
		pvCornerPointsWorld[7] = XMVectorSelect( vRightTopFar, vLeftBottomFar, vGrabY ); // RightBottomFar

	}

	//--------------------------------------------------------------------------------------
	// Used to compute an intersection of the orthographic projection and the Scene AABB
	//--------------------------------------------------------------------------------------
	struct Triangle
	{
		XMVECTOR pt[3];
		bool culled;
	};

	//--------------------------------------------------------------------------------------
	// Computing an accurate near and far plane will decrease surface acne and Peter-panning.
	// Surface acne is the term for erroneous self shadowing.  Peter-panning is the effect where
	// shadows disappear near the base of an object.
	// As offsets are generally used with PCF filtering due self shadowing issues, computing the
	// correct near and far planes becomes even more important.
	// This concept is not complicated, but the intersection code is.
	//--------------------------------------------------------------------------------------
	static void ComputeNearAndFar(
		float& fNearPlane,
		float& fFarPlane,
		FXMVECTOR vLightCameraOrthographicMin,
		FXMVECTOR vLightCameraOrthographicMax,
		const XMVECTOR pvPointsInCameraView[BOX_CORNER_POINTS_COUNT_8])
	{

		// Initialize the near and far planes
		fNearPlane = FLT_MAX;
		fFarPlane = -FLT_MAX;

		Triangle triangleList[BOX_CORNER_POINTS_COUNT_8 * 2] = {};
#if 0
		INT iTriangleCnt = 1;

		triangleList[0].pt[0] = pvPointsInCameraView[0];
		triangleList[0].pt[1] = pvPointsInCameraView[1];
		triangleList[0].pt[2] = pvPointsInCameraView[2];
		triangleList[0].culled = false;
#endif

		// These are the indices used to tessellate an AABB into a list of triangles.
		static const INT iAABBTriIndexes[] =
		{
			0,1,2,  1,2,3, // -Z
			4,5,6,  5,6,7, // +Z
			0,2,4,  2,4,6, // +X
			1,3,5,  3,5,7, // -X
			0,1,4,  1,4,5, // +Y
			2,3,6,  3,6,7, // -Y
		};
		// 面の向きは特に厳密に考慮しなくていいのか？　CW だったり CCW だったり統一性がないが……

		INT iPointPassesCollision[3] = {};

		// At a high level:
		// 1. Iterate over all 12 triangles of the AABB.
		// 2. Clip the triangles against each plane. Create new triangles as needed.
		// 3. Find the min and max z values as the near and far plane.

		// This is easier because the triangles are in camera spacing making the collisions tests simple comparisons.

		const float fLightCameraOrthographicMinX = XMVectorGetX( vLightCameraOrthographicMin );
		const float fLightCameraOrthographicMaxX = XMVectorGetX( vLightCameraOrthographicMax );
		const float fLightCameraOrthographicMinY = XMVectorGetY( vLightCameraOrthographicMin );
		const float fLightCameraOrthographicMaxY = XMVectorGetY( vLightCameraOrthographicMax );

		for( INT AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter )
		{
			triangleList[0].pt[0] = pvPointsInCameraView[ iAABBTriIndexes[ AABBTriIter*3 + 0 ] ];
			triangleList[0].pt[1] = pvPointsInCameraView[ iAABBTriIndexes[ AABBTriIter*3 + 1 ] ];
			triangleList[0].pt[2] = pvPointsInCameraView[ iAABBTriIndexes[ AABBTriIter*3 + 2 ] ];
			triangleList[0].culled = false;

			int iTriangleCnt = 1;

			// Clip each individual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles,
			// add them to the list.
			for( INT frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter )
			{

				float fEdge;
				INT iComponent;

				if( frustumPlaneIter == 0 )
				{
					fEdge = fLightCameraOrthographicMinX; // todo make float temp
					iComponent = 0;
				}
				else if( frustumPlaneIter == 1 )
				{
					fEdge = fLightCameraOrthographicMaxX;
					iComponent = 0;
				}
				else if( frustumPlaneIter == 2 )
				{
					fEdge = fLightCameraOrthographicMinY;
					iComponent = 1;
				}
				else
				{
					fEdge = fLightCameraOrthographicMaxY;
					iComponent = 1;
				}

				for( INT triIter = 0; triIter < iTriangleCnt; ++triIter )
				{
					// We don't delete triangles, so we skip those that have been culled.
					if( !triangleList[triIter].culled )
					{
						INT iInsideVertCount = 0;
						// Test against the correct frustum plane.
						// This could be written more compactly, but it would be harder to understand.

						if( frustumPlaneIter == 0 )
						{
							for( INT triPtIter=0; triPtIter < 3; ++triPtIter )
							{
								if( XMVectorGetX( triangleList[triIter].pt[triPtIter] ) >
									XMVectorGetX( vLightCameraOrthographicMin ) )
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}
						else if( frustumPlaneIter == 1 )
						{
							for( INT triPtIter=0; triPtIter < 3; ++triPtIter )
							{
								if( XMVectorGetX( triangleList[triIter].pt[triPtIter] ) <
									XMVectorGetX( vLightCameraOrthographicMax ) )
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}
						else if( frustumPlaneIter == 2 )
						{
							for( INT triPtIter=0; triPtIter < 3; ++triPtIter )
							{
								if( XMVectorGetY( triangleList[triIter].pt[triPtIter] ) >
									XMVectorGetY( vLightCameraOrthographicMin ) )
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}
						else
						{
							for( INT triPtIter=0; triPtIter < 3; ++triPtIter )
							{
								if( XMVectorGetY( triangleList[triIter].pt[triPtIter] ) <
									XMVectorGetY( vLightCameraOrthographicMax ) )
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}

						XMVECTOR tempOrder;
						// Move the points that pass the frustum test to the beginning of the array.
						if( iPointPassesCollision[1] && !iPointPassesCollision[0] )
						{
							tempOrder = triangleList[triIter].pt[0];
							triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
							triangleList[triIter].pt[1] = tempOrder;
							iPointPassesCollision[0] = 1;
							iPointPassesCollision[1] = 0;
						}
						if( iPointPassesCollision[2] && !iPointPassesCollision[1] )
						{
							tempOrder = triangleList[triIter].pt[1];
							triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
							triangleList[triIter].pt[2] = tempOrder;
							iPointPassesCollision[1] = 1;
							iPointPassesCollision[2] = 0;
						}
						if( iPointPassesCollision[1] && !iPointPassesCollision[0] )
						{
							tempOrder = triangleList[triIter].pt[0];
							triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
							triangleList[triIter].pt[1] = tempOrder;
							iPointPassesCollision[0] = 1;
							iPointPassesCollision[1] = 0;
						}

						if( iInsideVertCount == 0 )
						{
							// All points failed. We're done,
							triangleList[triIter].culled = true;
						}
						else if( iInsideVertCount == 1 )
						{
							// One point passed. Clip the triangle against the Frustum plane
							triangleList[triIter].culled = false;

							XMVECTOR vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
							XMVECTOR vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

							// Find the collision ratio.
							const float fHitPointTimeRatio = fEdge - XMVectorGetByIndex( triangleList[triIter].pt[0], iComponent );
							// Calculate the distance along the vector as ratio of the hit ratio to the component.
							const float fDistanceAlongVector01 = fHitPointTimeRatio / XMVectorGetByIndex( vVert0ToVert1, iComponent );
							const float fDistanceAlongVector02 = fHitPointTimeRatio / XMVectorGetByIndex( vVert0ToVert2, iComponent );
							// Add the point plus a percentage of the vector.
							vVert0ToVert1 *= fDistanceAlongVector01;
							vVert0ToVert1 += triangleList[triIter].pt[0];
							vVert0ToVert2 *= fDistanceAlongVector02;
							vVert0ToVert2 += triangleList[triIter].pt[0];

							triangleList[triIter].pt[1] = vVert0ToVert2;
							triangleList[triIter].pt[2] = vVert0ToVert1;

						}
						else if( iInsideVertCount == 2 )
						{
							// 2 in  // tessellate into 2 triangles

							// Copy the triangle\(if it exists) after the current triangle out of
							// the way so we can override it with the new triangle we're inserting.
							triangleList[iTriangleCnt] = triangleList[triIter+1];

							triangleList[triIter].culled = false;
							triangleList[triIter+1].culled = false;

							// Get the vector from the outside point into the 2 inside points.
							XMVECTOR vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
							XMVECTOR vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

							// Get the hit point ratio.
							const float fHitPointTime_2_0 = fEdge - XMVectorGetByIndex( triangleList[triIter].pt[2], iComponent );
							const float fDistanceAlongVector_2_0 = fHitPointTime_2_0 / XMVectorGetByIndex( vVert2ToVert0, iComponent );
							// Calculate the new vert by adding the percentage of the vector plus point 2.
							vVert2ToVert0 *= fDistanceAlongVector_2_0;
							vVert2ToVert0 += triangleList[triIter].pt[2];

							// Add a new triangle.
							triangleList[triIter+1].pt[0] = triangleList[triIter].pt[0];
							triangleList[triIter+1].pt[1] = triangleList[triIter].pt[1];
							triangleList[triIter+1].pt[2] = vVert2ToVert0;

							// Get the hit point ratio.
							const float fHitPointTime_2_1 = fEdge - XMVectorGetByIndex( triangleList[triIter].pt[2], iComponent );
							const float fDistanceAlongVector_2_1 = fHitPointTime_2_1 / XMVectorGetByIndex( vVert2ToVert1, iComponent );
							vVert2ToVert1 *= fDistanceAlongVector_2_1;
							vVert2ToVert1 += triangleList[triIter].pt[2];
							triangleList[triIter].pt[0] = triangleList[triIter+1].pt[1];
							triangleList[triIter].pt[1] = triangleList[triIter+1].pt[2];
							triangleList[triIter].pt[2] = vVert2ToVert1;
							// Increment triangle count and skip the triangle we just inserted.
							++iTriangleCnt;
							++triIter;

						}
						else
						{
							// all in
							triangleList[triIter].culled = false;

						}
					} // end if !culled loop
				}
			}
			for( INT index = 0; index < iTriangleCnt; ++index )
			{
				if( !triangleList[index].culled )
				{
					// Set the near and far plan and the min and max z values respectively.
					for( int vertind = 0; vertind < 3; ++vertind )
					{
						float fTriangleCoordZ = XMVectorGetZ( triangleList[index].pt[vertind] );
						if( fNearPlane > fTriangleCoordZ )
						{
							fNearPlane = fTriangleCoordZ;
						}
						if( fFarPlane < fTriangleCoordZ )
						{
							fFarPlane = fTriangleCoordZ;
						}
					}
				}
			}
		}

	}


	//--------------------------------------------------------------------------------------
	// This function converts the "center, extents" version of an AABB into 8 points.
	//--------------------------------------------------------------------------------------
	static void CreateAABBPoints(XMVECTOR vAABBPoints[BOX_CORNER_POINTS_COUNT_8], FXMVECTOR vCenter, FXMVECTOR vExtents)
	{
		// This map enables us to use a for-loop and do vector math.
		static const XMVECTORF32 vExtentsMap[BOX_CORNER_POINTS_COUNT_8] =
		{
			{ +1.0f, +1.0f, -1.0f, 1.0f }, // 0
			{ -1.0f, +1.0f, -1.0f, 1.0f }, // 1
			{ +1.0f, -1.0f, -1.0f, 1.0f }, // 2
			{ -1.0f, -1.0f, -1.0f, 1.0f }, // 3
			{ +1.0f, +1.0f, +1.0f, 1.0f }, // 4
			{ -1.0f, +1.0f, +1.0f, 1.0f }, // 5
			{ +1.0f, -1.0f, +1.0f, 1.0f }, // 6
			{ -1.0f, -1.0f, +1.0f, 1.0f }, // 7
		};

		for (INT index = 0; index < BOX_CORNER_POINTS_COUNT_8; ++index)
		{
			vAABBPoints[index] = XMVectorMultiplyAdd(vExtentsMap[index], vExtents, vCenter);
		}
	}

}

///////////////////////////////////////////////////////////////////////////////

MyShadowMapManager::MyShadowMapManager()
	: m_iCascadePartitionsMax(100)
	, m_fCascadePartitionsFrustum()
	, m_iCascadePartitionsZeroToOne()
	, m_iShadowBlurSize(3)
	, m_iBlurBetweenCascades()
	, m_fBlurBetweenCascadesAmount()
	, m_bMoveLightTexelSize(true)
	//, m_eSelectedCascadesFit(FIT_TO_CASCADES)
	, m_eSelectedCascadesFit(FIT_TO_SCENE)
	//, m_eSelectedNearFarFit(FIT_NEARFAR_ZERO_ONE)
	//, m_eSelectedNearFarFit(FIT_NEARFAR_AABB)
	, m_eSelectedNearFarFit(FIT_NEARFAR_SCENE_AABB)
	, m_vSceneAABBMin(MyMath::FLTMAX_VECTOR3F)
	, m_vSceneAABBMax(MyMath::FLTMIN_VECTOR3F)
	, m_matShadowView(MyMath::ZERO_MATRIX4X4F)
	, m_CopyOfCascadeConfig()
	//, m_pCascadeConfig()
{
	std::fill_n(m_matShadowProj, ARRAYSIZE(m_matShadowProj), MyMath::ZERO_MATRIX4X4F);

#if 1
	m_iCascadePartitionsZeroToOne[0] = 5;
	m_iCascadePartitionsZeroToOne[1] = 15;
	m_iCascadePartitionsZeroToOne[2] = 60;
	m_iCascadePartitionsZeroToOne[3] = 100;
	m_iCascadePartitionsZeroToOne[4] = 100;
	m_iCascadePartitionsZeroToOne[5] = 100;
	m_iCascadePartitionsZeroToOne[6] = 100;
	m_iCascadePartitionsZeroToOne[7] = 100;
#else
	m_iCascadePartitionsZeroToOne[0] = 100;
	m_iCascadePartitionsZeroToOne[1] = 100;
	m_iCascadePartitionsZeroToOne[2] = 100;
	m_iCascadePartitionsZeroToOne[3] = 100;
	m_iCascadePartitionsZeroToOne[4] = 100;
	m_iCascadePartitionsZeroToOne[5] = 100;
	m_iCascadePartitionsZeroToOne[6] = 100;
	m_iCascadePartitionsZeroToOne[7] = 100;
#endif

	// Pick some arbitrary intervals for the Cascade Maps
	//m_iCascadePartitionsZeroToOne[0] = 2;
	//m_iCascadePartitionsZeroToOne[1] = 4;
	//m_iCascadePartitionsZeroToOne[2] = 6;
	//m_iCascadePartitionsZeroToOne[3] = 9;
	//m_iCascadePartitionsZeroToOne[4] = 13;
	//m_iCascadePartitionsZeroToOne[5] = 26;
	//m_iCascadePartitionsZeroToOne[6] = 36;
	//m_iCascadePartitionsZeroToOne[7] = 70;
}

void MyShadowMapManager::InitFrame(
	const MyShadowCascadeConfig& cascadeConfig,
	const DirectX::XMFLOAT3& inSceneAABBMin,
	const DirectX::XMFLOAT3& inSceneAABBMax,
	const DirectX::XMFLOAT4X4& inSceneCameraView,
	const DirectX::XMFLOAT4X4& inSceneCameraProj,
	float inSceneCameraNearClip,
	float inSceneCameraFarClip,
	const DirectX::XMFLOAT4X4& inLightView)
{
	m_CopyOfCascadeConfig = cascadeConfig;

	const XMMATRIX matViewCameraProjection = XMLoadFloat4x4(&inSceneCameraProj);
	const XMMATRIX matViewCameraView = XMLoadFloat4x4(&inSceneCameraView);
	const XMMATRIX matLightCameraView = XMLoadFloat4x4(&inLightView);

	XMVECTOR det = g_XMZero;
	const XMMATRIX matInverseViewCamera = XMMatrixInverse(&det, matViewCameraView);

	m_vSceneAABBMin = inSceneAABBMin;
	m_vSceneAABBMax = inSceneAABBMax;

	// Convert from min max representation to center extents representation.
	// This will make it easier to pull the points out of the transformation.
#if 0
	const XMVECTOR vSceneCenter = XMVectorMultiply(XMVectorAdd(XMLoadFloat3(&m_vSceneAABBMax), XMLoadFloat3(&m_vSceneAABBMin)), g_XMOneHalf);
	const XMVECTOR vSceneExtents = XMVectorMultiply(XMVectorSubtract(XMLoadFloat3(&m_vSceneAABBMax), XMLoadFloat3(&m_vSceneAABBMin)), g_XMOneHalf);
#else
	const XMVECTOR vSceneCenter = (XMLoadFloat3(&m_vSceneAABBMax) + XMLoadFloat3(&m_vSceneAABBMin)) * g_XMOneHalf;
	const XMVECTOR vSceneExtents = (XMLoadFloat3(&m_vSceneAABBMax) - XMLoadFloat3(&m_vSceneAABBMin)) * g_XMOneHalf;
#endif

	XMVECTOR vSceneAABBPointsLightSpace[BOX_CORNER_POINTS_COUNT_8];
	// This function simply converts the center and extents of an AABB into 8 points
	CreateAABBPoints( vSceneAABBPointsLightSpace, vSceneCenter, vSceneExtents );

	// Transform the scene AABB to Light space.
	for (int index = 0; index < BOX_CORNER_POINTS_COUNT_8; ++index)
	{
		vSceneAABBPointsLightSpace[index] = XMVector4Transform( vSceneAABBPointsLightSpace[index], matLightCameraView );
	}

	//XMVECTOR vLightCameraOrthographicMin = g_XMZero;  // light space frustum AABB
	//XMVECTOR vLightCameraOrthographicMax = g_XMZero;

	const float fCameraNearFarRange = inSceneCameraFarClip - inSceneCameraNearClip;
	const float fCameraNearFarRangePercent = fCameraNearFarRange / float(m_iCascadePartitionsMax);

	// We loop over the cascades to calculate the orthographic projection for each cascade.
	for( uint32_t iCascadeIndex = 0; iCascadeIndex < cascadeConfig.m_nCascadeLevels; ++iCascadeIndex )
	{
		float fFrustumIntervalBegin = 0;

		// Calculate the interval of the View Frustum that this cascade covers. We measure the interval
		// the cascade covers as a Min and Max distance along the Z Axis.
		if (m_eSelectedCascadesFit == FIT_TO_CASCADES && iCascadeIndex != 0)
		{
			// Because we want to fit the orthographic projection tightly around the Cascade, we set the Mimimum cascade
			// value to the previous Frustum end Interval
			fFrustumIntervalBegin = float(m_iCascadePartitionsZeroToOne[iCascadeIndex - 1]) * fCameraNearFarRangePercent;
		}
#if 0
		else
		{
			// In the FIT_TO_SCENE technique the Cascades overlap each other.  In other words, interval 1 is covered by
			// cascades 1 to 8, interval 2 is covered by cascades 2 to 8 and so forth.
			fFrustumIntervalBegin = 0.0f;
		}
#endif

		// Scale the intervals between 0 and 1. They are now percentages that we can scale with.
		const float fFrustumIntervalEnd = float(m_iCascadePartitionsZeroToOne[iCascadeIndex]) * fCameraNearFarRangePercent;

		XMVECTOR vFrustumPoints[FRUSTUM_CORNER_POINTS_COUNT_8];

		// This function takes the began and end intervals along with the projection matrix and returns the 8
		// points that represent the cascade Interval
		CreateFrustumPointsFromCascadeInterval( fFrustumIntervalBegin, fFrustumIntervalEnd,
			matViewCameraProjection, vFrustumPoints );

		XMVECTOR vLightCameraOrthographicMin = g_XMFltMax;
		XMVECTOR vLightCameraOrthographicMax = g_XMFltMin;

		// This next section of code calculates the min and max values for the orthographic projection.
		for (int icpIndex = 0; icpIndex < FRUSTUM_CORNER_POINTS_COUNT_8; ++icpIndex)
		{
			// Transform the frustum from camera view space to world space.
			vFrustumPoints[icpIndex] = XMVector4Transform( vFrustumPoints[icpIndex], matInverseViewCamera );
			// Transform the point from world space to Light Camera Space.
			const XMVECTOR vTempTranslatedCornerPoint = XMVector4Transform( vFrustumPoints[icpIndex], matLightCameraView );
			// Find the closest point.
			vLightCameraOrthographicMin = XMVectorMin( vTempTranslatedCornerPoint, vLightCameraOrthographicMin );
			vLightCameraOrthographicMax = XMVectorMax( vTempTranslatedCornerPoint, vLightCameraOrthographicMax );
		}

		XMVECTOR vWorldUnitsPerTexel = g_XMZero;

		// This code removes the shimmering effect along the edges of shadows due to
		// the light changing to fit the camera.
		if( m_eSelectedCascadesFit == FIT_TO_SCENE )
		{
			// Fit the ortho projection to the cascades far plane and a near plane of zero.
			// Pad the projection to be the size of the diagonal of the Frustum partition.
			// 
			// To do this, we pad the ortho transform so that it is always big enough to cover
			// the entire camera view frustum.
			XMVECTOR vDiagonal = vFrustumPoints[0] - vFrustumPoints[6];
			vDiagonal = XMVector3Length( vDiagonal );

			// The bound is the length of the diagonal of the frustum interval.
			const float fCascadeBound = XMVectorGetX( vDiagonal );

			// The offset calculated will pad the ortho projection so that it is always the same size
			// and big enough to cover the entire cascade interval.
			XMVECTOR vBoarderOffset = ( vDiagonal -
				( vLightCameraOrthographicMax - vLightCameraOrthographicMin ) )
				* g_XMOneHalf;
			// Set the Z and W components to zero.
			vBoarderOffset *= g_vMultiplySetzwToZero;

			// Add the offsets to the projection.
			vLightCameraOrthographicMax += vBoarderOffset;
			vLightCameraOrthographicMin -= vBoarderOffset;

			// The world units per texel are used to snap the shadow the orthographic projection
			// to texel sized increments.  This keeps the edges of the shadows from shimmering.
			const float fWorldUnitsPerTexel = fCascadeBound / (float)cascadeConfig.m_iBufferSize;
			vWorldUnitsPerTexel = XMVectorSet( fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f );
		}
		else if( m_eSelectedCascadesFit == FIT_TO_CASCADES )
		{

			// We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're
			// sampling within the correct map.
			const float fScaleDuetoBlureAMT = ( (float)( m_iShadowBlurSize * 2 + 1 )
				/ (float)cascadeConfig.m_iBufferSize );
			const XMVECTORF32 vScaleDuetoBlureAMT = { fScaleDuetoBlureAMT, fScaleDuetoBlureAMT, 0.0f, 0.0f };
			// HACK: Blur のスペルミス？

			const float fNormalizeByBufferSize = ( 1.0f / (float)cascadeConfig.m_iBufferSize );
			const XMVECTOR vNormalizeByBufferSize = XMVectorSet( fNormalizeByBufferSize, fNormalizeByBufferSize, 0.0f, 0.0f );

			// We calculate the offsets as a percentage of the bound.
			XMVECTOR vBoarderOffset = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
			vBoarderOffset *= g_XMOneHalf;
			vBoarderOffset *= vScaleDuetoBlureAMT;
			vLightCameraOrthographicMax += vBoarderOffset;
			vLightCameraOrthographicMin -= vBoarderOffset;

			// The world units per texel are used to snap  the orthographic projection
			// to texel sized increments.
			// Because we're fitting tightly to the cascades, the shimmering shadow edges will still be present when the
			// camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
			vWorldUnitsPerTexel = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
			vWorldUnitsPerTexel *= vNormalizeByBufferSize;

		}
		else
		{
			_ASSERTE(false);
		}


		if( m_bMoveLightTexelSize )
		{

			// We snape the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
			// This is a matter of integer dividing by the world space size of a texel
			vLightCameraOrthographicMin /= vWorldUnitsPerTexel;
			vLightCameraOrthographicMin = XMVectorFloor( vLightCameraOrthographicMin );
			vLightCameraOrthographicMin *= vWorldUnitsPerTexel;

			vLightCameraOrthographicMax /= vWorldUnitsPerTexel;
			vLightCameraOrthographicMax = XMVectorFloor( vLightCameraOrthographicMax );
			vLightCameraOrthographicMax *= vWorldUnitsPerTexel;

		}

		// These are the unconfigured near and far plane values.  They are purposely awful to show
		// how important calculating accurate near and far planes is.
		float fNearPlane = 0.0f;
		float fFarPlane = 10000.0f;

		if( m_eSelectedNearFarFit == FIT_NEARFAR_AABB )
		{

			XMVECTOR vLightSpaceSceneAABBminValue = g_XMFltMax;  // world space scene AABB
			XMVECTOR vLightSpaceSceneAABBmaxValue = g_XMFltMin;
			// We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the
			// light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
			// and in some cases provides similar results.
			for(int index = 0; index < BOX_CORNER_POINTS_COUNT_8; ++index)
			{
				vLightSpaceSceneAABBminValue = XMVectorMin( vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBminValue );
				vLightSpaceSceneAABBmaxValue = XMVectorMax( vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBmaxValue );
			}

			// The min and max z values are the near and far planes.
			fNearPlane = XMVectorGetZ( vLightSpaceSceneAABBminValue );
			fFarPlane = XMVectorGetZ( vLightSpaceSceneAABBmaxValue );
		}
		else if( m_eSelectedNearFarFit == FIT_NEARFAR_SCENE_AABB )
		{
			// By intersecting the light frustum with the scene AABB we can get a tighter bound on the near and far plane.
			ComputeNearAndFar(fNearPlane, fFarPlane,
				vLightCameraOrthographicMin,
				vLightCameraOrthographicMax,
				vSceneAABBPointsLightSpace);
		}
		else
		{
			__noop;
		}

		// Create the orthographic projection for this cascade.
		XMStoreFloat4x4(&m_matShadowProj[ iCascadeIndex ],
#ifdef USE_LEFT_HAND_COORD_SYS
			XMMatrixOrthographicOffCenterLH(
#else
			XMMatrixOrthographicOffCenterRH(
#endif
			XMVectorGetX( vLightCameraOrthographicMin ),
			XMVectorGetX( vLightCameraOrthographicMax ),
			XMVectorGetY( vLightCameraOrthographicMin ),
			XMVectorGetY( vLightCameraOrthographicMax ),
			fNearPlane, fFarPlane));

		m_fCascadePartitionsFrustum[ iCascadeIndex ] = fFrustumIntervalEnd;
	}
	m_matShadowView = inLightView;
}
