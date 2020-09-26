#pragma once

#include "MyCommon.h"



namespace MyD3D
{
	// NOTE: もし COM のインターフェイス ポインタ配列を作る場合、Windows 専用だったら ATL::CInterfaceArray を使う手もあるけど、操作性が低いので std::vector を使うこと。

#pragma region // メッシュ関連。//


	//! @brief  Direct3D デバイス メッシュ パック。<br>
	//! 
	//! ID3DX10Mesh の代わり。<br>
	//! 1つの MyModelMesh に1つの MyDeviceMeshPack を配置し、属性テーブル（マテリアル テーブル）のストライド値およびオフセット値を <br>
	//! ID3D10Device::IASetVertexBuffers() あるいは ID3D11DeviceContext::IASetVertexBuffers() に渡すようにする。<br>
	//! 属性ごとに複数の頂点バッファ・複数のインデックス バッファに分けたりしない。<br>
	//! @attention  現状、プリミティブは D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST のみに対応。<br>
	//! テッセレーションを行なう場合、D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST が使われる。<br>
	class MyDeviceMeshPack final : MyUtils::MyNoncopyable<MyDeviceMeshPack>
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pTriListAdjIndexBuffer;
		uint32_t m_vertexStrideInBytes;
		uint32_t m_indexCount;
		uint32_t m_triListAdjIndexCount;
		DXGI_FORMAT m_indexFormat;
		MyMath::TMyAttributeRangeArray m_attrRangeArray;

		static const D3D_PRIMITIVE_TOPOLOGY m_topologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		static const D3D_PRIMITIVE_TOPOLOGY m_topologyAdjType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		static const D3D_PRIMITIVE_TOPOLOGY m_topologyPatchType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;

	public:
		MyDeviceMeshPack()
			: m_vertexStrideInBytes()
			, m_indexCount()
			, m_triListAdjIndexCount()
			, m_indexFormat()
		{
		}

	public:
		// ID3DX10Mesh::SetAttributeTable() の代わり。
		void SetAttributeRangeArray(const MyMath::TMyAttributeRangeArray& attrRangeArray) { m_attrRangeArray = attrRangeArray; }
		// ID3DX10Mesh::GetAttributeTable() の代わり。
		const MyMath::TMyAttributeRangeArray& GetAttributeRangeArray() const { return m_attrRangeArray; }

		//DXGI_FORMAT GetIndexFormat() const { return m_indexFormat; }

		void DrawSubset(ID3D11DeviceContext* pDeviceContext, size_t attribId, MyCommon::TopologyType topologyType = MyCommon::TopologyType_TriangleList) const;

	private:
		template<typename TVertex, typename TIndex, DXGI_FORMAT IndexFmt> bool CreateMeshImpl(ID3D11Device* pDevice,
			const TVertex pVertexArray[], uint32_t vertexCount,
			const TIndex pIndexArray[], uint32_t indexCount,
			const TIndex pAdjIndexArray[] = nullptr, uint32_t adjIndexCount = 0)
		{
			// NOTE: 頂点レイアウト オブジェクトはメッシュ クラス側では管理しないので、D3D_INPUT_ELEMENT_DESC を引数に受け取る必要はない。
			// たいていは、スキンメッシュ／非スキンメッシュの違いしかないので、レンダリング エンジン側で管理した方がいい。
			_ASSERTE(pDevice != nullptr);
			_ASSERTE(m_pVertexBuffer == nullptr);
			_ASSERTE(m_pIndexBuffer == nullptr);
			_ASSERTE(pVertexArray && vertexCount);
			_ASSERTE(pIndexArray && indexCount);

			HRESULT hr = E_FAIL;
			{
				D3D11_BUFFER_DESC vbDesc = {};
				vbDesc.ByteWidth = vertexCount * sizeof(TVertex);
				vbDesc.Usage = D3D11_USAGE_DEFAULT;
				vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				D3D11_SUBRESOURCE_DATA vbSubrData = {};
				vbSubrData.pSysMem = pVertexArray;

				hr = pDevice->CreateBuffer(&vbDesc, &vbSubrData, m_pVertexBuffer.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					return false;
				}
			}
			{
				D3D11_BUFFER_DESC ibDesc = {};
				ibDesc.ByteWidth = indexCount * sizeof(TIndex);
				ibDesc.Usage = D3D11_USAGE_DEFAULT;
				ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				D3D11_SUBRESOURCE_DATA ibSubrData = {};
				ibSubrData.pSysMem = pIndexArray;

				hr = pDevice->CreateBuffer(&ibDesc, &ibSubrData, m_pIndexBuffer.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					return false;
				}
			}
			if (pAdjIndexArray && adjIndexCount > 0)
			{
				D3D11_BUFFER_DESC ibDesc = {};
				ibDesc.ByteWidth = adjIndexCount * sizeof(TIndex);
				ibDesc.Usage = D3D11_USAGE_DEFAULT;
				ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				D3D11_SUBRESOURCE_DATA ibSubrData = {};
				ibSubrData.pSysMem = pAdjIndexArray;

				hr = pDevice->CreateBuffer(&ibDesc, &ibSubrData, m_pTriListAdjIndexBuffer.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					return false;
				}
			}

			m_vertexStrideInBytes = sizeof(TVertex);
			m_indexCount = indexCount;
			m_triListAdjIndexCount = adjIndexCount;
			m_indexFormat = IndexFmt;
			return true;
		}

	public:
		template<typename TVertex> bool CreateMesh(ID3D11Device* pDevice,
			const TVertex pVertexArray[], size_t vertexCount,
			const uint16_t pIndexArray[], size_t indexCount,
			const uint16_t pAdjIndexArray[] = nullptr, size_t adjIndexCount = 0)
		{
			// OpenGL は glBufferData() の引数が GLsizeiptr すなわち ptrdiff_t になっているため、
			// 64bit OS 環境であれば理論上は 4G を超えるインデックス バッファを扱えることになるが、
			// Direct3D は 64bit OS であっても 4G が限度。
			// 4G を超えるインデックスは映画制作のような超大規模プロジェクトでオフライン レンダリングするようなときには必要だと思うが、
			// 果たして 3DCG ゲームや CAD などのリアルタイム レンダリングでそんな大規模ジオメトリ データの GPU ハンドリングが必要になる時代が来るのか……

			return this->CreateMeshImpl<TVertex, uint16_t, DXGI_FORMAT_R16_UINT>(pDevice,
				pVertexArray, uint32_t(vertexCount), pIndexArray, uint32_t(indexCount), pAdjIndexArray, uint32_t(adjIndexCount));
		}

		template<typename TVertex> bool CreateMesh(ID3D11Device* pDevice,
			const TVertex pVertexArray[], size_t vertexCount,
			const uint32_t pIndexArray[], size_t indexCount,
			const uint32_t pAdjIndexArray[] = nullptr, size_t adjIndexCount = 0)
		{
			return this->CreateMeshImpl<TVertex, uint32_t, DXGI_FORMAT_R32_UINT>(pDevice,
				pVertexArray, uint32_t(vertexCount), pIndexArray, uint32_t(indexCount), pAdjIndexArray, uint32_t(adjIndexCount));
		}
#if 0
		// std::vector を直接受け取るオーバーロードはとりあえず封印。

		template<typename TVertex> bool CreateMesh(ID3D11Device* pDevice, const std::vector<TVertex>& vertexArray, const std::vector<uint16_t>& indexArray)
		{
			_ASSERTE(!vertexArray.empty() && !indexArray.empty());
			return this->CreateMeshImpl<TVertex, uint16_t, DXGI_FORMAT_R16_UINT>(
				pDevice, &vertexArray[0], uint32_t(vertexArray.size()), &indexArray[0], uint32_t(indexArray.size()));
		}

		template<typename TVertex> bool CreateMesh(ID3D11Device* pDevice, const std::vector<TVertex>& vertexArray, const std::vector<uint32_t>& indexArray)
		{
			_ASSERTE(!vertexArray.empty() && !indexArray.empty());
			return this->CreateMeshImpl<TVertex, uint32_t, DXGI_FORMAT_R32_UINT>(
				pDevice, &vertexArray[0], uint32_t(vertexArray.size()), &indexArray[0], uint32_t(indexArray.size()));
		}
#endif
	};

	typedef MyDeviceMeshPack MyModelMesh;
	typedef std::shared_ptr<MyModelMesh> TMyModelMeshPtr;
	typedef std::vector<TMyModelMeshPtr> TMyModelMeshPtrsArray;

	// HACK: メッシュ配列は XNA の Model クラスのように、クラス化してパッケージにする。

	// ディフューズ テクスチャ（アルベド）のように、変動しない（実行時に GPU で書き換えたりしない）テクスチャおよびそのビューを管理する。
	class MyTexture2DSRVPack final
	{
	public:
		Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture2D;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureSRV;
	public:
		MyTexture2DSRVPack() {}
	};

	typedef std::unordered_map<std::wstring, MyTexture2DSRVPack> TMyFileNameToTexture2DTable;

#pragma endregion

} // end of namespace
