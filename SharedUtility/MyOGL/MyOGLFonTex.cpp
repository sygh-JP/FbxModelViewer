#include "stdafx.h"
#include "MyOGLFonTex.h"
#include "MyDWriteWrapper.hpp"
//#include "MyUtil.h"

#include "DebugNew.h"


namespace MyOGL
{

	bool CreateFontAlphaTexture(const MyTextureHelper::TextureDataPack& srcData, _Out_ TextureResourcePtr& outTexture)
	{
		_ASSERTE(outTexture.get() == 0);
		_ASSERTE(srcData.TextureWidth % 4 == 0);
		_ASSERTE(!srcData.TextureDib.empty());

		outTexture = Factory::CreateOneTexturePtr();
		_ASSERTE(outTexture.get() != 0);
		if (outTexture.get() == 0)
		{
			return false;
		}

		glBindTexture(GL_TEXTURE_2D, outTexture.get());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		// 回転・拡大縮小を伴う場合は、GL_NEAREST でなく GL_LINEAR を使ったほうがよさげ。
		// ただしシェーダー側でバイリニア補間に伴うテクセルのずれを考慮する必要がある。
#if 0
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
		// 8bit アルファ マップを作る。
		const int inoutPixelFormat = GL_ALPHA;
		glTexImage2D(GL_TEXTURE_2D, 0, inoutPixelFormat,
			srcData.TextureWidth,
			srcData.TextureHeight,
			0, inoutPixelFormat, GL_UNSIGNED_BYTE, &srcData.TextureDib[0]);
		const auto lastError = glGetError();

		glBindTexture(GL_TEXTURE_2D, 0);

		return lastError == GL_NO_ERROR;
	}

} // end of namespace
