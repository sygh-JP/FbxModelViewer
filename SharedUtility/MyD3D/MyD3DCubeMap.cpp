#include "stdafx.h"
#include "MyD3DCubeMap.h"

#include "DebugNew.h"

#include "MyDDSHelper.h"


namespace MyD3D
{
	static bool CreateCubeVisual(ID3D11Device* pDevice, Microsoft::WRL::ComPtr<ID3D11Buffer>& out)
	{
		// Create a vertex buffer consisting of a quad for visualization
		const D3D11_BUFFER_DESC bufferDesc =
		{
			6 * sizeof(CubeMapVertex),
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_VERTEX_BUFFER,
			0,
			0,
		};
		// HACK: ただの平面であればわざわざキューブマップごとにインスタンスを作る必要はない。
		// 再利用可能なインデックス バッファと正規化済み頂点バッファを使ってキューブマップを張り付ける。
		// 立方体にマッピングする場合も同様。
		const CubeMapVertex pQuads[6] =
		{
			{ MyMath::Vector3F(-1.0f, +1.0f, 0.5f), MyMath::Vector4F(0.0f, 0.0f, -1.0f, 0), MyMath::Vector2F(0.0f, 0.0f) },
			{ MyMath::Vector3F(+1.0f, +1.0f, 0.5f), MyMath::Vector4F(0.0f, 0.0f, -1.0f, 0), MyMath::Vector2F(1.0f, 0.0f) },
			{ MyMath::Vector3F(-1.0f, -1.0f, 0.5f), MyMath::Vector4F(0.0f, 0.0f, -1.0f, 0), MyMath::Vector2F(0.0f, 1.0f) },
			{ MyMath::Vector3F(-1.0f, -1.0f, 0.5f), MyMath::Vector4F(0.0f, 0.0f, -1.0f, 0), MyMath::Vector2F(0.0f, 1.0f) },
			{ MyMath::Vector3F(+1.0f, +1.0f, 0.5f), MyMath::Vector4F(0.0f, 0.0f, -1.0f, 0), MyMath::Vector2F(1.0f, 0.0f) },
			{ MyMath::Vector3F(+1.0f, -1.0f, 0.5f), MyMath::Vector4F(0.0f, 0.0f, -1.0f, 0), MyMath::Vector2F(1.0f, 1.0f) },
		};
		const D3D11_SUBRESOURCE_DATA initData =
		{
			pQuads,
			sizeof(pQuads),
			sizeof(pQuads),
		};
		return SUCCEEDED(pDevice->CreateBuffer(&bufferDesc, &initData, out.ReleaseAndGetAddressOf()));
	}

	// HACK: DirectX::ScratchImage を直接渡すメソッドにしたほうがいいかも。
	// GetSurfaceInfo() を引っ張ってきて使うようなことをしなくてもよくなる。

	bool MyCubeMap::Create(ID3D11Device* pDevice, DXGI_FORMAT texFormat, uint32_t imageWidth, uint32_t imageHeight, uint32_t mipLevels, const uint8_t* pImageData, size_t imageDataSizeInBytes)
	{
		// 未作成もしくは明示的に解放処理を呼んでいることが前提。

		_ASSERTE(m_pEnvMap == nullptr);

		if (mipLevels < 1)
		{
			_ASSERTE(false);
			return false;
		}

		// キューブマップ配列はサポートしない。
		// なお、テクスチャ配列やミップマップを使う場合、CreateTexture2D() の第2引数には
		// D3D11_SUBRESOURCE_DATA の配列を渡せばよいらしい。

		const uint32_t arraySize = 6;
		const uint32_t imageDepth = 1;

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = imageWidth;
		texDesc.Height = imageHeight;
		texDesc.MipLevels = mipLevels;
		texDesc.ArraySize = arraySize;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Format = texFormat;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		std::vector<D3D11_SUBRESOURCE_DATA> initDataArray(mipLevels * arraySize);

		size_t offset = 0;

		for (uint32_t a = 0; a < arraySize; ++a)
		{
			size_t mipWidth = imageWidth, mipHeight = imageHeight, mipDepth = imageDepth;
			for (uint32_t m = 0; m < mipLevels; ++m)
			{
				size_t mipSizeInBytes = 0, mipRowBytes = 0, mipNumRows = 0;
				// DDS 圧縮されているときは、圧縮フォーマットに応じたブロック サイズをもとにした計算が必要。
				// なお、DDSTextureLoader などの内部実装ヘルパー関数を使わずとも、
				// DirectX::TexMetadata の情報をもとにして
				// DirectX::ScratchImage::GetImage() でミップマップの各データ領域を取得することはできる。
				// しかしパラメータの説明ぐらい書いて欲しい……
				// mip は分かるけど item が arrayIndex、slice が depthIndex だなんてぱっと見では分からない。
				MyDirectXTex::GetSurfaceInfo(imageWidth, imageHeight, texFormat, &mipSizeInBytes, &mipRowBytes, &mipNumRows);

				_ASSERTE(offset + mipSizeInBytes <= imageDataSizeInBytes);

				D3D11_SUBRESOURCE_DATA& resData = initDataArray[a * mipLevels + m];
				resData.pSysMem = pImageData + offset;
				resData.SysMemPitch = static_cast<uint32_t>(mipRowBytes);
				resData.SysMemSlicePitch = static_cast<uint32_t>(mipSizeInBytes);

				mipWidth = std::max<size_t>((mipWidth / 2), 1);
				mipHeight = std::max<size_t>((mipHeight / 2), 1);
				mipDepth = std::max<size_t>((mipDepth / 2), 1);

				offset += mipSizeInBytes;
			}
		}

		if (FAILED(pDevice->CreateTexture2D(&texDesc, &initDataArray[0], &m_pEnvMap)))
		{
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		if (FAILED(pDevice->CreateShaderResourceView(m_pEnvMap.Get(), &srvDesc, &m_pEnvMapSRV)))
		{
			return false;
		}

		return true;
	}


	bool MyRenderTargetCubeMap::Create(ID3D11Device* pDevice, uint32_t mipLevels, uint32_t envMapSizePixel)
	{
		_ASSERTE(m_pEnvMap == nullptr);

		{
			// Generate cube map view matrices
			const float fHeight = 1.5f;
			MyMath::Vector3F vEyePt = MyMath::Vector3F(0, fHeight, 0);
			MyMath::Vector3F vLookDir;
			MyMath::Vector3F vUpDir;

			vLookDir = MyMath::Vector3F(+1, fHeight, 0);
			vUpDir = MyMath::Vector3F(0, 1, 0);
			//D3DXMatrixLookAtLH(&m_amCubeMapViewAdjust[0], &vEyePt, &vLookDir, &vUpDir);
			MyMath::CreateMatrixLookAtLH(&m_amCubeMapViewAdjust[0], &vEyePt, &vLookDir, &vUpDir);

			vLookDir = MyMath::Vector3F(-1, fHeight, 0);
			vUpDir = MyMath::Vector3F(0, 1, 0);
			//D3DXMatrixLookAtLH(&m_amCubeMapViewAdjust[1], &vEyePt, &vLookDir, &vUpDir);
			MyMath::CreateMatrixLookAtLH(&m_amCubeMapViewAdjust[1], &vEyePt, &vLookDir, &vUpDir);

			vLookDir = MyMath::Vector3F(0, fHeight + 1.0f, 0);
			vUpDir = MyMath::Vector3F(0, 0, -1);
			//D3DXMatrixLookAtLH(&m_amCubeMapViewAdjust[2], &vEyePt, &vLookDir, &vUpDir);
			MyMath::CreateMatrixLookAtLH(&m_amCubeMapViewAdjust[2], &vEyePt, &vLookDir, &vUpDir);

			vLookDir = MyMath::Vector3F(0, fHeight - 1.0f, 0);
			vUpDir = MyMath::Vector3F(0, 0, +1);
			//D3DXMatrixLookAtLH(&m_amCubeMapViewAdjust[3], &vEyePt, &vLookDir, &vUpDir);
			MyMath::CreateMatrixLookAtLH(&m_amCubeMapViewAdjust[3], &vEyePt, &vLookDir, &vUpDir);

			vLookDir = MyMath::Vector3F(0, fHeight, +1);
			vUpDir = MyMath::Vector3F(0, 1, 0);
			//D3DXMatrixLookAtLH(&m_amCubeMapViewAdjust[4], &vEyePt, &vLookDir, &vUpDir);
			MyMath::CreateMatrixLookAtLH(&m_amCubeMapViewAdjust[4], &vEyePt, &vLookDir, &vUpDir);

			vLookDir = MyMath::Vector3F(0, fHeight, -1);
			vUpDir = MyMath::Vector3F(0, 1, 0);
			//D3DXMatrixLookAtLH(&m_amCubeMapViewAdjust[5], &vEyePt, &vLookDir, &vUpDir);
			MyMath::CreateMatrixLookAtLH(&m_amCubeMapViewAdjust[5], &vEyePt, &vLookDir, &vUpDir);
		}

		// Create cubic depth stencil texture.
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = envMapSizePixel;
		texDesc.Height = envMapSizePixel;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 6;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Format = DXGI_FORMAT_D32_FLOAT;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		if (FAILED(pDevice->CreateTexture2D(&texDesc, nullptr, &m_pEnvMapDepth)))
		{
			return false;
		}

		// Create the depth stencil view for the entire cube
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 6;
		dsvDesc.Texture2DArray.MipSlice = 0;
		if (FAILED(pDevice->CreateDepthStencilView(m_pEnvMapDepth.Get(), &dsvDesc, &m_pEnvMapDSV)))
		{
			return false;
		}

		// Create the depth stencil view for single face rendering
		dsvDesc.Texture2DArray.ArraySize = 1;
		if (FAILED(pDevice->CreateDepthStencilView(m_pEnvMapDepth.Get(), &dsvDesc, &m_pEnvMapOneDSV)))
		{
			return false;
		}

		// Create the cube map for env map render target
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;
		// D3D11_RESOURCE_MISC_GENERATE_MIPS は D3D11_BIND_RENDER_TARGET と D3D11_BIND_SHADER_RESOURCE の両方のフラグを要求するらしい。
		// http://msdn.microsoft.com/ja-jp/library/ee416267.aspx
		texDesc.MipLevels = mipLevels;
		if (FAILED(pDevice->CreateTexture2D(&texDesc, nullptr, &m_pEnvMap)))
		{
			return false;
		}

		// Create the 6-face render target view
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = texDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 6;
		rtvDesc.Texture2DArray.MipSlice = 0;
		if (FAILED(pDevice->CreateRenderTargetView(m_pEnvMap.Get(), &rtvDesc, &m_pEnvMapRTV)))
		{
			return false;
		}

		// Create the one-face render target views
		rtvDesc.Texture2DArray.ArraySize = 1;
		for (int i = 0; i < 6; ++i)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			if (FAILED(pDevice->CreateRenderTargetView(m_pEnvMap.Get(), &rtvDesc, &m_apEnvMapOneRTV[i])))
			{
				return false;
			}
		}

		// Create the shader resource view for the cubic env map
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		if (FAILED(pDevice->CreateShaderResourceView(m_pEnvMap.Get(), &srvDesc, &m_pEnvMapSRV)))
		{
			return false;
		}

		return CreateCubeVisual(pDevice, m_pVBVisual);
	}
}
