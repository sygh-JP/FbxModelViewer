#pragma once

#if 0

namespace MyFbx
{
	// テクスチャタイプ
	enum TextureType
	{
		TextureType_AmbientFactor,
		TextureType_Ambient,
		TextureType_Bump,
		TextureType_DiffuseFactor,
		TextureType_Diffuse,
		TextureType_EmissiveFactor,
		TextureType_Emissive,
		TextureType_NormalMap,
		TextureType_ReflectionFactor,
		TextureType_Reflection,
		TextureType_Shininess,
		TextureType_SpecularFactor,
		TextureType_Specular,
		TextureType_TransparencyFactor,
		TextureType_Transparency,
		TextureType_Num
	};

	// テクスチャラベル
	const LPCSTR c_ppTextureLabelsArray[TextureType_Num] =
	{
		"Ambient Factor",
		"Ambient",
		"Bump",
		"Diffuse Factor",
		"Diffuse",
		"Emissive Factor",
		"Emissive",
		"NormalMap",
		"Reflection Factor",
		"Reflection",
		"Shininess",
		"Specular Factor",
		"Specular",
		"Transparency Factor",
		"Transparency",
	};

	inline LPCSTR GetTextureLabel(TextureType type)
	{
		return c_ppTextureLabelsArray[type];
	}

	inline LPCSTR GetTextureLabel(int index)
	{
		return (0 <= index && index < TextureType_Num) ? GetTextureLabel(static_cast<TextureType>(index)) : "";
	}


	class MyFbxLayerAnalyzer
	{
	public:
		typedef std::shared_ptr<MyFbxLayerAnalyzer> TSharedPtr;
		typedef std::shared_ptr<const MyFbxLayerAnalyzer> TConstSharedPtr;
	public:
		MyFbxLayerAnalyzer();
		virtual ~MyFbxLayerAnalyzer();

		// レイヤー解析
		virtual bool Analyze(const FbxLayer* layer);

		// テクスチャ名（FBX オブジェクト名）を取得
		LPCSTR GetTextureName(TextureType type) const
		{
			return textureNameAry_[type].c_str();
		}

		LPCSTR GetTextureName(int index) const
		{
			return (0 <= index && index < TextureType_Num) ? this->GetTextureName(static_cast<TextureType>(index)) : "";
		}

	private:
		// テクスチャ名
		std::string textureNameAry_[TextureType_Num];

		std::string materialName_; // マテリアル名
	};
}

#endif
