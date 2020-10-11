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
		uvArray.clear();
		outModeData = {};

		if (!pElemUV)
		{
			return false;
		}

		// マッピングモード・リファレンスモード別に UV 取得。
		const auto mappingMode = pElemUV->GetMappingMode();
		const auto referenceMode = pElemUV->GetReferenceMode();
		outModeData.MappingMode = mappingMode;
		outModeData.ReferenceMode = referenceMode;

		if (mappingMode == FbxLayerElement::eByPolygonVertex)
		{
			if (referenceMode == FbxLayerElement::eDirect)
			{
				const int uvNum = pElemUV->GetDirectArray().GetCount();
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
			else if (referenceMode == FbxLayerElement::eIndexToDirect)
			{
				const int indexNum = pElemUV->GetIndexArray().GetCount();
				uvArray.resize(indexNum, MyMath::ZERO_VECTOR2F);
				// UV インデックスから取得。
				for (int i = 0; i < indexNum; ++i)
				{
					const int index = pElemUV->GetIndexArray().GetAt(i);
					uvArray[i] = MyFbx::ToVector2F(pElemUV->GetDirectArray().GetAt(index));
					uvArray[i].y = InverseTexCoord(uvArray[i].y);
				}
			}
		}

		return true;
	}
} // end of namespace


namespace MyFbx
{

	void MyFbxUVAnalyzer::Analyze(const FbxLayer* layer)
	{
		_ASSERTE(layer);

		ExtractUV(layer->GetUVs(FbxLayerElement::eTextureDiffuse), m_diffuseUVArray, m_diffuseLayerModeData);
		ExtractUV(layer->GetUVs(FbxLayerElement::eTextureSpecular), m_specularUVArray, m_specularLayerModeData);
		ExtractUV(layer->GetUVs(FbxLayerElement::eTextureAmbient), m_ambientUVArray, m_ambientLayerModeData);
		ExtractUV(layer->GetUVs(FbxLayerElement::eTextureEmissive), m_emissiveUVArray, m_emissiveLayerModeData);
	}

} // end of namespace
