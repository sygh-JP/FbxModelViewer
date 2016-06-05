#include "stdafx.h"
#include "MyD3DMesh.h"


namespace MyD3D
{
	void MyDeviceMeshPack::DrawSubset(ID3D11DeviceContext* pDeviceContext, size_t attribId, MyCommon::TopologyType topologyType) const
	{
		const int indexStepCount = (topologyType == MyCommon::TopologyType_TriangleListAdj) ? 6 : 3;
		const uint32_t maxIndexCount = (topologyType == MyCommon::TopologyType_TriangleListAdj) ? m_triListAdjIndexCount : m_indexCount;
		uint32_t indexCount = maxIndexCount;
		uint32_t indexStart = 0;
		const UINT vertexStart = 0;
		if (attribId < m_attrRangeArray.size())
		{
			// 属性テーブルに基づき、インデックス バッファの一部を利用してレンダリングする。
			indexCount = m_attrRangeArray[attribId].FaceCount * indexStepCount;
			indexStart = m_attrRangeArray[attribId].FaceStart * indexStepCount;
			//vertexStart = m_attrRangeArray[attribId].VertexStart;
		}
		_ASSERTE(maxIndexCount > indexStart && maxIndexCount >= indexCount);

		// 1つだけの場合は別に一時配列に入れなくても OK。
		const UINT numBuffers = 1;
		const UINT pStrides[numBuffers] = { m_vertexStrideInBytes };
		const UINT pOffsets[numBuffers] = { 0 };
		ID3D11Buffer* const ppVBuffers[numBuffers] = { m_pVertexBuffer.Get() };
		pDeviceContext->IASetVertexBuffers(0, numBuffers, ppVBuffers, pStrides, pOffsets);

		if (topologyType == MyCommon::TopologyType_TrianglePatchList)
		{
			// テッセレーションする場合もインデックス バッファは同じ。
			pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), m_indexFormat, 0);
			pDeviceContext->IASetPrimitiveTopology(m_topologyPatchType);
		}
		else if (topologyType == MyCommon::TopologyType_TriangleListAdj)
		{
			// 隣接情報を使う場合はインデックス バッファが異なる。
			pDeviceContext->IASetIndexBuffer(m_pTriListAdjIndexBuffer.Get(), m_indexFormat, 0);
			pDeviceContext->IASetPrimitiveTopology(m_topologyAdjType);
		}
		else
		{
			pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), m_indexFormat, 0);
			pDeviceContext->IASetPrimitiveTopology(m_topologyType);
		}

		// HACK: インデックスがない場合にも対応できるようにする？

		// 頂点バッファは頂点属性（頂点レイアウト）の設定があるので、呼び出し側で明示的にバインドできたほうがよい。
		// また、属性インデックスごとに頂点バッファをバインドし直す必要はないので、このメソッドの外でバインドしたほうが効率的。
		// インデックス バッファはこのメソッド内で暗黙的にバインドする。

		pDeviceContext->DrawIndexed(indexCount, indexStart, vertexStart);
	}
} // end of namespace
