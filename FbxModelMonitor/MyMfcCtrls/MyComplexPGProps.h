#pragma once


#include "CustomPGProperty.h"
#include "MyMath.hpp"


namespace MyMath
{
	class Vector2D
	{
	public:
		double X, Y;
	public:
		Vector2D()
			: X(), Y()
		{}
		Vector2D(double x, double y)
			: X(x), Y(y)
		{}
	};

	class Vector3D
	{
	public:
		double X, Y, Z;
	public:
		Vector3D()
			: X(), Y(), Z()
		{}
		Vector3D(double x, double y, double z)
			: X(x), Y(y), Z(z)
		{}
	};

	class Vector4D
	{
	public:
		double X, Y, Z, W;
	public:
		Vector4D()
			: X(), Y(), Z(), W()
		{}
		Vector4D(double x, double y, double z, double w)
			: X(x), Y(y), Z(z), W(w)
		{}
	};
}


namespace MyComplexPGProps
{
	// マテリアル色は CMFCPropertyGridColorProperty を使って RGB 各チャンネル 0~255 で設定すれば十分。
	// スペキュラー強度や透過率はスカラーの倍精度浮動小数点数用に作った CMyDoubleSpinPGProperty を使う。

	union MyDouble4SpinPGProps
	{
		struct
		{
			CMyDoubleSpinPGProperty* pX;
			CMyDoubleSpinPGProperty* pY;
			CMyDoubleSpinPGProperty* pZ;
			CMyDoubleSpinPGProperty* pW;
		};
		CMyDoubleSpinPGProperty* ppVector[4];
	};

	union MyMaterialBasicColorPGProps
	{
		struct
		{
			CMFCPropertyGridColorProperty* pDiffuseColor;
			CMFCPropertyGridColorProperty* pAmbientColor;
			CMFCPropertyGridColorProperty* pSpecularColor;
			CMFCPropertyGridColorProperty* pEmissiveColor;
		};
		CMFCPropertyGridColorProperty* ppColors[4];
	};

	union MyMaterialBasicLevelPGProps
	{
		struct
		{
			CMyDoubleSpinPGProperty* pDiffuseLevel;
			CMyDoubleSpinPGProperty* pAmbientLevel;
			CMyDoubleSpinPGProperty* pSpecularLevel;
			CMyDoubleSpinPGProperty* pEmissiveLevel;
		};
		CMyDoubleSpinPGProperty* ppLevels[4];
	};


	//! @brief  CMFCPropertyGridProperty あるいはその派生クラスへのポインタの集合。<br>
	class MyCPGPVector4D sealed
	{
		CMFCPropertyGridProperty* m_pPropsGroupVectorParams;

		MyDouble4SpinPGProps m_propsVector4D;
	public:
		MyCPGPVector4D()
			: m_pPropsGroupVectorParams()
			, m_propsVector4D()
		{
		}

		CMFCPropertyGridProperty* Create(const CString& strGroupName,
			const MyMath::Vector4D& initVal, const MyMath::Vector4D& minVal, const MyMath::Vector4D& maxVal, double delta);

		void SetProperties(const MyMath::Vector4D& inVal);
		void GetProperties(MyMath::Vector4D& outVal);

		void Show(bool shows = true);

		bool IsVisible() const;
	};


	class MyCPGPMaterial sealed
	{
		CMFCPropertyGridProperty* m_pPropsGroupMaterialParams;

		CMFCPropertyGridProperty* m_pPropMaterialName;
		CMFCPropertyGridProperty* m_pPropTexFileNameDiffuseMap;
		CMFCPropertyGridProperty* m_pPropTexFileNameNormalMap;

		MyMaterialBasicColorPGProps m_materialBasicColorProps;
		MyMaterialBasicLevelPGProps m_materialBasicLevelProps;

		CMyDoubleSpinPGProperty* m_pPropOpacityAlpha;
		CMyDoubleSpinPGProperty* m_pPropSpecularPower;
		CMyDoubleSpinPGProperty* m_pPropRoughness;
		CMyIntDropDownListPGProperty* m_pPropSpecularType;
		CMyDoubleSpinPGProperty* m_pPropReflectivity;
		CMyDoubleSpinPGProperty* m_pPropIndexOfRefraction;
		CMyIntDropDownListPGProperty* m_pPropReflexType;
		CMFCPropertyGridProperty* m_pPropSproutsFur;

		CMyGradientStopsProperty* m_pPropToonGradient;
	public:
		MyCPGPMaterial()
			: m_pPropsGroupMaterialParams()
			, m_pPropMaterialName()
			, m_pPropTexFileNameDiffuseMap()
			, m_pPropTexFileNameNormalMap()
			, m_materialBasicColorProps()
			, m_materialBasicLevelProps()
			, m_pPropOpacityAlpha()
			, m_pPropSpecularPower()
			, m_pPropRoughness()
			, m_pPropSpecularType()
			, m_pPropReflectivity()
			, m_pPropIndexOfRefraction()
			, m_pPropReflexType()
			, m_pPropSproutsFur()
			, m_pPropToonGradient()
		{
		}

		CMFCPropertyGridProperty* Create(const MyMath::MyMaterial& initVal);

		void Show(bool shows = true);

		void SetProperties(const MyMath::MyMaterial& inVal);
		void GetProperties(_Inout_ MyMath::MyMaterial& outVal) const;

		bool IsVisible() const;
	};

} // end of namespace
