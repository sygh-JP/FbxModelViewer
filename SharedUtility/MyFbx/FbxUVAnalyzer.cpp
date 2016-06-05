#include "stdafx.h"

#include "FbxUVAnalyzer.h"


namespace
{
	inline float InverseTexCoord(float v)
	{
		return -v + 1;
	}

	bool ExtractUV(const FbxLayerElementUV* pElemUV, MyFbx::TUVArray& uvArray, MyFbx::LayerModeData& outModeData)
	{
		if (!pElemUV)
		{
			return false;
		}

		// UV の数・インデックス。
		const int uvNum = pElemUV->GetDirectArray().GetCount();
		const int indexNum = pElemUV->GetIndexArray().GetCount();
		uvArray.clear();

		// マッピングモード・リファレンスモード別に UV 取得。
		outModeData.MappingMode = pElemUV->GetMappingMode();
		outModeData.ReferenceMode = pElemUV->GetReferenceMode();

		if (outModeData.MappingMode == FbxLayerElement::eByPolygonVertex)
		{
			if (outModeData.ReferenceMode == FbxLayerElement::eDirect)
			{
				uvArray.resize(uvNum, MyMath::ZERO_VECTOR2F);
				// 直接取得。
				for (int i = 0; i < uvNum; ++i)
				{
					// FBX の UV 原点は OpenGL 同様左下らしい。
					// ちなみに Metasequoia や Direct3D は 2D 画像座標系よろしく左上原点となっている。
					// 今回は Direct3D に合わせておく。
					uvArray[i] = MyFbx::ToVector2F(pElemUV->GetDirectArray().GetAt(i));
					uvArray[i].y = InverseTexCoord(uvArray[i].y);
				}
			}
			else if (outModeData.ReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				uvArray.resize(indexNum, MyMath::ZERO_VECTOR2F);
				// UV インデックスから取得。
				for (int i = 0; i < indexNum; ++i)
				{
					const int index = pElemUV->GetIndexArray().GetAt(int(i));
					uvArray[i] = MyFbx::ToVector2F(pElemUV->GetDirectArray().GetAt(index));
					uvArray[i].y = InverseTexCoord(uvArray[i].y);
				}
			}
		}

		return true;
	}
}


namespace MyFbx
{

	bool MyFbxUVAnalyzer::Analyze(const FbxLayer* layer)
	{
		_ASSERTE(layer);
		m_diffuseUVArray.clear();
		m_specularUVArray.clear();
		m_ambientUVArray.clear();
		m_emissiveUVArray.clear();

		bool isAvailable = false;
		if (ExtractUV(layer->GetUVs(FbxLayerElement::eTextureDiffuse), m_diffuseUVArray, m_diffuseLayerModeData))
		{ isAvailable = true; }
		if (ExtractUV(layer->GetUVs(FbxLayerElement::eTextureSpecular), m_specularUVArray, m_specularLayerModeData))
		{ isAvailable = true; }
		if (ExtractUV(layer->GetUVs(FbxLayerElement::eTextureAmbient), m_ambientUVArray, m_ambientLayerModeData))
		{ isAvailable = true; }
		if (ExtractUV(layer->GetUVs(FbxLayerElement::eTextureEmissive), m_emissiveUVArray, m_emissiveLayerModeData))
		{ isAvailable = true; }

		return isAvailable;
	}

}
