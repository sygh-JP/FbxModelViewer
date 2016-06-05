#include "stdafx.h"
#include "MyD3DFonTex.h"
#include "MyDWriteWrapper.hpp"
//#include "MyUtil.h"

#include "DebugNew.h"


namespace MyD3D
{

	bool CreateFontAlphaTexture(ID3D11Device* pD3DDevice, const MyTextureHelper::TextureDataPack& srcData, _Out_ Microsoft::WRL::ComPtr<ID3D11Texture2D>& pTexture)
	{
		_ASSERTE(pD3DDevice != nullptr);
		_ASSERTE(pTexture == nullptr);
		_ASSERTE(srcData.TextureWidth % 4 == 0);
		_ASSERTE(srcData.TextureColorDepthInBits == 8);
		_ASSERTE(!srcData.TextureDib.empty());

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = srcData.TextureWidth;
		texDesc.Height = srcData.TextureHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_A8_UNORM; // 8bit アルファ マップを作る。
		texDesc.Usage = D3D11_USAGE_IMMUTABLE; // 作成時に内容も初期化する。
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.SampleDesc.Count = 1;

		const uint32_t stride = srcData.GetStrideInBytes();
		D3D11_SUBRESOURCE_DATA subresData = {};
		subresData.pSysMem = &srcData.TextureDib[0];
		subresData.SysMemPitch = stride;

		HRESULT hr = pD3DDevice->CreateTexture2D(&texDesc, &subresData, pTexture.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}
		return true;
	}

} // end of namespace
