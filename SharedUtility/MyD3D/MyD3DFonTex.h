#pragma once

#include "MyTextureHelper.hpp"


namespace MyD3D
{
	extern bool CreateFontAlphaTexture(ID3D11Device* pD3DDevice, const MyTextureHelper::TextureDataPack& srcData, _Out_ Microsoft::WRL::ComPtr<ID3D11Texture2D>& pTexture);
} // end of namespace
