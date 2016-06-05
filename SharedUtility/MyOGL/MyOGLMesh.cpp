#include "stdafx.h"
#include "MyOGLMesh.h"


namespace MyOGL
{
	void MyDeviceMeshPack::DrawSubset(size_t attribId, MyCommon::TopologyType topologyType) const
	{
		const int indexStepCount = (topologyType == MyCommon::TopologyType_TriangleListAdj) ? 6 : 3;
		const uint32_t maxIndexCount = static_cast<uint32_t>((topologyType == MyCommon::TopologyType_TriangleListAdj) ? m_triListAdjIndexCount : m_indexCount);
		uint32_t indexCount = maxIndexCount;
		uint32_t indexStart = 0;
		if (attribId < m_attrRangeArray.size())
		{
			indexCount = m_attrRangeArray[attribId].FaceCount * indexStepCount;
			indexStart = m_attrRangeArray[attribId].FaceStart * indexStepCount;
		}
		_ASSERTE(maxIndexCount > indexStart && maxIndexCount >= indexCount);

		if (topologyType == MyCommon::TopologyType_TrianglePatchList)
		{
			// テッセレーションする場合もインデックス バッファは同じ。
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.get());
			// Direct3D は D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST などでプリミティブ種別と制御点の数を同時に設定するが、
			// OpenGL は設定 API が分かれているらしい。
			glPatchParameteri(GL_PATCH_VERTICES, m_patchCtrlPointCount);
		}
		else if (topologyType == MyCommon::TopologyType_TriangleListAdj)
		{
			// 隣接情報を使う場合はインデックス バッファが異なる。
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triListAdjIndexBuffer.get());
		}
		else
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.get());
		}

		// HACK: インデックスがない場合にも対応できるようにする？

		// 頂点バッファは頂点属性（頂点レイアウト）の設定があるので、呼び出し側で明示的にバインドできたほうがよい。
		// また、属性インデックスごとに頂点バッファをバインドし直す必要はないので、このメソッドの外でバインドしたほうが効率的。
		// インデックス バッファはこのメソッド内で暗黙的にバインドする。

		const uint8_t* pByteOffset = nullptr;
		pByteOffset += m_sizeOfOneIndex * indexStart;
		glDrawElements(m_topologyType, GLsizei(indexCount), m_indexFormat, pByteOffset);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

} // end of namespace
