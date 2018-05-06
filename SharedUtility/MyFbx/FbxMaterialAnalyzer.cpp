#include "stdafx.h"

#include "FbxMaterialAnalyzer.h"


namespace
{
	void ExtractTextureName(const FbxSurfaceMaterial* material, const char* pMatFlagName,
		std::wstring& outTexName, std::wstring& outTexFileName, std::wstring& outTexRelativeFileName)
	{
		const auto prop(material->FindProperty(pMatFlagName));
		//const int layerNum = prop.GetSrcObjectCount(KFbxLayeredTexture::ClassId);
		const int layerNum = prop.GetSrcObjectCount<FbxLayeredTexture>();
		ATLTRACE(__FUNCTION__ "(), Count of FbxLayeredTexture: %d\n", layerNum);
		if (layerNum == 0) // マルチ レイヤーは非対応。
		{
			//const int generalTextureCount = prop.GetSrcObjectCount(KFbxTexture::ClassId);
			const int generalTextureCount = prop.GetSrcObjectCount<FbxTexture>();
			ATLTRACE(__FUNCTION__ "(), Count of FbxTexture: %d\n", generalTextureCount);
			for (int i = 0; i < generalTextureCount; ++i)
			{
				//const KFbxTexture* texture = KFbxCast<KFbxTexture>(prop.GetSrcObject(KFbxTexture::ClassId, i));
				const auto* texture = prop.GetSrcObject<const FbxTexture>(i);
				//outTexName = texture->GetFileName(); // 昔の FBX SDK ではこの GetFileName() メソッドでファイル名を取得できていた？
				outTexName = MyUtils::SafeConvertUtf8toUtf16(texture->GetName());
				// GetName() で抽出するのは名前（FBX オブジェクト名）であってファイル名ではない。
				// Metasequoia 用の FBX Exporter プラグインを使って出力したとき、"マテリアル名" + "_tex" という名前になっていた。
				// http://horsetail.sakura.ne.jp/mqplugin/mqfbxe.html
				// ファイル名は FbxLayerElementTexture 経由で取得する？　FbxFileTexture 経由で取得する？
				// 昔の FBX SDK では KFbxLayerElementTexture 経由で取得できていたらしい？
				break; // HACK: とりあえず現時点では1マテリアルにつきテクスチャ1枚だけサポート。
			}

			const int generalFileTextureCount = prop.GetSrcObjectCount<FbxFileTexture>();
			ATLTRACE(__FUNCTION__ "(), Count of FbxFileTexture: %d\n", generalFileTextureCount);
			for (int i = 0; i < generalFileTextureCount; ++i)
			{
				const auto* fileTexture = prop.GetSrcObject<const FbxFileTexture>(i);
				outTexFileName = MyUtils::SafeConvertUtf8toUtf16(fileTexture->GetFileName());
				outTexRelativeFileName = MyUtils::SafeConvertUtf8toUtf16(fileTexture->GetRelativeFileName());
				// FBX の RelativeFileName は、FBX ファイルからテクスチャ画像ファイルへの相対パスでなく、
				// FBX ファイルからその FBX を作成したプロセス（EXE）への相対ディレクトリ パス＋ファイル名が記録されることがあるなど、
				// 挙動不審というか可搬性が全くない。
				// ただしこの挙動は Metasequoia 用の FBX Exporter プラグインを使って出力したときの結果なので、
				// 最新の FBX SDK では変更されているかもしれない。
				// FbxFileTexture::GetRelativeFileName() はテクスチャ ファイルへの相対パスが取得できるとコメントに書いてあるが、
				// FBX ファイルに記述されているパス文字列と異なる文字列が返ったり、フルパスが返ったりする場合もある模様。
				// FbxFileTexture::GetFileName() はテクスチャ ファイルへの絶対パスが取得できるとコメントに書いてあるが、
				// ただの "<FileBody>.<Extension>" だけになる場合もある模様。
				// あまり信用しないほうがいい。
				break; // HACK: とりあえず現時点では1マテリアルにつきテクスチャ1枚だけサポート。
			}
		}
	}
}

namespace MyFbx
{

	MyFbxMaterialAnalyzer::MyFbxMaterialAnalyzer()
		: m_fbxRgbAmbient(0, 0, 0)
		, m_fbxRgbDiffuse(0, 0, 0)
		, m_fbxRgbSpecular(0, 0, 0)
		, m_fbxRgbEmissive(0, 0, 0)
		, m_fbxRgbBump(0, 0, 0)
		, m_fbxRgbNormalMap(0, 0, 0)
		, m_fbxRgbTransparentColor(0, 0, 0)
		, m_fbxRgbReflection(0, 0, 0)
		, m_fbxAmbientFactor()
		, m_fbxDiffuseFactor()
		, m_fbxSpecularFactor()
		, m_fbxEmissiveFactor()
		, m_fbxBumpFactor()
		, m_fbxTransparencyFactor()
		, m_fbxReflectionFactor()
		, m_fbxShininess()
#pragma region // Custom Properties //
		, m_fbxRoughness()
		, m_fbxReflectivity()
		, m_fbxIndexOfRefraction(1)
		, m_fbxTranslucency()
		, m_fbxOpacity(1)
#pragma endregion
	{
	}

	MyFbxMaterialAnalyzer::~MyFbxMaterialAnalyzer()
	{
	}

	void MyFbxMaterialAnalyzer::Analyze(const FbxSurfaceMaterial* material)
	{
		this->SetMaterialName(material->GetName());

		// Lambert か Phong かチェック。
		if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			// Lambert.
			const auto* lambert = StaticDownCast<const FbxSurfaceLambert>(material);
			this->ExtractLambertInfo(lambert);
			// 昔の FBX SDK では dynamic_cast が使えていたが、
			// FBX SDK 2013.3 以降の DLL は RTTI を OFF にしてビルドされているらしく、
			// dynamic_cast が使えない。
		}
		// Phong は Lambert の派生クラス。つまり FbxSurfacePhong は FbxSurfaceLambert でもある。
		// ClassId で照合する場合は注意。
		if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			// Phong.
			const auto* phong = StaticDownCast<const FbxSurfacePhong>(material);

			// スペキュラー。
			//MyFbx::ToVector3F(m_fbxRgbSpecular, phong->GetSpecularColor());
			MyFbx::ToVector3F(m_fbxRgbSpecular, phong->Specular);
			m_fbxSpecularFactor = float(phong->SpecularFactor);
			//ATLTRACE(_T("SpecularFactor = %f\n"), MyFbx::ToFloat(phong->SpecularFactor));
			ExtractTextureName(phong, FbxSurfaceMaterial::sSpecular,
				m_texNameSpecular, m_texFileNameSpecular, m_texRelativeFileNameSpecular);

			// 光沢。
			//m_fbxShininess = static_cast<float>(phong->GetShininess().Get());
			//m_fbxShininess = static_cast<float>(phong->Shininess.Get());
			m_fbxShininess = MyFbx::ToFloat(phong->Shininess);
			// いわゆるグロス マップというのは Shininess ではなく Specular マップとして割り当てられるものなのか？
			// Shininess をピクセル単位で制御する場合は Shininess のテクスチャになるはず。

			// 反射。
			MyFbx::ToVector3F(m_fbxRgbReflection, phong->Reflection);
			//m_fbxReflectionFactor = static_cast<float>(phong->GetReflectionFactor().Get());
			//m_fbxReflectionFactor = static_cast<float>(phong->ReflectionFactor.Get());
			m_fbxReflectionFactor = MyFbx::ToFloat(phong->ReflectionFactor);
			ExtractTextureName(phong, FbxSurfaceMaterial::sReflection,
				m_texNameReflection, m_texFileNameReflection, m_texRelativeFileNameReflection);

			const auto& findCustomScalarProperty = [](const FbxObject* target, const char* propName, float& outVal)
			{
				// 大文字・小文字の区別はしない。
				auto myFbxProp = target->FindProperty(propName, false);
				if (myFbxProp.IsValid() && myFbxProp.GetPropertyDataType().GetType() == eFbxDouble)
				{
					outVal = float(myFbxProp.Get<double>());
					ATLTRACE("FBX Custom Property: [%s] Found.\n", propName);
				}
			};

			findCustomScalarProperty(phong, "Roughness", m_fbxRoughness);
			findCustomScalarProperty(phong, "Reflectivity", m_fbxReflectivity);
			findCustomScalarProperty(phong, "IndexOfRefraction", m_fbxIndexOfRefraction);
			findCustomScalarProperty(phong, "Translucency", m_fbxTranslucency);
			findCustomScalarProperty(phong, "Opacity", m_fbxOpacity);
		}
	}

	void MyFbxMaterialAnalyzer::ExtractLambertInfo(const FbxSurfaceLambert* lambert)
	{
		// 本来、FBX の仕様としては下記が正しいはず。
		// DiffuseColor = Diffuse * DiffuseFactor;
		// AmbientColor = Ambient * AmbientFactor;
		// SpecularColor = Specular * SpecularFactor;
		// EmissiveColor = Emissive * EmissiveFactor;
		// DiffuseShadingResult = DiffuseColor * dot(Normal, Light);
		// SpecularShadingResult = SpecularColor * pow(dot(Normal, HalfWay), Shininess); // HalfWay = Light + Eye
		// http://forums.autodesk.com/t5/FBX-SDK/EmissiveFactor-AmbientFactor-DiffuseFactor/td-p/4230572
		// 
		// 本来、Metasequoia の MqoMaterialColor（Float3 ベクトル）に相当するのは FBX Diffuse（Double3 ベクトル）で、
		// MqoDiffuseLevel（Float スカラー、0～1）に相当するのは FBX DiffuseFactor（Double スカラー）だが、
		// Metasequoia 用の FBX エクスポーター Ver.1.2.8 以前は MqoMaterialColor * MqoDiffuseLevel を FBX Diffuse として出力してしまっているらしく、
		// 情報が完全に失われている（不可逆変換）。本来は乗算せず、分けて出力するべきだが、これは 3ds Max の動作・仕様に合わせているらしい。
		// また、MqoAmbientLevel（Float スカラー、0～1）に相当するのは FBX Ambient（Double3 ベクトル）でなく FBX AmbientFactor（Double スカラー）、
		// MqoSpecularLevel（Float スカラー、0～1）に相当するのは FBX Specular（Double3 ベクトル）でなく FBX SpecularFactor（Double スカラー）、
		// MqoEmissiveLevel（Float スカラー、0～1）に相当するのは FBX Emissive（Double3 ベクトル）でなく FBX EmissiveFactor（Double スカラー）、
		// MqoMaterialOpacity（Float スカラー、0～1）に相当するものは FBX Opacity（Double スカラー、1 - Transparency）、のはず。
		// エクスポーター Ver.1.2.8 で Transparency が常に 1 だったのは、Maya の動作・仕様に合わせているらしい。
		// 
		// なお、MqoSpecularPower（Float スカラー、0～100）に相当するものは FBX Shininess（Double スカラー）となっている。
		// これはちゃんと出力されている模様。
		// エクスポーター Ver.1.2.10 以降は ColorRGB と Factor を分離して出力できるオプションが付加されている。
		// また、Ambient の ColorRGB には、MQO ドキュメントの環境光 RGB が格納される。

		// アンビエント。
		//MyFbx::ToVector3F(m_fbxRgbAmbient, lambert->GetAmbientColor());
		MyFbx::ToVector3F(m_fbxRgbAmbient, lambert->Ambient);
		m_fbxAmbientFactor = float(lambert->AmbientFactor);
		//ATLTRACE(_T("AmbientFactor = %f\n"), MyFbx::ToFloat(lambert->AmbientFactor));
		ExtractTextureName(lambert, FbxSurfaceMaterial::sAmbient,
			m_texNameAmbient, m_texFileNameAmbient, m_texRelativeFileNameAmbient);

		// ディフューズ。
		//MyFbx::ToVector3F(m_fbxRgbDiffuse, lambert->GetDiffuseColor());
		MyFbx::ToVector3F(m_fbxRgbDiffuse, lambert->Diffuse);
		m_fbxDiffuseFactor = float(lambert->DiffuseFactor);
		//ATLTRACE(_T("DiffuseFactor = %f\n"), MyFbx::ToFloat(lambert->DiffuseFactor));
		ExtractTextureName(lambert, FbxSurfaceMaterial::sDiffuse,
			m_texNameDiffuse, m_texFileNameDiffuse, m_texRelativeFileNameDiffuse);

		// エミッシブ。
		//MyFbx::ToVector3F(m_fbxRgbEmissive, lambert->GetEmissiveColor());
		MyFbx::ToVector3F(m_fbxRgbEmissive, lambert->Emissive);
		m_fbxEmissiveFactor = float(lambert->EmissiveFactor);
		//ATLTRACE(_T("EmissiveFactor = %f\n"), MyFbx::ToFloat(lambert->EmissiveFactor));
		ExtractTextureName(lambert, FbxSurfaceMaterial::sEmissive,
			m_texNameEmissive, m_texFileNameEmissive, m_texRelativeFileNameEmissive);

		// バンプ。
		//MyFbx::ToVector3F(m_fbxRgbBump, lambert->GetBump());
		MyFbx::ToVector3F(m_fbxRgbBump, lambert->Bump);
		m_fbxBumpFactor = float(lambert->BumpFactor);
		ExtractTextureName(lambert, FbxSurfaceMaterial::sBump,
			m_texNameBump, m_texFileNameBump, m_texRelativeFileNameBump);

		// 法線マップ。
		MyFbx::ToVector3F(m_fbxRgbNormalMap, lambert->NormalMap);
		ExtractTextureName(lambert, FbxSurfaceMaterial::sNormalMap,
			m_texNameNormalMap, m_texFileNameNormalMap, m_texRelativeFileNameNormalMap);

		// 透過度。
		MyFbx::ToVector3F(lambert->TransparentColor);
		//m_fbxTransparencyFactor = static_cast<float>(lambert->GetTransparencyFactor().Get());
		//m_fbxTransparencyFactor = static_cast<float>(lambert->TransparencyFactor.Get());
		m_fbxTransparencyFactor = MyFbx::ToFloat(lambert->TransparencyFactor);
		// HACK: アルファマップはどっち？　Color のほうを調べるだけでよい？　Factor は調べる必要がない？
		// Metaseq FBX Exporter で調べてみる？
		ExtractTextureName(lambert, FbxSurfaceMaterial::sTransparentColor,
			m_texNameTransparentColor, m_texFileNameTransparentColor, m_texRelativeFileNameTransparentColor);
		ExtractTextureName(lambert, FbxSurfaceMaterial::sTransparencyFactor,
			m_texNameTransparencyFactor, m_texFileNameTransparencyFactor, m_texRelativeFileNameTransparencyFactor);
	}

	const MyMath::Vector3F& MyFbxMaterialAnalyzer::GetFbxRgbColorRGB(PropertyType propertyType) const
	{
		switch (propertyType)
		{
		case PropertyType_Ambient:
			return m_fbxRgbAmbient;
		case PropertyType_Diffuse:
			return m_fbxRgbDiffuse;
		case PropertyType_Specular:
			return m_fbxRgbSpecular;
		case PropertyType_Emissive:
			return m_fbxRgbEmissive;
		case PropertyType_Bump:
			return m_fbxRgbBump;
		case PropertyType_NormalMap:
			return m_fbxRgbNormalMap;
		case PropertyType_TransparentColor:
			return m_fbxRgbTransparentColor;
		case PropertyType_Reflection:
			return m_fbxRgbReflection;
		default:
			return MyMath::ZERO_VECTOR3F;
		}
	}

	const float MyFbxMaterialAnalyzer::GetFbxFactor(PropertyType propertyType) const
	{
		switch (propertyType)
		{
		case PropertyType_Ambient:
			return m_fbxAmbientFactor;
		case PropertyType_Diffuse:
			return m_fbxDiffuseFactor;
		case PropertyType_Specular:
			return m_fbxSpecularFactor;
		case PropertyType_Emissive:
			return m_fbxEmissiveFactor;
		case PropertyType_Bump:
		case PropertyType_NormalMap:
			return m_fbxBumpFactor; // どの程度凹凸を適用するかの度合なので、Bump と Normal の Factor は共有らしい。
		case PropertyType_TransparencyFactor:
			return m_fbxTransparencyFactor;
		case PropertyType_Reflection:
			return m_fbxReflectionFactor;
		default:
			return 0;
		}
	}

	const std::wstring& MyFbxMaterialAnalyzer::GetTextureName(PropertyType propertyType) const
	{
		switch (propertyType)
		{
		case PropertyType_Ambient:
			return m_texNameAmbient;
		case PropertyType_Diffuse:
			return m_texNameDiffuse;
		case PropertyType_Specular:
			return m_texNameSpecular;
		case PropertyType_Emissive:
			return m_texNameEmissive;
		case PropertyType_Bump:
			return m_texNameBump;
		case PropertyType_NormalMap:
			return m_texNameNormalMap;
		case PropertyType_TransparentColor:
			return m_texNameTransparentColor;
		case PropertyType_TransparencyFactor:
			return m_texNameTransparencyFactor;
		case PropertyType_Reflection:
			return m_texNameReflection;
		default:
			return MyUtils::EmptyStdStringW;
		}
	}

	const std::wstring& MyFbxMaterialAnalyzer::GetTextureFileName(PropertyType propertyType) const
	{
		switch (propertyType)
		{
		case PropertyType_Ambient:
			return m_texFileNameAmbient;
		case PropertyType_Diffuse:
			return m_texFileNameDiffuse;
		case PropertyType_Specular:
			return m_texFileNameSpecular;
		case PropertyType_Emissive:
			return m_texFileNameEmissive;
		case PropertyType_Bump:
			return m_texFileNameBump;
		case PropertyType_NormalMap:
			return m_texFileNameNormalMap;
		case PropertyType_TransparentColor:
			return m_texFileNameTransparentColor;
		case PropertyType_TransparencyFactor:
			return m_texFileNameTransparencyFactor;
		case PropertyType_Reflection:
			return m_texFileNameReflection;
		default:
			return MyUtils::EmptyStdStringW;
		}
	}

	const std::wstring& MyFbxMaterialAnalyzer::GetTextureRelativeFileName(PropertyType propertyType) const
	{
		switch (propertyType)
		{
		case PropertyType_Ambient:
			return m_texRelativeFileNameAmbient;
		case PropertyType_Diffuse:
			return m_texRelativeFileNameDiffuse;
		case PropertyType_Specular:
			return m_texRelativeFileNameSpecular;
		case PropertyType_Emissive:
			return m_texRelativeFileNameEmissive;
		case PropertyType_Bump:
			return m_texRelativeFileNameBump;
		case PropertyType_NormalMap:
			return m_texRelativeFileNameNormalMap;
		case PropertyType_TransparentColor:
			return m_texRelativeFileNameTransparentColor;
		case PropertyType_TransparencyFactor:
			return m_texRelativeFileNameTransparencyFactor;
		case PropertyType_Reflection:
			return m_texRelativeFileNameReflection;
		default:
			return MyUtils::EmptyStdStringW;
		}
	}
}
