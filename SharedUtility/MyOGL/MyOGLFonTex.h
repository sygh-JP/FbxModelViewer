#pragma once

#include "MyGLRAII.hpp"
#include "MyTextureHelper.hpp"


namespace MyOGL
{
	extern bool CreateFontAlphaTexture(const MyTextureHelper::TextureDataPack& srcData, _Out_ TextureResourcePtr& outTexture);
} // end of namespace
