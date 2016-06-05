#pragma once

#include "MyCommon.h"
#include "MyGLRAII.hpp"


namespace MyOGL
{

#pragma region // メッシュ関連。//


	//! @brief  OpenGL デバイス メッシュ パック。<br>
	//! 
	//! @attention  現状、プリミティブは GL_TRIANGLES のみに対応。<br>
	class MyDeviceMeshPack final : boost::noncopyable
	{
	private:
		BufferResourcePtr m_vertexBuffer;
		BufferResourcePtr m_indexBuffer;
		BufferResourcePtr m_triListAdjIndexBuffer;

		// OpenGL でのインデックス バッファのバインドは、glBindBuffer() と GL_ELEMENT_ARRAY_BUFFER を使って行ない、
		// さらに glDrawElements() で描画する際に indices パラメータに NULL を指定するが、
		// glDrawElements() では Direct3D と違って（バインド中のインデックス バッファに対する）オフセット指定ができないので、
		// 属性テーブルを使った描画に支障が出る。
		// そのため、GPU 側のインデックス バッファでなく CPU 側のインデックス配列を使用する。
		// ……と思ったが、インデックス バッファをバインドしている場合、
		// glDrawElements() の indices には絶対ポインタではなくオフセット サイズを指定することができるらしい。
		// glVertexAttribPointer() の pointer パラメータと同じく、非常に分かりづらい（普通はオフセット指定にポインタ型を使うべきではない）。
		// OpenGL.org の API 説明も決して十分であるとは思えない。
		// http://www.opengl.org/sdk/docs/man/xhtml/glDrawElements.xml
		// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribPointer.xml

		uint32_t m_vertexStrideInBytes;
		size_t m_indexCount;
		size_t m_triListAdjIndexCount;
		GLenum m_indexFormat;
		size_t m_sizeOfOneIndex;
		MyMath::TMyAttributeRangeArray m_attrRangeArray;

		static const GLenum m_topologyType = GL_TRIANGLES;
		static const GLenum m_topologyAdjType = GL_TRIANGLES_ADJACENCY;
		static const GLenum m_topologyPatchType = GL_PATCHES;
		static const GLint m_patchCtrlPointCount = 3;

	public:
		MyDeviceMeshPack()
			: m_vertexStrideInBytes()
			, m_indexCount()
			, m_triListAdjIndexCount()
			, m_indexFormat()
			, m_sizeOfOneIndex()
		{
		}

	public:
		void SetAttributeRangeArray(const MyMath::TMyAttributeRangeArray& attrRangeArray) { m_attrRangeArray = attrRangeArray; }
		const MyMath::TMyAttributeRangeArray& GetAttributeRangeArray() const { return m_attrRangeArray; }

		GLuint GetVertexBuffer() const { return m_vertexBuffer.get(); }

#if 0
		// このクラスに Bind/Unbind メソッドを作るのはちょっとオブジェクト指向的でないので却下。
		void BindVertexBuffer() const
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.get());
		}

		static void UnbindVertexBuffer()
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
#endif

		//! @brief  頂点バッファおよびインデックス バッファを使用してメッシュのサブセットを描画する。<br>
		//! @pre  頂点バッファと頂点レイアウト（VBO と VAO）は事前に呼び出し側でバインドしておくこと。<br>
		void DrawSubset(size_t attribId, MyCommon::TopologyType topologyType = MyCommon::TopologyType_TriangleList) const;

	private:
		template<typename TVertex, typename TIndex, GLenum IndexFmt> bool CreateMeshImpl(
			const TVertex pVertexArray[], size_t vertexCount,
			const TIndex pIndexArray[], size_t indexCount,
			const TIndex pAdjIndexArray[] = nullptr, size_t adjIndexCount = 0)
		{
			_ASSERTE(m_vertexBuffer.get() == 0);
			_ASSERTE(m_indexBuffer.get() == 0);
			_ASSERTE(pVertexArray && vertexCount);
			_ASSERTE(pIndexArray && indexCount);

			{
				m_vertexBuffer = Factory::CreateOneBufferPtr();
				_ASSERTE(m_vertexBuffer.get() != 0);
				if (!m_vertexBuffer)
				{
					return false;
				}
				glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.get());
				glBufferData(GL_ARRAY_BUFFER, sizeof(TVertex) * vertexCount, pVertexArray, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			{
				m_indexBuffer = Factory::CreateOneBufferPtr();
				_ASSERTE(m_indexBuffer.get() != 0);
				if (!m_indexBuffer)
				{
					return false;
				}
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.get());
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TIndex) * indexCount, pIndexArray, GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			if (pAdjIndexArray && adjIndexCount > 0)
			{
				m_triListAdjIndexBuffer = Factory::CreateOneBufferPtr();
				_ASSERTE(m_triListAdjIndexBuffer.get() != 0);
				if (!m_triListAdjIndexBuffer)
				{
					return false;
				}
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triListAdjIndexBuffer.get());
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TIndex) * adjIndexCount, pAdjIndexArray, GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}

			m_vertexStrideInBytes = sizeof(TVertex);
			m_indexCount = indexCount;
			m_triListAdjIndexCount = adjIndexCount;
			m_indexFormat = IndexFmt;
			m_sizeOfOneIndex = sizeof(TIndex);

			return true;
		}

	public:
		template<typename TVertex> bool CreateMesh(
			const TVertex pVertexArray[], size_t vertexCount,
			const uint16_t pIndexArray[], size_t indexCount,
			const uint16_t pAdjIndexArray[] = nullptr, size_t adjIndexCount = 0)
		{
			return this->CreateMeshImpl<TVertex, uint16_t, GL_UNSIGNED_SHORT>(
				pVertexArray, vertexCount, pIndexArray, indexCount, pAdjIndexArray, adjIndexCount);
		}

		template<typename TVertex> bool CreateMesh(
			const TVertex pVertexArray[], size_t vertexCount,
			const uint32_t pIndexArray[], size_t indexCount,
			const uint32_t pAdjIndexArray[] = nullptr, size_t adjIndexCount = 0)
		{
			return this->CreateMeshImpl<TVertex, uint32_t, GL_UNSIGNED_INT>(
				pVertexArray, vertexCount, pIndexArray, indexCount, pAdjIndexArray, adjIndexCount);
		}
#if 0
		// std::vector を直接受け取るオーバーロードはとりあえず封印。

		template<typename TVertex> bool CreateMesh(const std::vector<TVertex>& vertexArray, const std::vector<uint16_t>& indexArray)
		{
			_ASSERTE(!vertexArray.empty() && !indexArray.empty());
			return this->CreateMeshImpl<TVertex, uint16_t, GL_UNSIGNED_SHORT>(
				&vertexArray[0], vertexArray.size(), &indexArray[0], indexArray.size());
		}

		template<typename TVertex> bool CreateMesh(const std::vector<TVertex>& vertexArray, const std::vector<uint32_t>& indexArray)
		{
			_ASSERTE(!vertexArray.empty() && !indexArray.empty());
			return this->CreateMeshImpl<TVertex, uint32_t, GL_UNSIGNED_INT>(
				&vertexArray[0], vertexArray.size(), &indexArray[0], indexArray.size());
		}
#endif
	};

	typedef MyDeviceMeshPack MyModelMesh;
	typedef std::shared_ptr<MyModelMesh> TMyModelMeshPtr;
	typedef std::vector<TMyModelMeshPtr> TMyModelMeshPtrsArray;

	// HACK: メッシュ配列は XNA の Model クラスのように、クラス化してパッケージにする。


	typedef std::unordered_map<std::wstring, TextureResourcePtr> TMyFileNameToTexture2DTable;

#pragma endregion

} // end of namespace
