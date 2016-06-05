#include "StdAfx.h"
#include "MyComplexPGProps.h"


namespace MyComplexPGProps
{

	CMFCPropertyGridProperty* MyCPGPVector4D::Create(const CString& strGroupName,
		const MyMath::Vector4D& initVal, const MyMath::Vector4D& minVal, const MyMath::Vector4D& maxVal, double delta)
	{
		_ASSERTE(m_pPropsGroupVectorParams == nullptr);

		// 構築する際に new したオブジェクトを CMFCPropertyGridCtrl のツリーに追加せずに途中 return すると当然メモリーリークするので注意すること。
		// 追加した後は、オブジェクトの delete は CMFCPropertyGridCtrl 側が行なう。

		m_pPropsGroupVectorParams = new CMFCPropertyGridProperty(strGroupName);

		//propsRoot.AddProperty(m_pPropsGroupVectorParams);

		const LPCTSTR ppNamesArray[] =
		{
			_T("X"),
			_T("Y"),
			_T("Z"),
			_T("W"),
		};
		for (int i = 0; i < 4; ++i)
		{
			CString strDesc;
			strDesc.Format(_T("Specifies the %s component of Vector4."), ppNamesArray[i]);
			auto& target = m_propsVector4D.ppVector[i];
			target = new CMyDoubleSpinPGProperty(ppNamesArray[i],
				(&initVal.X)[i],
				(&minVal.X)[i],
				(&maxVal.X)[i],
				delta,
				strDesc);

			m_pPropsGroupVectorParams->AddSubItem(target);
			//target->SetManagerCtrl(&propsRoot);
		}

		return m_pPropsGroupVectorParams;
	}

	void MyCPGPVector4D::SetProperties(const MyMath::Vector4D& inVal)
	{
		_ASSERTE(m_pPropsGroupVectorParams != nullptr);

		for (int i = 0; i < 4; ++i)
		{
			_ASSERTE(m_propsVector4D.ppVector[i] != nullptr);
			m_propsVector4D.ppVector[i]->SetCurrentValue((&inVal.X)[i]);
		}
	}

	void MyCPGPVector4D::GetProperties(MyMath::Vector4D& outVal)
	{
		_ASSERTE(m_pPropsGroupVectorParams != nullptr);

		for (int i = 0; i < 4; ++i)
		{
			_ASSERTE(m_propsVector4D.ppVector[i] != nullptr);
			(&outVal.X)[i] = m_propsVector4D.ppVector[i]->GetCurrentValue();
		}
	}

	void MyCPGPVector4D::Show(bool shows)
	{
		_ASSERTE(m_pPropsGroupVectorParams != nullptr);

		m_pPropsGroupVectorParams->Show(shows);
	}

	bool MyCPGPVector4D::IsVisible() const
	{
		_ASSERTE(m_pPropsGroupVectorParams != nullptr);

		return !!m_pPropsGroupVectorParams->IsVisible();
	}


	CMFCPropertyGridProperty* MyCPGPMaterial::Create(const MyMath::MyMaterial& initVal)
	{
		_ASSERTE(m_pPropsGroupMaterialParams == nullptr);

		m_pPropsGroupMaterialParams = new CMFCPropertyGridProperty(_T("Material"));

		//propsRoot.AddProperty(m_pPropsGroupMaterialParams);

		const double delta = 0.1;

		{
			m_pPropMaterialName = new CMyReadonlyPGProperty(_T("Name"),
				initVal.GetMaterialName().c_str(), _T("Material name."));
			m_pPropsGroupMaterialParams->AddSubItem(m_pPropMaterialName);

			// HACK: プロシージャル テクスチャ対応はどうする？
			m_pPropTexFileNameDiffuseMap = new CMyReadonlyPGProperty(_T("DiffuseMapFileName"),
				initVal.GetTexFileNameDiffuseMap().c_str(), _T("Texture file name of diffuse map."));
			m_pPropsGroupMaterialParams->AddSubItem(m_pPropTexFileNameDiffuseMap);

			m_pPropTexFileNameNormalMap = new CMyReadonlyPGProperty(_T("NormalMapFileName"),
				initVal.GetTexFileNameNormalMap().c_str(), _T("Texture file name of normal map."));
			m_pPropsGroupMaterialParams->AddSubItem(m_pPropTexFileNameNormalMap);
		}

		const LPCTSTR ppNamesArray[] =
		{
			_T("Diffuse"),
			_T("Ambient"),
			_T("Specular"),
			_T("Emissive"),
		};
		const MyMath::Vector4F initColorArray[] =
		{
			initVal.GetDiffuse(),
			initVal.GetAmbient(),
			initVal.GetSpecular(),
			initVal.GetEmissive(),
		};
		for (int i = 0; i < 4; ++i)
		{
			const LPCTSTR pMatElemName = ppNamesArray[i];
			{
				CString strName;
				strName.Format(_T("%s Color"), pMatElemName);
				CString strDesc;
				strDesc.Format(_T("%s color RGB value."), pMatElemName);
				auto& target = m_materialBasicColorProps.ppColors[i];
				target = new CMFCPropertyGridColorProperty(strName,
					MyMath::ConvertColor4FToRGBX(initColorArray[i]),
					nullptr,
					strDesc);

				m_pPropsGroupMaterialParams->AddSubItem(target);

				// MSDN の CMFCPropertyGridColorProperty::EnableOtherButton() の説明には、
				// 「標準のその他ボタンのラベルは [その他の色] です。」とあるが、
				// 別にローカライズ済みリソースが用意されているわけではないらしいので注意。
				// ローカライズはアプリケーション側で行なう必要がある。
				// また、実装を見る限り、lpszLabel に nullptr や 空文字列を渡すと、bEnable がいくら true でも表示されないらしい。
				// なお、CMFCColorButton::EnableOtherButton(), CMFCColorBar::EnableOtherButton() とは仕様が同じようだが、
				// CMFCRibbonColorButton のほうは常に CMFCColorDialog が使われるらしく、従来の CColorDialog は使えないらしい？
				target->EnableOtherButton(_T("その他"));
			}
			{
				CString strName;
				strName.Format(_T("%s Level"), pMatElemName);
				CString strDesc;
				strDesc.Format(_T("%s level value."), pMatElemName);
				auto& target = m_materialBasicLevelProps.ppLevels[i];
				target = new CMyDoubleSpinPGProperty(strName, initColorArray[i].w, 0, 1, delta, strDesc);

				m_pPropsGroupMaterialParams->AddSubItem(target);
			}
		}

		{
			m_pPropOpacityAlpha = new CMyDoubleSpinPGProperty(_T("Opacity"),
				initVal.GetOpacityAlpha(),
				0,
				1,
				delta,
				_T("Opacity (1 = opaque, 0 = transparent)"));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropOpacityAlpha);
			//m_pPropOpacityAlpha->SetManagerCtrl(&propsRoot);
		}

		{
			m_pPropSpecularPower = new CMyDoubleSpinPGProperty(_T("Power"),
				initVal.GetSpecularPower(),
				0,
				100,
				1,
				_T("Specular Power (Shininess)"));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropSpecularPower);
			//m_pPropSpecularPower->SetManagerCtrl(&propsRoot);
		}

		{
			m_pPropRoughness = new CMyDoubleSpinPGProperty(_T("Roughness"),
				initVal.GetRoughness(),
				0,
				1,
				delta,
				_T("Roughness of surface."));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropRoughness);
			//m_pPropRoughness->SetManagerCtrl(&propsRoot);
		}

		{
			m_pPropSpecularType = new CMyIntDropDownListPGProperty(_T("Specular Type"),
				_variant_t(static_cast<int>(initVal.GetReflexType())),
				_T("Specular type."));
			m_pPropSpecularType->SetDropDownValues(
			{
				CMyIntDropDownListPGProperty::TDropDownValue(_T("None"), MyMath::MySpecularType_None),
				CMyIntDropDownListPGProperty::TDropDownValue(_T("BlinnPhong"), MyMath::MySpecularType_BlinnPhong),
				CMyIntDropDownListPGProperty::TDropDownValue(_T("CookTorrance"), MyMath::MySpecularType_CookTorrance),
			});

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropSpecularType);
		}

		{
			m_pPropReflectivity = new CMyDoubleSpinPGProperty(_T("Reflectivity"),
				initVal.GetReflectivity(),
				0,
				1,
				delta,
				_T("Reflectivity for reflection mapping."));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropReflectivity);
			//m_pPropReflectivity->SetManagerCtrl(&propsRoot);
		}

		{
			// ダイヤモンドの屈折率は 2.4 程度。
			// 屈折率が負になる物理現象も発生しうるらしい。
			m_pPropIndexOfRefraction = new CMyDoubleSpinPGProperty(_T("IOR"),
				initVal.GetIndexOfRefraction(),
				1,
				3,
				delta,
				_T("Index of refraction for refraction mapping."));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropIndexOfRefraction);
			//m_pPropIndexOfRefraction->SetManagerCtrl(&propsRoot);
		}

		// _variant_t のコンストラクタに引数を渡すときは型に注意すること。特にリテラルは注意。
		{
			m_pPropReflexType = new CMyIntDropDownListPGProperty(_T("Reflex Type"),
				_variant_t(static_cast<int>(initVal.GetReflexType())),
				_T("Reflex type for environment mapping."));
			m_pPropReflexType->SetDropDownValues(
			{
				CMyIntDropDownListPGProperty::TDropDownValue(_T("None"), MyMath::MyReflexType_None),
				CMyIntDropDownListPGProperty::TDropDownValue(_T("ReflectOnly"), MyMath::MyReflexType_ReflectOnly),
				CMyIntDropDownListPGProperty::TDropDownValue(_T("RefractOnly"), MyMath::MyReflexType_RefractOnly),
				CMyIntDropDownListPGProperty::TDropDownValue(_T("Fresnel"), MyMath::MyReflexType_Fresnel),
			});

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropReflexType);
		}

		{
			m_pPropSproutsFur = new CMFCPropertyGridProperty(_T("Sprouts Fur"),
				_variant_t(initVal.GetSproutsFur()),
				_T("Sprouts fur or not."));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropSproutsFur);
		}

		{
			m_pPropToonGradient = new CMyGradientStopsProperty(_T("Toon Gradient"), L"", _T("Gradient stops for toon shading.\n"));

			m_pPropsGroupMaterialParams->AddSubItem(m_pPropToonGradient);
		}

		return m_pPropsGroupMaterialParams;
	}

	void MyCPGPMaterial::SetProperties(const MyMath::MyMaterial& inVal)
	{
		_ASSERTE(m_pPropsGroupMaterialParams != nullptr);

		m_pPropMaterialName->SetValue(inVal.GetMaterialName().c_str());

		m_pPropTexFileNameDiffuseMap->SetValue(inVal.GetTexFileNameDiffuseMap().c_str());
		m_pPropTexFileNameNormalMap->SetValue(inVal.GetTexFileNameNormalMap().c_str());

		m_materialBasicColorProps.pDiffuseColor->SetColor(MyMath::ConvertColor4FToRGBX(inVal.GetDiffuse()));
		m_materialBasicColorProps.pAmbientColor->SetColor(MyMath::ConvertColor4FToRGBX(inVal.GetAmbient()));
		m_materialBasicColorProps.pSpecularColor->SetColor(MyMath::ConvertColor4FToRGBX(inVal.GetSpecular()));
		m_materialBasicColorProps.pEmissiveColor->SetColor(MyMath::ConvertColor4FToRGBX(inVal.GetEmissive()));

		m_materialBasicLevelProps.pDiffuseLevel->SetCurrentValue(inVal.GetDiffuse().w);
		m_materialBasicLevelProps.pAmbientLevel->SetCurrentValue(inVal.GetAmbient().w);
		m_materialBasicLevelProps.pSpecularLevel->SetCurrentValue(inVal.GetSpecular().w);
		m_materialBasicLevelProps.pEmissiveLevel->SetCurrentValue(inVal.GetEmissive().w);

		m_pPropOpacityAlpha->SetCurrentValue(inVal.GetOpacityAlpha());
		m_pPropSpecularPower->SetCurrentValue(inVal.GetSpecularPower());
		m_pPropRoughness->SetCurrentValue(inVal.GetRoughness());
		m_pPropSpecularType->SetCurrentValue(static_cast<int>(inVal.GetSpecularType()));
		m_pPropReflectivity->SetCurrentValue(inVal.GetReflectivity());
		m_pPropIndexOfRefraction->SetCurrentValue(inVal.GetIndexOfRefraction());
		m_pPropReflexType->SetCurrentValue(static_cast<int>(inVal.GetReflexType()));

		m_pPropToonGradient->SetGradientColorStops(inVal.GetToonGradientColorStops());

		m_pPropSproutsFur->SetValue(_variant_t(inVal.GetSproutsFur()));
	}

	void MyCPGPMaterial::GetProperties(_Inout_ MyMath::MyMaterial& outVal) const
	{
		_ASSERTE(m_pPropsGroupMaterialParams != nullptr);

		// VARIANT から値を取得するときは型に注意すること。
		_ASSERTE(m_pPropMaterialName->GetValue().vt == VT_BSTR);
		outVal.SetMaterialName(m_pPropMaterialName->GetValue().bstrVal);
		_ASSERTE(m_pPropTexFileNameDiffuseMap->GetValue().vt == VT_BSTR);
		outVal.SetTexFileNameDiffuseMap(m_pPropTexFileNameDiffuseMap->GetValue().bstrVal);
		_ASSERTE(m_pPropTexFileNameNormalMap->GetValue().vt == VT_BSTR);
		outVal.SetTexFileNameNormalMap(m_pPropTexFileNameNormalMap->GetValue().bstrVal);

		outVal.SetDiffuseColorRGB(MyMath::ConvertRGBXToColor3F(m_materialBasicColorProps.pDiffuseColor->GetColor()));
		outVal.SetAmbientColorRGB(MyMath::ConvertRGBXToColor3F(m_materialBasicColorProps.pAmbientColor->GetColor()));
		outVal.SetSpecularColorRGB(MyMath::ConvertRGBXToColor3F(m_materialBasicColorProps.pSpecularColor->GetColor()));
		outVal.SetEmissiveColorRGB(MyMath::ConvertRGBXToColor3F(m_materialBasicColorProps.pEmissiveColor->GetColor()));

		outVal.SetDiffuseLevel(float(m_materialBasicLevelProps.pDiffuseLevel->GetCurrentValue()));
		outVal.SetAmbientLevel(float(m_materialBasicLevelProps.pAmbientLevel->GetCurrentValue()));
		outVal.SetSpecularLevel(float(m_materialBasicLevelProps.pSpecularLevel->GetCurrentValue()));
		outVal.SetEmissiveLevel(float(m_materialBasicLevelProps.pEmissiveLevel->GetCurrentValue()));

		outVal.SetOpacityAlpha(float(m_pPropOpacityAlpha->GetCurrentValue()));
		outVal.SetSpecularPower(float(m_pPropSpecularPower->GetCurrentValue()));
		outVal.SetRoughness(float(m_pPropRoughness->GetCurrentValue()));
		outVal.SetSpecularType(static_cast<MyMath::MySpecularType>(m_pPropSpecularType->GetCurrentValue()));
		outVal.SetReflectivity(float(m_pPropReflectivity->GetCurrentValue()));
		outVal.SetIndexOfRefraction(float(m_pPropIndexOfRefraction->GetCurrentValue()));
		outVal.SetReflexType(static_cast<MyMath::MyReflexType>(m_pPropReflexType->GetCurrentValue()));

		m_pPropToonGradient->GetGradientColorStops(outVal.GetToonGradientColorStops());

		_ASSERTE(m_pPropSproutsFur->GetValue().vt == VT_BOOL);
		outVal.SetSproutsFur(!!m_pPropSproutsFur->GetValue().boolVal);
	}

	void MyCPGPMaterial::Show(bool shows)
	{
		_ASSERTE(m_pPropsGroupMaterialParams != nullptr);

		m_pPropsGroupMaterialParams->Show(shows);
	}

	bool MyCPGPMaterial::IsVisible() const
	{
		_ASSERTE(m_pPropsGroupMaterialParams != nullptr);

		return !!m_pPropsGroupMaterialParams->IsVisible();
	}

} // end of namespace
