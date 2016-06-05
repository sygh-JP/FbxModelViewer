#pragma once

#include "MyFbx.h"


namespace MyFbx
{
	class MyFbxMaterialAnalyzer final
	{
	public:
		typedef std::shared_ptr<MyFbxMaterialAnalyzer> TSharedPtr;
		typedef std::shared_ptr<const MyFbxMaterialAnalyzer> TConstSharedPtr;
	public:
		enum PropertyType
		{
			PropertyType_Ambient,
			PropertyType_Diffuse,
			PropertyType_Specular,
			PropertyType_Emissive,
			PropertyType_Bump,
			PropertyType_NormalMap,
			PropertyType_TransparentColor,
			PropertyType_TransparencyFactor,
			PropertyType_Reflection,
		};

	public:
		// コンストラクタ
		MyFbxMaterialAnalyzer();

		// デストラクタ
		virtual ~MyFbxMaterialAnalyzer();

		// 解析
		void Analyze(const FbxSurfaceMaterial* material);

		const MyMath::Vector3F& GetFbxRgbAmbient() const { return m_fbxRgbAmbient; }
		const MyMath::Vector3F& GetFbxRgbDiffuse() const { return m_fbxRgbDiffuse; }
		const MyMath::Vector3F& GetFbxRgbSpecular() const { return m_fbxRgbSpecular; }
		const MyMath::Vector3F& GetFbxRgbEmissive() const { return m_fbxRgbEmissive; }
		const MyMath::Vector3F& GetFbxRgbBump() const { return m_fbxRgbBump; }
		const MyMath::Vector3F& GetFbxRgbNormalMap() const { return m_fbxRgbNormalMap; }
		const MyMath::Vector3F& GetFbxRgbTransparentColor() const { return m_fbxRgbTransparentColor; }
		const MyMath::Vector3F& GetFbxRgbReflection() const { return m_fbxRgbReflection; }

		float GetFbxAmbientFactor() const { return m_fbxAmbientFactor; }
		float GetFbxDiffuseFactor() const { return m_fbxDiffuseFactor; }
		float GetFbxSpecularFactor() const { return m_fbxSpecularFactor; }
		float GetFbxEmissiveFactor() const { return m_fbxEmissiveFactor; }
		float GetFbxBumpFactor() const { return m_fbxBumpFactor; }
		float GetFbxNormalFactor() const { return m_fbxBumpFactor; }

		float GetFbxTransparencyFactor() const { return m_fbxTransparencyFactor; }
		float GetFbxReflectionFactor() const { return m_fbxReflectionFactor; }
		float GetFbxShininess() const { return m_fbxShininess; }

#pragma region // Custom Properties //
		float GetFbxRoughness() const { return m_fbxRoughness; }
		float GetFbxReflectivity() const { return m_fbxReflectivity; }
		float GetFbxIndexOfRefraction() const { return m_fbxIndexOfRefraction; }
		float GetFbxTranslucency() const { return m_fbxTranslucency; }
		float GetFbxOpacity() const { return m_fbxOpacity; }
#pragma endregion

		const MyMath::Vector3F& GetFbxRgbColorRGB(PropertyType propertyType) const;

		const float GetFbxFactor(PropertyType propertyType) const;

		//! @brief  テクスチャ名（FBX オブジェクト名）を取得する。<br>
		const std::wstring& GetTextureName(PropertyType propertyType) const;

		//! @brief  テクスチャのファイル名を取得する。<br>
		const std::wstring& GetTextureFileName(PropertyType propertyType) const;

		//! @brief  テクスチャの相対ファイル名を取得する。<br>
		const std::wstring& GetTextureRelativeFileName(PropertyType propertyType) const;

		void SetMaterialName(const char* pName) { m_strMaterialName = pName ? MyUtil::ConvertUtf8toUtf16(pName) : L""; }
		void SetMaterialName(const wchar_t* pName) { m_strMaterialName = pName ? pName : L""; }

		const std::wstring& GetMaterialNameW() const { return m_strMaterialName; }

	private:
		// Lambert 情報の抽出。
		void ExtractLambertInfo(const FbxSurfaceLambert* lambert);

		// Vector3 の X, Y, Z が R, G, B にそれぞれ相当する。
		// FBX ではディフューズ カラーとディフューズ強度（拡散光レベル）が分離されている。
		// Max フォーマット、および DirectX の X フォーマットではディフューズ カラーとディフューズ強度が合成されている。
		// なお、Metasequoia や LightWave ではマテリアル カラーとディフューズ強度が分離されている。
	private:
		MyMath::Vector3F m_fbxRgbAmbient; // アンビエント カラー
		MyMath::Vector3F m_fbxRgbDiffuse; // ディフューズ カラー
		MyMath::Vector3F m_fbxRgbSpecular; // スペキュラー カラー
		MyMath::Vector3F m_fbxRgbEmissive; // エミッシブ カラー
		MyMath::Vector3F m_fbxRgbBump; // バンプ カラー
		MyMath::Vector3F m_fbxRgbNormalMap; // 法線マップ カラー
		MyMath::Vector3F m_fbxRgbTransparentColor; //!< 透過カラー。<br>
		MyMath::Vector3F m_fbxRgbReflection;

		// 詳しいことはよく分からないが、Maya では透明度はスカラーだけでなくカラーアトリビュート設定も可能らしい。
		// Shade でも透過色を指定できるらしい。色付きのガラスや水の表現にそのまま使える。
		// LightWave 標準マテリアルや Metasequoia では透過色はサポートされない。

		float m_fbxAmbientFactor;
		float m_fbxDiffuseFactor;
		float m_fbxSpecularFactor;
		float m_fbxEmissiveFactor;
		float m_fbxBumpFactor;

		float m_fbxTransparencyFactor; //!< 透過度。Trasparency = (1 - Opacity) = (1 - Alpha) であることに注意。<br>
		float m_fbxReflectionFactor; //!< Reflection と Reflectivity との違いがよく分からない。Reflectivity はブレンド比率だと思うが、Factor は？<br>
		float m_fbxShininess; //!< 光沢の強さ（Power）。<br>

#pragma region // Custom Properties //
		float m_fbxRoughness; //!< 面の粗さ。<br>
		float m_fbxReflectivity; //!< 反射率（Reflectivity）。<br>
		float m_fbxIndexOfRefraction;
		float m_fbxTranslucency;
		float m_fbxOpacity; //!< 不透明度。<br>
#pragma endregion

		// FBX に Refractivity や Index of Refraction（IOR = 屈折率）はないらしい。
		// SSS（Subsurface Scattering）で必要となる Translucency（半透明度）もないらしい。
		// 3ds Max には Translucency が存在する。
		// http://docs.autodesk.com/3DSMAX/16/JPN/3ds-Max-Help/index.html?url=files/GUID-B8011F77-73AC-4BB8-BA73-5094465A7152.htm,topicNumber=d30e477247
		// FBX マテリアルは標準のプロパティ（FbxSurfaceMaterial クラスに名前のリテラルが定義されているもの）に加えて、
		// 独自のカスタム プロパティを追加できるらしい。
		// Collada には IOR はあるが、Translucency はないらしい。
		// FBX SDK の <2014.2\include\fbxsdk\fileio\collada\fbxcolladatokens.h> に、
		// COLLADA_INDEXOFREFRACTION_MATERIAL_PARAMETER の定義がある。
		// LightWave には屈折インデックスおよび半透明度が存在する。
		// なお、シェーダーで「空気→ガラス→水→ガラス→空気」などの透過条件で屈折マッピングしようと思ったら
		// 屈折率の比が要るために隣接マテリアルの情報が要るはず。
		// 素直にレイトレースかフォトンマッピングするべき領域かもしれない。
		// Refractivity は IOR とは別物のようなので、そもそも不要かも。
		// 
		// 3ds Max では Roughness と Glossiness 両方を持っている。
		// Roughness が大きいほど光沢がなくなり、また Glossiness が大きいほどハイライトが小さくなるが、
		// 両者は対立する概念ではない。
		// http://docs.autodesk.com/3DSMAX/16/JPN/3ds-Max-Help/index.html?url=files/GUID-21BB5B24-C553-47A7-8472-8443936E13FB.htm,topicNumber=d30e477152
		// http://docs.autodesk.com/3DSMAX/16/JPN/3ds-Max-Help/index.html?url=files/GUID-7C791BBC-B27F-4BD2-821D-D7DBEA9E7CC9.htm,topicNumber=d30e477568
		// http://docs.autodesk.com/3DSMAX/16/JPN/3ds-Max-Help/index.html?url=files/GUID-8E3FF64C-63B6-42FA-A198-D65E10387A5D.htm,topicNumber=d30e477778
		// グロスマップはともかく、単体（スカラー）のラフネスではなくラフネスマップを使って面のマイクロファセットを定義するようにしたほうがいいかも。
		// Shininess と Roughness の換算式は一応存在するようだが、数学関数を使わなければならないのでヘビー。
		// Shininess = exp((10 * (1 - Roughness) + 2) * log(2)))
		// http://lifewithmodo.blogspot.jp/2011/12/fbx.html
		// この log は log10 なのか？　それとも ln なのか？
		// もし log10 であるならば、R = 1 のとき S = 1.8 で、R = 0 のとき S = 37 となる。
		// もし ln であるならば、R = 1 のとき S = 4 で、R = 0 のとき S = 4096 となる。

		std::wstring m_strMaterialName; //!< マテリアル名。<br>

		std::wstring m_texNameAmbient; // アンビエント テクスチャ名
		std::wstring m_texNameDiffuse; // ディフューズ テクスチャ名
		std::wstring m_texNameSpecular; // スペキュラー テクスチャ名
		std::wstring m_texNameEmissive; // エミッシブ テクスチャ名
		std::wstring m_texNameBump; // バンプ テクスチャ名
		std::wstring m_texNameNormalMap; // 法線マップ テクスチャ名
		std::wstring m_texNameTransparentColor;
		std::wstring m_texNameTransparencyFactor;
		// アルファ マップ相当は TransparencyFactor のほう？
		std::wstring m_texNameReflection;

		std::wstring m_texFileNameAmbient; // アンビエント テクスチャのファイル名
		std::wstring m_texFileNameDiffuse; // ディフューズ テクスチャのファイル名
		std::wstring m_texFileNameSpecular; // スペキュラー テクスチャのファイル名
		std::wstring m_texFileNameEmissive; // エミッシブ テクスチャのファイル名
		std::wstring m_texFileNameBump; // バンプ テクスチャのファイル名
		std::wstring m_texFileNameNormalMap; // 法線マップ テクスチャのファイル名
		std::wstring m_texFileNameTransparentColor;
		std::wstring m_texFileNameTransparencyFactor;
		std::wstring m_texFileNameReflection;

		std::wstring m_texRelativeFileNameAmbient; // アンビエント テクスチャ相対ファイル名
		std::wstring m_texRelativeFileNameDiffuse; // ディフューズ テクスチャ相対ファイル名
		std::wstring m_texRelativeFileNameSpecular; // スペキュラー テクスチャ相対ファイル名
		std::wstring m_texRelativeFileNameEmissive; // エミッシブ テクスチャ相対ファイル名
		std::wstring m_texRelativeFileNameBump; // バンプ テクスチャ相対ファイル名
		std::wstring m_texRelativeFileNameNormalMap; // 法線マップ テクスチャ相対ファイル名
		std::wstring m_texRelativeFileNameTransparentColor;
		std::wstring m_texRelativeFileNameTransparencyFactor;
		std::wstring m_texRelativeFileNameReflection;
	};
}
