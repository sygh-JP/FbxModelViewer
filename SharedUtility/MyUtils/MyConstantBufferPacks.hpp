#pragma once

#include "MyBoneMatrixPalettePack.h"


namespace MyCpuGpuCommon
{

	class MyViewParamsPack final
	{
	public:
		MyViewParamsPack()
			: UniWorldMatrix(MyMath::ZERO_MATRIXF)
			, UniViewMatrix(MyMath::ZERO_MATRIXF)
			, UniProjectionMatrix(MyMath::ZERO_MATRIXF)
			, UniWorldViewMatrix(MyMath::ZERO_MATRIXF)
			, UniViewProjMatrix(MyMath::ZERO_MATRIXF)
			, UniWorldViewProjMatrix(MyMath::ZERO_MATRIXF)
			, UniEyePosition(MyMath::ZERO_VECTOR3F)
			, UniEyePositionDummy()
			, UniScreenSize(MyMath::ZERO_VECTOR2I)
			, UniScreenSizeDummy()
		{}
	public:
		MyMath::MatrixF UniWorldMatrix;
		MyMath::MatrixF UniViewMatrix;
		MyMath::MatrixF UniProjectionMatrix;
		MyMath::MatrixF UniWorldViewMatrix;
		MyMath::MatrixF UniViewProjMatrix;
		MyMath::MatrixF UniWorldViewProjMatrix;

		MyMath::Vector3F UniEyePosition;
		float UniEyePositionDummy;

		MyMath::Vector2I UniScreenSize;
		int32_t UniScreenSizeDummy[2];
	};


	class MyMeshPartAttributePack final
	{
	public:
		MyMeshPartAttributePack()
			: MaterialColorDiffuse(MyMath::ZERO_VECTOR4F)
			, MaterialColorAmbient(MyMath::ZERO_VECTOR4F)
			, MaterialColorSpecular(MyMath::ZERO_VECTOR4F)
			, MaterialColorEmissive(MyMath::ZERO_VECTOR4F)
			, MaterialOpacityAlpha(1)
			, MaterialSpecularPower(MyMath::MinSpecularPowerValue)
			, MaterialReflectivity()
			, MaterialIndexOfRefraction()
			, ToonShadingRefTexV()
			, UniMaterialRoughness()
			, Dummy0()
		{}
	public:
		// メッシュのカレント マテリアルのディフューズ。
		MyMath::Vector4F MaterialColorDiffuse;
		// メッシュのカレント マテリアルのアンビエント。
		MyMath::Vector4F MaterialColorAmbient;
		// メッシュのカレント マテリアルのスペキュラー。
		MyMath::Vector4F MaterialColorSpecular;
		// メッシュのカレント マテリアルのエミッシブ。
		MyMath::Vector4F MaterialColorEmissive;
		float MaterialOpacityAlpha; // マテリアル全体のアルファ。
		float MaterialSpecularPower;
		float MaterialReflectivity;
		float MaterialIndexOfRefraction;
		float ToonShadingRefTexV;
		float UniMaterialRoughness;
		float Dummy0[2];
	};


	// ライティング情報。
	class MyLightParamsPack
	{
	public:
		MyLightParamsPack()
			: LightDir(MyMath::ZERO_VECTOR3F)
			, LightDirDummy()
			, LightPos(MyMath::ZERO_VECTOR3F)
			, LightPosDummy()
			, LightColor(MyMath::ZERO_VECTOR4F)
			, AmbientLight(MyMath::ZERO_VECTOR4F)
		{}
	public:
		// ライトの向き。
		MyMath::Vector3F LightDir;
		float LightDirDummy;
		// ライトのワールド位置。
		MyMath::Vector3F LightPos;
		float LightPosDummy;
		// ライトの色。
		MyMath::Vector4F LightColor;
		// 環境光の色。
		MyMath::Vector4F AmbientLight;
	};

	static_assert(sizeof(MyViewParamsPack) % 16 == 0, "Not aligned!!");
	static_assert(sizeof(MyMeshPartAttributePack) % 16 == 0, "Not aligned!!");
	static_assert(sizeof(MyLightParamsPack) % 16 == 0, "Not aligned!!");
}
