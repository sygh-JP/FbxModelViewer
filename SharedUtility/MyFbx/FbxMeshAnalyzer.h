#pragma once

#include "FbxMaterialAnalyzer.h"
#include "FbxSkinAnalyzer.h"
#include "FbxUVAnalyzer.h"
#include "FbxLayerAnalyzer.h"
#include "MyFbx.h"
#include "MyBasicVertexTypes.hpp"


// NOTE: shared_ptr<const T>, shared_ptr<T const> はともに const T* の shared_ptr 版を提供する。
// const 操作のみを許可し、なおかつ shared_ptr<T> を暗黙的に代入することができるようになる。
namespace MyFbx
{

	typedef std::vector<MyMath::Vector3F> TVector3FArray;
	typedef std::vector<MyMath::Vector4F> TVector4FArray;


	inline void GetNodeLocalTransformMatrix(FbxNode* node, MyMath::MatrixF& outMatrix)
	{
		_ASSERTE(node != nullptr);
		// ノードに MQO ローカル座標系が格納されている？
		const auto fbxNodeLocalTransformMat = node->EvaluateLocalTransform();
		MyFbx::ToMatrixF(outMatrix, fbxNodeLocalTransformMat);
	}


	// FBXメッシュ解析クラス
	class MyFbxMeshAnalyzer final
	{
	public:
		typedef std::shared_ptr<MyFbxMeshAnalyzer> TSharedPtr;
		typedef std::shared_ptr<const MyFbxMeshAnalyzer> TConstSharedPtr;
	public:
		MyFbxMeshAnalyzer();

		virtual ~MyFbxMeshAnalyzer();

		// 解析エントリ
		void Analyze(FbxNode* node, const FbxMesh* mesh, const MyFbxAnimTimeInfo& animTimeInfo);

		void SetMeshName(const char* pName) { m_meshName = MyUtils::SafeConvertUtf8toUtf16(pName); }
		void SetMeshName(const wchar_t* pName) { m_meshName = pName ? pName : L""; }

		//! @brief  メッシュ名を取得する。<br>
		const std::wstring& GetMeshNameW() const { return m_meshName; }

		// ポリゴン数を取得
		int GetPolygonCount() const { return m_polygonCount; }

		// インデックスバッファを取得
		const TIntArray& GetIndexBuffer() const { return m_indexBuffer; }

		const TIntArray& GetVertexCountPerPolygonArray() const { return m_vertexCountPerPolygonArray; }

		// ポリゴン構成（プリミティブ個数マップ）を取得
		const TIntToIntMap& GetPrimitiveCountMap() const { return m_primitiveCountMap; }

		// コントロール点バッファを取得
		const TVector4FArray& GetControlPointBuffer() const { return m_ctrlPtsBuffer; }

		// 法線バッファを取得
		const TVector3FArray& GetNormalBuffer() const { return m_normalBuffer; }

		// マテリアルの数を取得
		size_t GetMaterialAnalyzerCount() const { return m_materialAnalyzerArray.size(); }

		//! @brief  マテリアル詳細情報を取得。<br>
		MyFbxMaterialAnalyzer::TConstSharedPtr GetMaterialAnalyzer(size_t index) const
		{
			return (index >= this->GetMaterialAnalyzerCount()) ? MyFbxMaterialAnalyzer::TConstSharedPtr() : m_materialAnalyzerArray[index];
		}

		const TIntArray& GetMaterialIndexBuffer() const { return m_materialIndexBuffer; }

		// （レイヤーごとの）UV の数（レイヤー数）を取得
		size_t GetUVAnalyzerCount() const { return m_uvAnalyzerArray.size(); }

		// （レイヤーごとの）UV を取得
		MyFbxUVAnalyzer::TConstSharedPtr GetUVAnalyzer(size_t index) const
		{
			return (index >= this->GetUVAnalyzerCount()) ? MyFbxUVAnalyzer::TConstSharedPtr() : m_uvAnalyzerArray[index];
		}

		// （レイヤーごとの）ディフューズ UV 座標数を取得
		size_t GetDiffuseUVCoordCountOfLayer(size_t index) const
		{
			return (index >= this->GetUVAnalyzerCount()) ? 0 : m_uvAnalyzerArray[index]->GetDiffuseUVCoordArray().size();
		}

		// スキンの数を取得
		size_t GetSkinAnalyzerCount() const { return m_skinAnalyzerArray.size(); }

		// スキン情報を取得
		MyFbxSkinAnalyzer::TConstSharedPtr GetSkinAnalyzer(size_t index) const
		{
			return (index >= this->GetSkinAnalyzerCount()) ? MyFbxSkinAnalyzer::TConstSharedPtr() : m_skinAnalyzerArray[index];
		}

		LayerModeData GetNormalLayerModeData() const { return m_normalLayerModeData; }
		LayerModeData GetMaterialLayerModeData() const { return m_materialLayerModeData; }

	public:
		void ConvertAllBufferAsTrianglesOnly();

	public:
		bool CreateTriangleOnlySkinMeshSourceData(
			MyVertexTypes::TMySkinVertexArray& outVertexArray,
			std::vector<uint16_t>& outIndexArray,
			MyMath::TMyAttributeRangeArray& outAttrRangeArray,
			MyMath::TMyMaterialPtrsArray& outMaterialsArray,
			TIntArray& outMaterialIndicesForAttrTable
			) const;
	private:
		//! @brief  頂点位置座標を解析する。<br>
		void AnalyzeVertexPosition(const FbxMesh* mesh);

		//! @brief  法線を解析する。<br>
		void AnalyzeNormal(const FbxMesh* mesh);

		//! @brief  マテリアルを解析する。<br>
		void AnalyzeMaterial(const FbxNode* node, const FbxMesh* mesh);

		void AnalyzeMaterialIndexBuffer(const FbxMesh* mesh);

		//! @brief  UV を解析する。<br>
		void AnalyzeUV(const FbxMesh* mesh);

		//! @brief  スキン情報を解析する。<br>
		void AnalyzeSkin(FbxScene* scene, const FbxMesh* mesh, const MyFbxAnimTimeInfo& animTimeInfo);

	private:
		int m_layerCount; // レイヤー数
		int m_polygonCount; // ポリゴン数
		bool m_isMaterialIndexDirectMapped;
		bool m_isConvertedAllBufferAsTrianglesOnly;
		LayerModeData m_normalLayerModeData;
		LayerModeData m_materialLayerModeData;
		//MyMath::MatrixF m_localTransformMatrix;
		//std::wstring m_fbxFilePath;
		//std::wstring m_fbxDirPath;
		std::wstring m_meshName; // メッシュ名
		TIntArray m_indexBuffer; // インデックス配列
		TIntArray m_vertexCountPerPolygonArray; //!< ポリゴンあたりの頂点数の配列。<br>
		TIntArray m_materialIndexBuffer;
		TVector4FArray m_ctrlPtsBuffer; // コントロール点配列（頂点バッファのようなもの？）
		TVector3FArray m_normalBuffer; // 法線配列
		TIntToIntMap m_primitiveCountMap; // プリミティブ個数マップ（頂点数とその面数のペア）
		std::vector<MyFbxMaterialAnalyzer::TSharedPtr> m_materialAnalyzerArray; //!< マテリアル詳細情報配列。<br>
		std::vector<MyFbxUVAnalyzer::TSharedPtr> m_uvAnalyzerArray; // （レイヤーごとの）UV
		std::vector<MyFbxSkinAnalyzer::TSharedPtr> m_skinAnalyzerArray; // スキン
	};
} // end of namespace
