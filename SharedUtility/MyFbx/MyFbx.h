#pragma once

#include "MyMath.hpp"


namespace MyFbx
{
#pragma region // typedefs //

	typedef std::shared_ptr<FbxManager> TFbxSdkManagerPtr;
	typedef std::shared_ptr<FbxScene> TFbxSdkScenePtr;
	typedef std::shared_ptr<FbxImporter> TFbxSdkImporterPtr;
	typedef std::shared_ptr<FbxMesh> TFbxSdkMeshPtr;

	typedef FbxPropertyT<FbxDouble4> TFbxPropertyDouble4;
	typedef FbxPropertyT<FbxDouble3> TFbxPropertyDouble3;
	typedef FbxPropertyT<FbxDouble2> TFbxPropertyDouble2;
	typedef FbxPropertyT<FbxDouble>  TFbxPropertyDouble;
#if 0
	// FBX SDK 2013.3 にはいくつかの K～ 互換シンボルが存在しない。
	// FBX SDK 2014.1 では完全に削除されている模様。
	// もともと Kaydara で開発・管理されていた SDK だったので、K プレフィックスが付いていたらしい？
	__declspec(deprecated) typedef FbxPropertyT<FbxDouble4> KFbxPropertyDouble4;
	__declspec(deprecated) typedef FbxPropertyT<FbxDouble3> KFbxPropertyDouble3;
	__declspec(deprecated) typedef FbxTrimNurbsSurface KFbxTrimNurbsSurface;
	__declspec(deprecated) typedef FbxBoundary KFbxBoundary;
#endif

#pragma endregion

	class LayerModeData final
	{
	public:
		FbxLayerElement::EMappingMode MappingMode;
		FbxLayerElement::EReferenceMode ReferenceMode;
	public:
		LayerModeData()
			: MappingMode()
			, ReferenceMode()
		{}
		explicit LayerModeData(FbxLayerElement::EMappingMode mapMode, FbxLayerElement::EReferenceMode refMode)
			: MappingMode(mapMode)
			, ReferenceMode(refMode)
		{}
	};

	extern const wchar_t* GetLayerMappingModeNameW(FbxLayerElement::EMappingMode mapMode);

	extern const wchar_t* GetLayerReferenceModeNameW(FbxLayerElement::EReferenceMode refMode);

	extern bool CreateSceneFromFbx(LPCWSTR pFilePath, FbxManager* pSdkManager, TFbxSdkScenePtr& pScene);

	extern void ToMatrixF(MyMath::MatrixF& dst, const FbxAMatrix& src);

	inline void ToVector4F(MyMath::Vector4F& dst, const FbxVector4& src)
	{
		dst.x = static_cast<float>(src[0]);
		dst.y = static_cast<float>(src[1]);
		dst.z = static_cast<float>(src[2]);
		dst.w = static_cast<float>(src[3]);
	}

	inline MyMath::Vector4F ToVector4F(const FbxVector4& src)
	{
		MyMath::Vector4F dst;
		ToVector4F(dst, src);
		return dst;
	}

	// w を欠落させる。
	inline void ToVector3F(MyMath::Vector3F& dst, const FbxVector4& src)
	{
		dst.x = static_cast<float>(src[0]);
		dst.y = static_cast<float>(src[1]);
		dst.z = static_cast<float>(src[2]);
	}

	// w を欠落させる。
	inline MyMath::Vector3F ToVector3F(const FbxVector4& src)
	{
		MyMath::Vector3F dst;
		ToVector3F(dst, src);
		return dst;
	}

	// FbxVector3 は存在しないらしい。

	inline void ToVector2F(MyMath::Vector2F& dst, const FbxVector2& src)
	{
		dst.x = static_cast<float>(src[0]);
		dst.y = static_cast<float>(src[1]);
	}

	inline MyMath::Vector2F ToVector2F(const FbxVector2& src)
	{
		MyMath::Vector2F dst;
		ToVector2F(dst, src);
		return dst;
	}

	inline void ToVector4F(MyMath::Vector4F& dst, const TFbxPropertyDouble4& src)
	{
		dst.x = static_cast<float>(src.Get()[0]);
		dst.y = static_cast<float>(src.Get()[1]);
		dst.z = static_cast<float>(src.Get()[2]);
		dst.w = static_cast<float>(src.Get()[3]);
	}

	inline MyMath::Vector4F ToVector4F(const TFbxPropertyDouble4& src)
	{
		MyMath::Vector4F dst;
		ToVector4F(dst, src);
		return dst;
	}

	inline void ToVector3F(MyMath::Vector3F& dst, const TFbxPropertyDouble3& src)
	{
		dst.x = static_cast<float>(src.Get()[0]);
		dst.y = static_cast<float>(src.Get()[1]);
		dst.z = static_cast<float>(src.Get()[2]);
	}

	inline MyMath::Vector3F ToVector3F(const TFbxPropertyDouble3& src)
	{
		MyMath::Vector3F dst;
		ToVector3F(dst, src);
		return dst;
	}

	inline void ToVector2F(MyMath::Vector2F& dst, const TFbxPropertyDouble2& src)
	{
		dst.x = static_cast<float>(src.Get()[0]);
		dst.y = static_cast<float>(src.Get()[1]);
	}

	inline MyMath::Vector2F ToVector2F(const TFbxPropertyDouble2& src)
	{
		MyMath::Vector2F dst;
		ToVector2F(dst, src);
		return dst;
	}

	inline float ToFloat(const TFbxPropertyDouble& src)
	{
		return static_cast<float>(src.Get());
	}


	//! @brief  ダックタイピングでカスタム デリーターを定義するための FBX オブジェクト用ヘルパー。<br>
	//! 
	//! FBX オブジェクトの作成と破棄はすべて Create/Destroy によるファクトリ デザイン パターンで実装されている。<br>
	//! delete でデストラクタを直接呼び出すことはできない。<br>
	template<typename T> inline void SafeDestroy(T*& ptr)
	{
		if (ptr)
		{
			ptr->Destroy();
			ptr = nullptr;
		}
	}

	// FBX SDK 2013.3 以降の DLL は RTTI を OFF にしてビルドされているらしく、
	// DLL 内で生成されたオブジェクトへのポインタを dynamic_cast すると std::__non_rtti_object 例外が送出される。
	// レガシーだが型情報は enum で保持するようなルールになっているようなので、
	// それを信用して静的ダウンキャストを使うようにする。
	template<typename T> T* StaticDownCast(void* pIntermediate)
	{ return static_cast<T*>(pIntermediate); }

	template<typename T> const T* StaticDownCast(const void* pIntermediate)
	{ return static_cast<const T*>(pIntermediate); }

#if 0
	// 他と違う処理が必要なので例外的に特殊化する。
	template<> inline void SafeDestroy(KFbxSdkManager*& ptr)
	{
		if (ptr)
		{
			IOSREF.FreeIOSettings(); // KFbxSdkManager::Destroy() を呼ぶ前にこいつを呼び出さないとメモリーリークする。
			// FBX SDK 2011.2 以降では、IOSREF が存在しない模様。リークも修正されている模様。
			ptr->Destroy();
			ptr = nullptr;
		}
	}
#endif

	//! @brief  shared_ptr のコンストラクタに渡すカスタム削除子。<br>
	template<typename T> struct Deleter
	{
		void operator()(T* ptr)
		{
			SafeDestroy(ptr);
		}
	};

} // end of namespace
