#include "stdafx.h"

#include "FbxLayerAnalyzer.h"


#if 0

namespace
{
#if 0
	inline void GetNamePtr(LPCSTR& pDest, const FbxLayerElementTexture* pSrc)
	{
		if (pSrc)
		{
			pDest = pSrc->GetName();
		}
	}

	inline void GetNamePtr(LPCSTR& pDest, const FbxLayerElementMaterial* pSrc)
	{
		if (pSrc)
		{
			pDest = pSrc->GetName();
		}
	}
#endif

	template<typename T> inline std::string GetNameOf(const T* pSrc)
	{
		return pSrc ? pSrc->GetName() : std::string();
	}
}

namespace MyFbx
{
	MyFbxLayerAnalyzer::MyFbxLayerAnalyzer()
	{
	}

	MyFbxLayerAnalyzer::~MyFbxLayerAnalyzer()
	{
	}

	// レイヤー解析。
	bool MyFbxLayerAnalyzer::Analyze(const FbxLayer* layer)
	{

#if 0
		// FBX SDK 2010.2 以前。
		KFbxLayerElementTexture* ambientFactorTexture = layer->GetAmbientFactorTextures();
		KFbxLayerElementTexture* ambientTexture = layer->GetAmbientTextures();
		KFbxLayerElementTexture* diffuseFactorTexture = layer->GetDiffuseFactorTextures();
		KFbxLayerElementTexture* diffuseTexture = layer->GetDiffuseTextures();
		KFbxLayerElementTexture* emissiveFactorTexture = layer->GetEmissiveFactorTextures();
		KFbxLayerElementTexture* emissiveTexture = layer->GetEmissiveTextures();
		KFbxLayerElementTexture* reflectionFactorTexture = layer->GetReflectionFactorTextures();
		KFbxLayerElementTexture* reflectionTexture = layer->GetReflectionTextures();
		KFbxLayerElementTexture* specularFactorTexture = layer->GetSpecularFactorTextures();
		KFbxLayerElementTexture* specularTexture = layer->GetSpecularTextures();
		KFbxLayerElementTexture* transparencyFactorTexture = layer->GetTransparencyFactorTextures();
		KFbxLayerElementTexture* transparencyTexture = layer->GetTransparentTextures();

		KFbxLayerElementTexture* bumpTexture = layer->GetBumpTextures();
		KFbxLayerElementTexture* normalMapTexture = layer->GetNormalMapTextures();
		KFbxLayerElementTexture* shininessTexture = layer->GetShininessTextures();
		// --> いわゆるスペキュラーマップ（グロス マップ）のこと？
#elif 0
		// FBX SDK 2011.2 以降。
		KFbxLayerElementTexture* ambientFactorTexture = layer->GetTextures(KFbxLayerElement::eAMBIENT_FACTOR_TEXTURES);
		KFbxLayerElementTexture* ambientTexture = layer->GetTextures(KFbxLayerElement::eAMBIENT_TEXTURES);
		KFbxLayerElementTexture* diffuseFactorTexture = layer->GetTextures(KFbxLayerElement::eDIFFUSE_FACTOR_TEXTURES);
		KFbxLayerElementTexture* diffuseTexture = layer->GetTextures(KFbxLayerElement::eDIFFUSE_TEXTURES);
		KFbxLayerElementTexture* emissiveFactorTexture = layer->GetTextures(KFbxLayerElement::eEMISSIVE_FACTOR_TEXTURES);
		KFbxLayerElementTexture* emissiveTexture = layer->GetTextures(KFbxLayerElement::eEMISSIVE_TEXTURES);
		KFbxLayerElementTexture* reflectionFactorTexture = layer->GetTextures(KFbxLayerElement::eREFLECTION_FACTOR_TEXTURES);
		KFbxLayerElementTexture* reflectionTexture = layer->GetTextures(KFbxLayerElement::eREFLECTION_TEXTURES);
		KFbxLayerElementTexture* specularFactorTexture = layer->GetTextures(KFbxLayerElement::eSPECULAR_FACTOR_TEXTURES);
		KFbxLayerElementTexture* specularTexture = layer->GetTextures(KFbxLayerElement::eSPECULAR_TEXTURES);
		KFbxLayerElementTexture* transparencyFactorTexture = layer->GetTextures(KFbxLayerElement::eTRANSPARENCY_FACTOR_TEXTURES);
		KFbxLayerElementTexture* transparencyTexture = layer->GetTextures(KFbxLayerElement::eTRANSPARENT_TEXTURES);

		KFbxLayerElementTexture* bumpTexture = layer->GetTextures(KFbxLayerElement::eBUMP_TEXTURES);
		KFbxLayerElementTexture* normalMapTexture = layer->GetTextures(KFbxLayerElement::eNORMALMAP_TEXTURES);
		KFbxLayerElementTexture* shininessTexture = layer->GetTextures(KFbxLayerElement::eSHININESS_TEXTURES);
		// --> いわゆるスペキュラーマップ（グロス マップ）のこと？
#else
		// FBX SDK 2013.3 とか。
		const auto* ambientFactorTexture = layer->GetTextures(FbxLayerElement::eTextureAmbientFactor);
		const auto* ambientTexture = layer->GetTextures(FbxLayerElement::eTextureAmbient);
		const auto* diffuseFactorTexture = layer->GetTextures(FbxLayerElement::eTextureDiffuseFactor);
		const auto* diffuseTexture = layer->GetTextures(FbxLayerElement::eTextureDiffuse);
		const auto* emissiveFactorTexture = layer->GetTextures(FbxLayerElement::eTextureEmissiveFactor);
		const auto* emissiveTexture = layer->GetTextures(FbxLayerElement::eTextureEmissive);
		const auto* reflectionFactorTexture = layer->GetTextures(FbxLayerElement::eTextureReflectionFactor);
		const auto* reflectionTexture = layer->GetTextures(FbxLayerElement::eTextureReflection);
		const auto* specularFactorTexture = layer->GetTextures(FbxLayerElement::eTextureSpecularFactor);
		const auto* specularTexture = layer->GetTextures(FbxLayerElement::eTextureSpecular);
		const auto* transparencyFactorTexture = layer->GetTextures(FbxLayerElement::eTextureTransparencyFactor);
		const auto* transparencyTexture = layer->GetTextures(FbxLayerElement::eTextureTransparency);

		const auto* bumpTexture = layer->GetTextures(FbxLayerElement::eTextureBump);
		const auto* normalMapTexture = layer->GetTextures(FbxLayerElement::eTextureNormalMap);
		const auto* shininessTexture = layer->GetTextures(FbxLayerElement::eTextureShininess);
		// --> いわゆるスペキュラーマップ（グロス マップ）のこと？
#endif
		textureNameAry_[TextureType_AmbientFactor]        = GetNameOf(ambientFactorTexture);
		textureNameAry_[TextureType_Ambient]              = GetNameOf(ambientTexture);
		textureNameAry_[TextureType_Bump]                 = GetNameOf(bumpTexture);
		textureNameAry_[TextureType_DiffuseFactor]        = GetNameOf(diffuseFactorTexture);
		textureNameAry_[TextureType_Diffuse]              = GetNameOf(diffuseTexture);
		textureNameAry_[TextureType_EmissiveFactor]       = GetNameOf(emissiveFactorTexture);
		textureNameAry_[TextureType_Emissive]             = GetNameOf(emissiveTexture);
		textureNameAry_[TextureType_NormalMap]            = GetNameOf(normalMapTexture);
		textureNameAry_[TextureType_ReflectionFactor]     = GetNameOf(reflectionFactorTexture);
		textureNameAry_[TextureType_Reflection]           = GetNameOf(reflectionTexture);
		textureNameAry_[TextureType_Shininess]            = GetNameOf(shininessTexture);
		textureNameAry_[TextureType_SpecularFactor]       = GetNameOf(specularFactorTexture);
		textureNameAry_[TextureType_Specular]             = GetNameOf(specularTexture);
		textureNameAry_[TextureType_SpecularFactor]       = GetNameOf(specularFactorTexture);
		textureNameAry_[TextureType_Specular]             = GetNameOf(specularTexture);
		textureNameAry_[TextureType_TransparencyFactor]   = GetNameOf(transparencyFactorTexture);
		textureNameAry_[TextureType_Transparency]         = GetNameOf(transparencyTexture);

		const auto* material = layer->GetMaterials();
		materialName_ = GetNameOf(material);

		return true;
	}

} // end of namespace

#endif
