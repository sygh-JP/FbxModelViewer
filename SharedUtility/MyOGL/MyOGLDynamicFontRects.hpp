#pragma once

#include "MyGLRAII.hpp"
#include "MyBasicVertexTypes.hpp"


namespace MyOGL
{
	//! @brief  矩形集合の頂点バッファを管理する。<br>
	class MyDynamicManyRectsBase
	{
	public:
		typedef uint16_t TIndex;
		static const GLenum IndexFormat = GL_UNSIGNED_SHORT;

	private:
		BufferResourcePtr m_vertexBuffer;
		// glDrawElements() には Direct3D とは違ってオフセット指定ができないので、
		// GPU 側のインデックス バッファでなく CPU 側のインデックス配列を使用する。
		std::vector<TIndex> m_indexArray;
		uint32_t m_rectCount;
		uint32_t m_vertexBufferSizeInBytes;

	public:
		GLuint GetVertexBuffer() const { return m_vertexBuffer.get(); }
		//const std::vector<TIndex>& GetIndexArray() const { return m_indexArray; }

		uint32_t GetVertexBufferSizeInBytes() const { return m_vertexBufferSizeInBytes; }

	public:
		MyDynamicManyRectsBase()
			: m_rectCount()
			, m_vertexBufferSizeInBytes()
		{}

		virtual ~MyDynamicManyRectsBase()
		{}

		bool Create(uint32_t rectCount, uint32_t strideInBytes, const void* pInitialData);

		bool ReplaceVertexData(const void* pSrcData);

#if 0
		void BindVertexBuffer() const
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.get());
		}

		static void UnbindVertexBuffer()
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
#endif

		void DrawElements(int rectCount) const
		{
			_ASSERTE(size_t(rectCount * 6) <= m_indexArray.size());
			if (rectCount > 0)
			{
				glDrawElements(GL_TRIANGLES, rectCount * 6, IndexFormat, &m_indexArray[0]);
			}
		}
	};


	//! @brief  フォント描画用の矩形集合を管理する。<br>
	class MyDynamicFontRects : public MyDynamicManyRectsBase
	{
	public:
		static const int MaxCharacterCount = 1024; //!< 一度に描画できる最大文字数。<br>
		static const int MaxVertexCount = MaxCharacterCount * 4;
		typedef MyVertexTypes::MyVertexPCT TVertex;

	private:
		std::vector<TVertex> m_vertexArray;
		int m_stringLength;

	public:

		MyDynamicFontRects()
			: m_vertexArray(MaxVertexCount)
			, m_stringLength()
		{}

		int GetStringLength() const { return m_stringLength; }

		bool CreateEx()
		{
			return this->Create(MaxCharacterCount, sizeof(TVertex), &m_vertexArray[0]);
		}

		// 垂直方向の2色線形グラデーションのみサポート。
		bool UpdateVertexBufferByString(LPCWSTR pString,
			MyMath::Vector2F posInPixels,
			const MyMath::Vector4F& upperColor, const MyMath::Vector4F& lowerColor,
			long fontHeight, bool usesFixedFeed,
			uint32_t fontTexWidth, uint32_t fontTexHeight,
			const MyMath::TCharCodeUVMap& codeUVMap);

		void DrawString() const
		{
			return this->DrawElements(this->GetStringLength());
		}
	};

} // end of namespace
