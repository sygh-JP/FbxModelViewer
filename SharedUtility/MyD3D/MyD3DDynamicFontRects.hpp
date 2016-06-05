#pragma once


#include "MyBasicVertexTypes.hpp"


namespace MyD3D
{
	//! @brief  矩形集合の頂点バッファを管理する。<br>
	class MyDynamicManyRectsBase
	{
	public:
		typedef uint16_t TIndex;
		static const DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer;
		uint32_t m_rectCount;
		uint32_t m_vertexBufferSizeInBytes;

	public:
		ID3D11Buffer* GetVertexBuffer() { return m_pVertexBuffer.Get(); }
		ID3D11Buffer* GetIndexBuffer() { return m_pIndexBuffer.Get(); }

		uint32_t GetVertexBufferSizeInBytes() const { return m_vertexBufferSizeInBytes; }

	public:
		MyDynamicManyRectsBase()
			: m_rectCount()
			, m_vertexBufferSizeInBytes()
		{}

		virtual ~MyDynamicManyRectsBase()
		{}

		bool Create(ID3D11Device* pD3DDevice, UINT rectCount, UINT strideInBytes, const void* pInitialData);

		bool ReplaceVertexData(ID3D11DeviceContext* pDeviceContext, const void* pSrcData);
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

		bool CreateEx(ID3D11Device* pD3DDevice)
		{
			return this->Create(pD3DDevice, MaxCharacterCount, sizeof(TVertex), &m_vertexArray[0]);
		}

		// 垂直方向の2色線形グラデーションのみサポート。
		bool UpdateVertexBufferByString(ID3D11DeviceContext* pDeviceContext, LPCWSTR pString,
			MyMath::Vector2F posInPixels,
			const MyMath::Vector4F& upperColor, const MyMath::Vector4F& lowerColor,
			long fontHeight, bool usesFixedFeed,
			uint32_t fontTexWidth, uint32_t fontTexHeight,
			const MyMath::TCharCodeUVMap& codeUVMap);
	};

} // end of namespace
