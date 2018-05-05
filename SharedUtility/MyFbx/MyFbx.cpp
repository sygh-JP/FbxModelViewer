#include "stdafx.h"

#include "MyFbx.h"
#include "MyUtil.h"

//#pragma warning (disable : 4996) // FBX SDK の K_DEPRECATED 起因で、関数が古い形式として宣言されたという警告が出るのを抑止する。ただ廃止予定の関数を使うべきでないので、本当は抑止してはいけない。


namespace MyFbx
{
	const wchar_t* GetLayerMappingModeNameW(FbxLayerElement::EMappingMode mapMode)
	{
		switch (mapMode)
		{
		case FbxLayerElement::eNone:
			return L"None";
		case FbxLayerElement::eByControlPoint:
			return L"ByControlPoint";
		case FbxLayerElement::eByPolygonVertex:
			return L"ByPolygonVertex";
		case FbxLayerElement::eByPolygon:
			return L"ByPolygon";
		case FbxLayerElement::eByEdge:
			return L"ByEdge";
		case FbxLayerElement::eAllSame:
			return L"AllSame";
		default:
			return L"";
		}
	}

	const wchar_t* GetLayerReferenceModeNameW(FbxLayerElement::EReferenceMode refMode)
	{
		switch (refMode)
		{
		case FbxLayerElement::eDirect:
			return L"Direct";
		case FbxLayerElement::eIndex:
			return L"Index";
		case FbxLayerElement::eIndexToDirect:
			return L"IndexToDirect";
		default:
			return L"";
		}
	}

	// 旧 KFbxXMatrix は FBX SDK 2014.1 で完全廃止され、FbxAMatrix となったらしい。
	void ToMatrixF(MyMath::MatrixF& dst, const FbxAMatrix& src)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				dst(i, j) = static_cast<float>(src.mData[i][j]);
			}
		}
	}


	//! @brief  FBX ファイルからシーンを作成する。<br>
	bool CreateSceneFromFbx(LPCWSTR pFilePath, FbxManager* pSdkManager, TFbxSdkScenePtr& pScene)
	{
		_ASSERTE(pSdkManager != nullptr);

		// インポータの生成。名前を付ける必要は特にないらしい。付ける場合はおそらく UTF-8 を使う必要がある。
		TFbxSdkImporterPtr pImporter(FbxImporter::Create(pSdkManager, ""), MyFbx::Deleter<FbxImporter>());

		std::string fullPath;
		MyUtils::GetUtf8FullPath(pFilePath, fullPath);
		int fileFormat = -1;
#if 0
		// FBX SDK 2010.2 以前
		if (!pSdkManager->GetIOPluginRegistry()->DetectFileFormat(fullPath.c_str(), fileFormat))
#else
		// FBX SDK 2011.2 以降
		if (!pSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(fullPath.c_str(), fileFormat))
#endif
		{
			return false;
		}
#if 0
		// FBX SDK 2010.2 以前
		pImporter->SetFileFormat(fileFormat);
#endif

		if (!pImporter->Initialize(fullPath.c_str()))
		{
			return false;
		}

		// インポータとシーンを関連付ける。
		pScene = TFbxSdkScenePtr(FbxScene::Create(pSdkManager, ""), MyFbx::Deleter<FbxScene>());
		pImporter->Import(pScene.get());

		return true;
	}

} // end of namespace
