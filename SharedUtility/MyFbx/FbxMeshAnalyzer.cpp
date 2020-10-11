#include "stdafx.h"

#include "FbxAnimTimeInfo.h"
#include "FbxMeshAnalyzer.h"
#include "VertexSeparator.h"


namespace
{
	std::wstring CreateLowerCaseMinimumRelativeFileName(const std::wstring& strSrc)
	{
		if (!strSrc.empty())
		{
			if (::PathIsRelativeW(strSrc.c_str()))
			{
				ATLTRACE(L"\"%s\" is relative path.\n", strSrc.c_str());
			}
			else
			{
				ATLTRACE(L"\"%s\" is not relative path.\n", strSrc.c_str());
			}
			CPathW tempRelPath(strSrc.c_str());
			tempRelPath.StripPath(); // "<Filebody>.<Extension>" だけにする。多重拡張子には未対応。
			tempRelPath.MakePretty(); // Windows 環境を考慮して、すべて小文字にする。
			return tempRelPath.m_strPath.GetString();
			// 相対パスの場合、.FBX ファイルからのパスとみなす。
			// 絶対パスであっても、独自バイナリへの変換する際のことを考慮して、相対パス化してしまう。
		}
		return std::wstring();
	}

#if 0
	void TestMeshElementMaterial(const FbxMesh* mesh)
	{
		const int elementMaterialCount = mesh->GetElementMaterialCount();
		for (int i = 0; i < elementMaterialCount; ++i)
		{
			const auto* elementMaterial = mesh->GetElementMaterial(i);
			const char* name = elementMaterial->GetName();
		}
	}
#endif
} // end of namespace


namespace MyFbx
{

	MyFbxMeshAnalyzer::MyFbxMeshAnalyzer()
		: m_layerCount()
		, m_polygonCount()
		, m_isMaterialIndexDirectMapped()
		, m_isConvertedAllBufferAsTrianglesOnly()
		//, m_localTransformMatrix(MyMath::ZERO_MATRIXF)
	{
	}

	MyFbxMeshAnalyzer::~MyFbxMeshAnalyzer()
	{
	}

	void MyFbxMeshAnalyzer::Analyze(FbxNode* node, const FbxMesh* mesh, const MyFbxAnimTimeInfo& animTimeInfo)
	{
		this->SetMeshName(node->GetName()); // メッシュ名。
		m_polygonCount = mesh->GetPolygonCount(); // ポリゴン数。
		const int polygonVertexCount = mesh->GetPolygonVertexCount(); // 頂点インデックス数。
		// 三角形面に展開する前の数値。
		ATLTRACE(L"Mesh=\"%s\", TotalPolygonCount=%d, TotalIndexCount=%d\n", m_meshName.c_str(), m_polygonCount, polygonVertexCount);

		// 32bit インデックス バッファのコピー。
		m_indexBuffer.resize(polygonVertexCount);
		std::copy(mesh->GetPolygonVertices(), mesh->GetPolygonVertices() + polygonVertexCount, m_indexBuffer.begin());

		// 構成ポリゴン解析。
		// 頂点数別（三角形面、四角形面、……）に数をカウント。
		m_vertexCountPerPolygonArray.resize(m_polygonCount);
		for (int i = 0; i < m_polygonCount; ++i)
		{
			const int indexNumInPolygon = mesh->GetPolygonSize(i);
			m_vertexCountPerPolygonArray[i] = indexNumInPolygon;
#if 1
			// キーがなければ新たにペアを作る。マップの値が新規作成された場合、数値型であればゼロで初期化されている。
			// insert() の戻り値は、マップ中の該当要素を表すイテレータと新規作成か否かを表す bool フラグのペアとなっているので、それを使って判定する方法もある。
			// ちなみに find() して insert() するのは冗長（実質2回のツリーorマップ検索が入る）。
			m_primitiveCountMap[indexNumInPolygon]++;
#else
			// NG。
			auto it = m_primitiveCountMap.find(indexNumInPolygon);
			if (it == m_primitiveCountMap.end())
			{
				// 見つからなかった場合。
				m_primitiveCountMap.insert(std::make_pair(indexNumInPolygon, 1));
			}
			else
			{
				// 見つかった場合。
				it->second += 1;
			}
#endif
		}

		// 頂点座標を格納。
		this->AnalyzeVertexPosition(mesh);

		// 法線を格納。
		this->AnalyzeNormal(mesh);

		// UVを格納。
		this->AnalyzeUV(mesh);

		// マテリアルを格納。
		this->AnalyzeMaterial(node, mesh);

		this->AnalyzeMaterialIndexBuffer(mesh);

		// スキン情報を格納。
		this->AnalyzeSkin(node->GetScene(), mesh, animTimeInfo);
	}

	template<typename T> void AddTriAsTri(std::vector<T>& outBuffer, const std::vector<T>& inBuffer, size_t offset)
	{
		outBuffer.push_back(inBuffer[offset + 0]);
		outBuffer.push_back(inBuffer[offset + 1]);
		outBuffer.push_back(inBuffer[offset + 2]);
	}

	template<typename T> void AddQuadAsDualTri(std::vector<T>& outBuffer, const std::vector<T>& inBuffer, size_t offset)
	{
		outBuffer.push_back(inBuffer[offset + 0]);
		outBuffer.push_back(inBuffer[offset + 1]);
		outBuffer.push_back(inBuffer[offset + 2]);
		outBuffer.push_back(inBuffer[offset + 0]);
		outBuffer.push_back(inBuffer[offset + 2]);
		outBuffer.push_back(inBuffer[offset + 3]);
	}

	//! @brief  インデックス、法線、マテリアル、UV のバッファを、全て三角形面で構成されるように更新する。<br>
	void MyFbxMeshAnalyzer::ConvertAllBufferAsTrianglesOnly()
	{
		if (m_isConvertedAllBufferAsTrianglesOnly)
		{
			// すでに変換済みの場合。
			return;
		}
		else
		{
			// 更新前に、マテリアル インデックス バッファの要素数がポリゴン数と等しいか否かをチェック。
			m_isMaterialIndexDirectMapped = (m_vertexCountPerPolygonArray.size() == m_materialIndexBuffer.size());
		}

		TIntArray newIndexBuffer;
		TVector3FArray newNormalBuffer;
		TUVArray newDiffuseUVBuffer;
		TIntArray newMaterialIndexBuffer;

		_ASSERTE(!m_uvAnalyzerArray.empty());
		auto oldDiffuseUVBuffer = m_uvAnalyzerArray[0]->GetDiffuseUVCoordArray();
		const bool isNormalBufCountSameAsCtrlPtsBufferCount = (m_normalBuffer.size() == m_ctrlPtsBuffer.size());
		const bool isNormalBufCountSameAsCtrlPtsIndexBufferCount = (m_normalBuffer.size() == m_indexBuffer.size());
		if (isNormalBufCountSameAsCtrlPtsBufferCount)
		{
			ATLTRACE(__FUNCTION__ "() NormalBufferCount == CtrlPtsBufferCount.\n");
		}
		else if (isNormalBufCountSameAsCtrlPtsIndexBufferCount)
		{
			ATLTRACE(__FUNCTION__ "() NormalBufferCount == CtrlPtsIndexBufferCount.\n");
		}
		else
		{
			ATLTRACE(__FUNCTION__ "() Unsupported normal-buffer format.\n");
		}
		if (oldDiffuseUVBuffer.size() != m_indexBuffer.size())
		{
			ATLTRACE(__FUNCTION__ "() IndexBufCount != DiffuseUV0BufCount.\n");
			oldDiffuseUVBuffer.resize(m_indexBuffer.size(), MyMath::ZERO_VECTOR2F);
			// HACK: ただの応急処置。必ずしも正しい対処ではない。
			// メッシュに UV が含まれていない場合や、UV 要素数と頂点数が一致する場合などはここに到達する。
			// UV が含まれていない場合はこれでも OK だが、頂点数と一致する場合は不適切。
			// HACK: FBX Converter 付属の LocalMotionBlend.fbx はココで引っかかる。
			// "Floor" メッシュには UV が含まれていないらしい。
		}
		// 頂点に法線ベクトル、DiffuseUV が必ず含まれていることが前提。
		// HACK: 標準外フォーマットでも対応できるようにする。
		// UV は自動計算できないが、法線は一応自動計算できるはず（頂点の定義順から面法線を計算し、平均化する）。
		for (size_t i = 0, offset = 0; i < m_vertexCountPerPolygonArray.size(); ++i)
		{
			const int vertPerPoly = m_vertexCountPerPolygonArray[i];
			if (vertPerPoly == 2)
			{
				// ラインは描画プリミティブの変更が必要になって厄介なのでスキップ。
				// ただし単純にスキップするだけだとインデックス バッファがずれるので、表示が崩れる。
				// vertPerPoly だけ offset を進めることで対処できるはず。
				ATLTRACE("Line primitive is detected.\n");
				//_ASSERTE(!"Line primitive has not been supported yet!!");
			}
			else if (vertPerPoly == 3)
			{
				AddTriAsTri(newIndexBuffer, m_indexBuffer, offset);
				if (isNormalBufCountSameAsCtrlPtsIndexBufferCount)
				{
					AddTriAsTri(newNormalBuffer, m_normalBuffer, offset);
				}
				AddTriAsTri(newDiffuseUVBuffer, oldDiffuseUVBuffer, offset);
				if (m_isMaterialIndexDirectMapped)
				{
					newMaterialIndexBuffer.push_back(m_materialIndexBuffer[i]);
				}
			}
			else if (vertPerPoly == 4)
			{
				// ABCD を ABC + ACD に分解する。
				// 凸4角形面の分割方法は2種類あるが、凹4角形面の分割方法は1種類しかない。
				// また、凸4角形面であっても、4頂点が同一面内に存在しない場合、デザイン時に意図していない分割がなされると歪んでしまう。
				// Direct3D / OpenGL メッシュ化する前に、
				// HACK: FBX SDK に用意されている三角形分割メソッド FbxGeometryConverter::Triangulate() を使ったほうがいいかもしれない。
				// もしくはモデラーで FBX を出力する際に、必ず三角形化する。
				// FBX SDK のメソッドは確実で安定しているはずだが、デバッグ ビルドではかなり遅い。
				AddQuadAsDualTri(newIndexBuffer, m_indexBuffer, offset);
				if (isNormalBufCountSameAsCtrlPtsIndexBufferCount)
				{
					AddQuadAsDualTri(newNormalBuffer, m_normalBuffer, offset);
				}
				AddQuadAsDualTri(newDiffuseUVBuffer, oldDiffuseUVBuffer, offset);
				if (m_isMaterialIndexDirectMapped)
				{
					newMaterialIndexBuffer.push_back(m_materialIndexBuffer[i]);
					newMaterialIndexBuffer.push_back(m_materialIndexBuffer[i]);
				}
			}
			else
			{
				// NOTE: 5角形面以上には現時点で未対応。やはり FbxGeometryConverter::Triangulate() を使ったほうがいい。
				ATLTRACE("Pentagon or other polygon with more corners is detected.\n");
				//_ASSERTE(!"Pentagon or other polygon with more corners has not been supported yet!!");
			}
			offset += vertPerPoly;
		}
		m_indexBuffer.swap(newIndexBuffer);
		if (isNormalBufCountSameAsCtrlPtsIndexBufferCount)
		{
			m_normalBuffer.swap(newNormalBuffer);
		}
		m_uvAnalyzerArray[0]->SetDiffuseUVCoordArray(newDiffuseUVBuffer);
		if (m_isMaterialIndexDirectMapped)
		{
			m_materialIndexBuffer.swap(newMaterialIndexBuffer);
		}
		m_isConvertedAllBufferAsTrianglesOnly = true;
	}

	// std::vector::resize() を使って D3DXVECTOR4 や XMFLOAT4 の配列のサイズを拡張する場合、
	// 各々のクラスのデフォルト コンストラクタは要素をゼロクリアしないことに注意。
	// MFC の CPoint, CSize, CRect に関しても同様。
	// デフォルト コンストラクタ T() が、ゴミデータが入ったままの怠惰なコンストラクタ実装になっているため。
	// ちなみに int や double などの組込型や、ユーザー定義のコンストラクタを持たない POD 型構造体は、
	// T() でゼロ初期化になるので、vector::resize() の第2引数に明示的にゼロ相当値を指定しなくてもゼロクリアされる。

	void MyFbxMeshAnalyzer::AnalyzeVertexPosition(const FbxMesh* mesh)
	{
		const int ctrlPtsCount = mesh->GetControlPointsCount(); // 頂点数
		m_ctrlPtsBuffer.resize(ctrlPtsCount, MyMath::ZERO_VECTOR4F);

		// 頂点バッファをコピー。
		const auto* src = mesh->GetControlPoints();
		for (int i = 0; i < ctrlPtsCount; ++i)
		{
			m_ctrlPtsBuffer[i] = MyFbx::ToVector4F(src[i]);
		}
	}

	void MyFbxMeshAnalyzer::AnalyzeNormal(const FbxMesh* mesh)
	{
		// メッシュに含まれるレイヤーをチェック。
		const int layerNum = mesh->GetLayerCount();
		for (int i = 0; i < layerNum; ++i)
		{
			const auto* pLayer = mesh->GetLayer(i);
			const auto* pLayerElemNormal = pLayer->GetNormals();
			if (!pLayerElemNormal)
			{
				continue;
			}

			// 法線バッファの要素数を取得。
			const int normalCount = pLayerElemNormal->GetDirectArray().GetCount();
#if 0
			// 法線インデックス バッファの要素数を取得。eIndexToDirect のとき必要らしい。
			const int normalIndexCount = pLayerElemNormal->GetIndexArray().GetCount();
#endif
			m_normalBuffer.resize(normalCount, MyMath::ZERO_VECTOR3F);

			// マッピング モードの取得。
			const auto mappingMode = pLayerElemNormal->GetMappingMode();
			// リファレンス モードの取得。
			const auto referenceMode = pLayerElemNormal->GetReferenceMode();
			m_normalLayerModeData.MappingMode = mappingMode;
			m_normalLayerModeData.ReferenceMode = referenceMode;

			// 頂点ごとの法線を取得。
			if ((
				mappingMode == FbxLayerElement::eByPolygonVertex
				||
				mappingMode == FbxLayerElement::eByControlPoint
				)
				&&
				referenceMode == FbxLayerElement::eDirect)
			{
				// eDirect & eByPolygonVertex のとき、頂点インデックス バッファの要素数と法線バッファの要素数が一致する。
				// eDirect & eByControlPoint のとき、頂点バッファの要素数と法線バッファの要素数が一致する。

				// 直接取得モード。
				for (int j = 0; j < normalCount; ++j)
				{
					// 3D Vec <-- 4D Vec
					m_normalBuffer[j] = MyFbx::ToVector3F(pLayerElemNormal->GetDirectArray().GetAt(j));
					//MyMath::NormalizeVector3(&m_normalBuffer[j], &m_normalBuffer[j]);
				}
			}

			break; // 1個だけ取得して終了。
		}
	}

	void MyFbxMeshAnalyzer::AnalyzeMaterial(const FbxNode* node, const FbxMesh* mesh)
	{
		// マテリアルの数を取得。
		m_materialAnalyzerArray.clear();
		const int materialCount = node->GetMaterialCount();
		if (materialCount == 0)
		{
			return;
		}

		// 各マテリアルの詳細情報を取得。
		for (int i = 0; i < materialCount; ++i)
		{
			const auto* material = node->GetMaterial(i);
			_ASSERTE(material != nullptr);
			// マテリアル解析。
			MyFbxMaterialAnalyzer::TSharedPtr pMaterialAnalyzer(new MyFbxMaterialAnalyzer());
			m_materialAnalyzerArray.push_back(pMaterialAnalyzer);
			pMaterialAnalyzer->Analyze(material);
		}
	}

	void MyFbxMeshAnalyzer::AnalyzeMaterialIndexBuffer(const FbxMesh* mesh)
	{
		// メッシュに含まれるレイヤーをチェック。
		const int layerNum = mesh->GetLayerCount();
		for (int i = 0; i < layerNum; ++i)
		{
			const auto* pLayer = mesh->GetLayer(i);
			const auto* pLayerElemMaterial = pLayer->GetMaterials();
			if (!pLayerElemMaterial)
			{
				continue;
			}

			// FbxMesh::GetMaterialIndices() は使わない？

			// マテリアル インデックス バッファの要素数を取得。
			const int materialIndexCount = pLayerElemMaterial->GetIndexArray().GetCount();
			m_materialIndexBuffer.resize(materialIndexCount);
			// 非対応のモードの場合、とりあえずすべてゼロにしておく。

			// マッピング モードの取得。
			const auto mappingMode = pLayerElemMaterial->GetMappingMode();
			// リファレンス モードの取得。
			const auto referenceMode = pLayerElemMaterial->GetReferenceMode();
			m_materialLayerModeData.MappingMode = mappingMode;
			m_materialLayerModeData.ReferenceMode = referenceMode;

			// 法線レイヤー同様に、ポリゴンごとのマテリアル番号を取得する。
			// 頂点ごとの取得（頂点カラー）は未対応。
			if ((
				mappingMode == FbxLayerElement::eByPolygon
				||
				mappingMode == FbxLayerElement::eAllSame
				)
				&&
				referenceMode == FbxLayerElement::eIndexToDirect)
			{
				// インデックス バッファを取得。
				for (int j = 0; j < materialIndexCount; ++j)
				{
					// どのポリゴンにどのマテリアル番号が対応するか、が格納される。
					m_materialIndexBuffer[j] = pLayerElemMaterial->GetIndexArray().GetAt(j);
				}
				// ちなみに FbxLayerElementMaterial::GetDirectArray() は private になっていてアクセスできない。
			}

			break; // 1個だけ取得して終了。
		}
	}

	void MyFbxMeshAnalyzer::AnalyzeUV(const FbxMesh* mesh)
	{
		m_uvAnalyzerArray.clear();
#if 0
		MyFbxUVAnalyzer::TSharedPtr pUVAnalyzer(new MyFbxUVAnalyzer());
		if (pUVAnalyzer->Analyze(mesh))
		{
			m_uvAnalyzerArray.push_back(pUVAnalyzer);
		}
#else
		// メッシュに含まれるレイヤーをチェック。
		const int layerNum = mesh->GetLayerCount();

		for (int i = 0; i < layerNum; ++i)
		{
			// FBX では、UV 座標はレイヤー×マテリアル種別の数だけ存在する。
			// 通例ファースト レイヤーのディフューズ UV がよく使われる。
			// セカンド レイヤー以降の UV の用途は様々。
			const auto* layer = mesh->GetLayer(i);
			MyFbxUVAnalyzer::TSharedPtr pUVAnalyzer(new MyFbxUVAnalyzer());
			if (layer)
			{
				pUVAnalyzer->Analyze(layer);
			}
			// 登録。
			m_uvAnalyzerArray.push_back(pUVAnalyzer);

			// FbxMesh::GetTextureUV() は使わない？

#if 0
			// テクスチャ ファイル名は FbxLayerElementTexture 経由で取得するものだと思っていたが、FbxLayeredTexture の下の FbxFileTexture で取得するらしい？
			// FBX SDK 2020.1 以降では、Metasequoia 用の FBX エクスポーターで出力した fbx ファイルのディフューズテクスチャが FbxSurfaceMaterial 経由で取得できない問題がある。
			// FBX SDK 2020.0.1 以前では FbxSurfaceMaterial 経由で取得できるが、FbxLayer::GetTextures() でディフューズの FbxLayerElementTexture を取得することができない。
			// FBX SDK のバグなのか、それともファイル側やアプリケーション側に問題があるのか、詳細不明。
			// なお、FBX SDK 2020.0 以前にはバッファオーバーフローのセキュリティ脆弱性があるらしい。おそらく 2020.0.1 にも存在すると思われる。いずれは移行が必要。
			// https://www.autodesk.com/trust/security-advisories/adsk-sa-2020-0002
			// https://forest.watch.impress.co.jp/docs/news/1249104.html
			MyFbxLayerAnalyzer::TSharedPtr pLayerAnalyzer(new MyFbxLayerAnalyzer());
			if (layer)
			{
				pLayerAnalyzer->Analyze(layer);
			}
#endif
		}
#endif
	}

	void MyFbxMeshAnalyzer::AnalyzeSkin(FbxScene* scene, const FbxMesh* mesh, const MyFbxAnimTimeInfo& animTimeInfo)
	{
		m_skinAnalyzerArray.clear();
		const int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);

		for (int i = 0; i < skinCount; ++i)
		{
			auto* skin = FbxCast<FbxSkin>(mesh->GetDeformer(i));
			MyFbxSkinAnalyzer::TSharedPtr pSkinAnalyzer(new MyFbxSkinAnalyzer());
			pSkinAnalyzer->Analyze(scene, skin, animTimeInfo);
			m_skinAnalyzerArray.push_back(pSkinAnalyzer);
		}
	}

	bool MyFbxMeshAnalyzer::CreateTriangleOnlySkinMeshSourceData(
		MyVertexTypes::TMySkinVertexArray& outVertexArray,
		std::vector<uint16_t>& outIndexArray,
		MyMath::TMyAttributeRangeArray& outAttrRangeArray,
		MyMath::TMyMaterialPtrsArray& outMaterialsArray,
		TIntArray& outMaterialIndicesForAttrTable
		) const
	{
		auto pMeshAnalyzer = this;
		_ASSERTE(pMeshAnalyzer != nullptr);
		_ASSERTE(m_isConvertedAllBufferAsTrianglesOnly);
		_ASSERTE(outVertexArray.empty());
		_ASSERTE(outIndexArray.empty());
		_ASSERTE(outAttrRangeArray.empty());
		_ASSERTE(outMaterialsArray.empty());
		_ASSERTE(outMaterialIndicesForAttrTable.empty());

		if (pMeshAnalyzer->GetIndexBuffer().empty())
		{
			ATLTRACE("No index buffer.\n");
			return false;
		}

		const size_t indexCount = pMeshAnalyzer->GetIndexBuffer().size();

		const size_t originalVertexCount = pMeshAnalyzer->GetControlPointBuffer().size();

		{
			// HACK: スムージングを掛ける場合、FBX ファイルの法線モードによってはインデックスを使って平均化法線を計算する必要あり？
			// Direct3D 10/11 で描画する場合、普通に頂点シェーダーでランバート シェーディングを行なうと補間されてグーローシェーディングとなる。
			// Direct3D 9 までの D3DSHADEMODE (D3DSHADE_FLAT, D3DSHADE_GOURAUD) は廃止されている。
			// Direct3D 10/11 でフラット シェーディングを行なうには、ジオメトリ シェーダーを使ってプリミティブごとに面法線を計算する必要があるらしい。
			// GLSL の場合、3.x 以降では flat 修飾子を使う。
			// http://msdn.microsoft.com/ja-jp/library/ee415655.aspx
			// http://msdn.microsoft.com/ja-jp/library/ee416406.aspx

			// NOTE: 位置は同じでも、法線あるいはテクスチャ UV の異なる頂点は分離する（別のインデックスを付ける）。
			std::vector<MyVertexTypes::MyExtraDataPerSkinVertex> extraDaraArray(indexCount);
			// 法線および UV が存在する場合、その個数は頂点数もしくはインデックス数と一致するはず。
			// UNDONE: Normal や UV が無い場合には自前で生成する必要がある。特に法線は頂点位置と隣接情報と面法線からの計算が必要となる。
			const bool isNormalBufCountSameAsCtrlPtsBufferCount = (pMeshAnalyzer->GetNormalBuffer().size() == originalVertexCount);
			const bool isNormalBufCountSameAsCtrlPtsIndexBufferCount = (pMeshAnalyzer->GetNormalBuffer().size() == indexCount);
			if (!isNormalBufCountSameAsCtrlPtsBufferCount && !isNormalBufCountSameAsCtrlPtsIndexBufferCount)
			{
				ATLTRACE(__FUNCTION__ "() Unsupported normal-buffer format.\n");
				return false;
			}
			// UNDONE: UV バッファの要素数がインデックス バッファの要素数と同じであるときのみに対応。
			// UV バッファの要素数が頂点バッファの要素数と一致するときには対応していない。
			if (indexCount != pMeshAnalyzer->GetDiffuseUVCoordCountOfLayer(0))
			{
				ATLTRACE(__FUNCTION__ "() IndexBufCount != DiffuseUV0BufCount.\n");
				return false;
			}
			auto pUVAnalyzer = pMeshAnalyzer->GetUVAnalyzer(0);
			_ASSERTE(pUVAnalyzer.get() != nullptr); // UV が存在することが前提。

			// とりあえず 0 番スキン情報のみを使う。しかしマルチスキンレイヤーとかどういう使い道があるのか……
			// マルチ初期姿勢とか？
			auto pSkinAnalyzer = pMeshAnalyzer->GetSkinAnalyzer(0);
			if (!pSkinAnalyzer)
			{
				// スキニング情報はなくても OK。
				ATLTRACE(L"INFO: No Skinning Info in Mesh \"%s\".\n", pMeshAnalyzer->GetMeshNameW().c_str());
			}
			if (pSkinAnalyzer && originalVertexCount != pSkinAnalyzer->GetBoneInflInfoNum())
			{
				// 頂点数とボーン影響度情報数が異なる場合には対応しない。
				ATLTRACE("VertexBufCount != BoneInflInfoCount.\n");
				return false;
			}
			// 通例、オリジナルの頂点数とボーン影響度情報数は同じ。
			// 頂点位置とボーン ウェイトとボーン インデックスを格納した入力データ配列を作成する。
			// ただし、ツールによっては一部の頂点に影響度情報を設定しないで FBX 出力すると、両者の数が一致しないこともある。
			// プログラム側で補正・自動計算するのは面倒なので、簡単のためスキン メッシュ パーツの作成をスキップする。
			// TODO: 変換中にエラーが発生した場合、変換プログラムのログに分かりやすく出力するなりしたほうがよい。
			// bool ではなくエラーコードもしくは例外を使う。ただし C++ 例外はやめておいたほうがよい。

			// FBX には制御点インデックスのほか、法線インデックスや UV インデックスが存在する。
			// 簡単のため、すべてを単一のインデックス バッファで扱えるように頂点情報を統合する（データ周波数をそろえる）。
			// マルチストリーム複数インデックス レンダリングというテクニックを使えば、マルチインデックスに直接対応できないこともない。
			// http://msdn.microsoft.com/ja-jp/library/bb205327.aspx

			// 分離させたい追加データを登録する。
			for (size_t j = 0; j < indexCount; ++j)
			{
				if (isNormalBufCountSameAsCtrlPtsIndexBufferCount)
				{
					extraDaraArray[j].Normal = pMeshAnalyzer->GetNormalBuffer()[j];
				}
				extraDaraArray[j].TexCoord = pUVAnalyzer->GetDiffuseUVCoordArray()[j];
			}
			// 頂点数とボーン影響度情報数は同じなので、先にまとめておく。
			std::vector<MyVertexTypes::MySkinVertex> originalDataArray(originalVertexCount);
			for (size_t j = 0; j < originalVertexCount; ++j)
			{
				originalDataArray[j].Position.x = pMeshAnalyzer->GetControlPointBuffer()[j].x;
				originalDataArray[j].Position.y = pMeshAnalyzer->GetControlPointBuffer()[j].y;
				originalDataArray[j].Position.z = pMeshAnalyzer->GetControlPointBuffer()[j].z;
				if (isNormalBufCountSameAsCtrlPtsBufferCount)
				{
					originalDataArray[j].Normal = pMeshAnalyzer->GetNormalBuffer()[j];
				}

				auto tempBoneInflInfo = pSkinAnalyzer ? pSkinAnalyzer->GetBoneInflInfo(static_cast<int>(j)) : TBoneInfluenceArrayConstPtr();

				if (tempBoneInflInfo)
				{
					// ボーン情報が含まれているスキン メッシュ。
					//float totalWeightForCheck = 0;
					for (size_t k = 0; k < MyVertexTypes::MaxBoneNumPerVertex && k < tempBoneInflInfo->size(); ++k)
					{
						originalDataArray[j].BoneIndices[k] = (*tempBoneInflInfo)[k].Index;
						originalDataArray[j].BoneWeights[k] = (*tempBoneInflInfo)[k].Weight;
						//totalWeightForCheck += originalDataArray[j].BoneWeights[k];
					}
					//ATLTRACE("Vertex[%Iu], TotalWeight = %f\n", j, totalWeightForCheck);
				}
				else
				{
					for (int k = 0; k < MyVertexTypes::MaxBoneNumPerVertex; ++k)
					{
						originalDataArray[j].BoneIndices[k] = 0;
						originalDataArray[j].BoneWeights[k] = 0;
					}
					originalDataArray[j].BoneWeights[0] = 1;
					// ボーン情報が含まれていないメッシュ。
					//_ASSERTE(false);
				}
			}

			// コンストラクタで必要な情報を渡し、インデックスを更新する。
			// C++11 が使えるならば、テンプレート引数は decltype で決めてもいい。
			MyUtility::VertexSeparator<MyVertexTypes::MySkinVertex, int, MyVertexTypes::MyExtraDataPerSkinVertex> vs(
				originalVertexCount, indexCount, &pMeshAnalyzer->GetIndexBuffer()[0]);
			vs.UpdateIndex(&extraDaraArray[0]);

			// 頂点を複製して、新たな頂点バッファを作成する。
			std::vector<MyVertexTypes::MySkinVertex> newVertices(vs.GetNewVertexCount()); // 複製された頂点を格納するバッファ。
			vs.SeparateData(&newVertices[0], &originalDataArray[0]);
			// HACK: 念のため、VertexSeparator のすべてのメンバー関数は、引数に出力先バッファのサイズを渡すようにする。あるいは最初から自己記述型配列を使う。

			// 不要なデータを削除した、新たな追加データ配列を作成する。要素数は頂点バッファの要素数と一致する。
			std::vector<MyVertexTypes::MyExtraDataPerSkinVertex> newExtraDataArray(vs.GetNewVertexCount());
			vs.GetNewExtraDataArray(&newExtraDataArray[0], &extraDaraArray[0]);
			// HACK: 念のため、VertexSeparator のすべてのメンバー関数は、引数に出力先バッファのサイズを渡すようにする。あるいは最初から自己記述型配列を使う。

			// 更新されたインデックス バッファを取得し、16bit インデックスに変換する。
			// ちなみに 32bit インデックスは 64bit アプリケーション向け。32bit アプリでは現実的でない。
			// http://msdn.microsoft.com/ja-jp/library/ee417369.aspx
			// http://msdn.microsoft.com/ja-jp/library/ee416752.aspx
			outIndexArray.resize(indexCount);
			for (size_t j = 0; j < indexCount; ++j)
			{
				outIndexArray[j] = vs.GetNewIndexBuffer()[j];
			}

			outVertexArray.resize(newVertices.size());

			for (size_t j = 0; j < outVertexArray.size(); ++j)
			{
				outVertexArray[j].Position = newVertices[j].Position;
				if (isNormalBufCountSameAsCtrlPtsBufferCount)
				{
					outVertexArray[j].Normal = newVertices[j].Normal;
				}
				else if (isNormalBufCountSameAsCtrlPtsIndexBufferCount)
				{
					outVertexArray[j].Normal = newExtraDataArray[j].Normal;
				}
				outVertexArray[j].TexCoord = newExtraDataArray[j].TexCoord;
				for (int k = 0; k < MyVertexTypes::MaxBoneNumPerVertex; ++k)
				{
					outVertexArray[j].BoneIndices[k] = newVertices[j].BoneIndices[k];
					outVertexArray[j].BoneWeights[k] = newVertices[j].BoneWeights[k];
				}
			}

			// マテリアルの取得とコンポジション化。複数のマテリアルが含まれる場合、マテリアルごとにメッシュ パーツ（属性テーブル）を作成する。
			// UNDONE: マテリアルをキーにして安定ソートして、同じマテリアルの属性テーブルはまとめるように手動最適化する？
			// ID3DX10Mesh::Optimize() と D3DX10_MESHOPT_ATTR_SORT を使う？

			for (size_t j = 0; j < pMeshAnalyzer->GetMaterialAnalyzerCount(); ++j)
			{
				auto pMaterialAnalyzer = pMeshAnalyzer->GetMaterialAnalyzer(j);
				MyMath::MyMaterial::TSharedPtr pTempMat(new MyMath::MyMaterial());
				const float diffuseLevel = pMaterialAnalyzer->GetFbxDiffuseFactor();
				const float ambientLevel = pMaterialAnalyzer->GetFbxAmbientFactor();
				const float specularLevel = pMaterialAnalyzer->GetFbxSpecularFactor();
				const float emissiveLevel = pMaterialAnalyzer->GetFbxEmissiveFactor();
				pTempMat->SetDiffuse(MyMath::CreateVector4(pMaterialAnalyzer->GetFbxRgbDiffuse(), diffuseLevel));
				pTempMat->SetAmbient(MyMath::CreateVector4(pMaterialAnalyzer->GetFbxRgbAmbient(), ambientLevel));
				pTempMat->SetSpecular(MyMath::CreateVector4(pMaterialAnalyzer->GetFbxRgbSpecular(), specularLevel));
				pTempMat->SetEmissive(MyMath::CreateVector4(pMaterialAnalyzer->GetFbxRgbEmissive(), emissiveLevel));
				pTempMat->SetTransparentColorRGB(pMaterialAnalyzer->GetFbxRgbTransparentColor());
				pTempMat->SetTransparency(pMaterialAnalyzer->GetFbxTransparencyFactor());
				pTempMat->SetReflectivity(pMaterialAnalyzer->GetFbxReflectivity());
				pTempMat->SetSpecularPower(pMaterialAnalyzer->GetFbxShininess());
				pTempMat->SetRoughness(pMaterialAnalyzer->GetFbxRoughness());
				// マテリアル名に特定の文字列を含む場合、ファーを有効にするようにしたり、もしくは
				// ビューアーで UI を使って個別設定できるようにする。
				// トゥーン設定同様に、FBX を読み込んだ時点ではファー OFF で、UI で設定できるようにしたほうがよいかも。
				// シャドウ レシーバー／キャスターの設定は、マテリアル単位ではなくオブジェクト単位（メッシュ部品単位）で行なう。
				// FBX では FbxGeometryBase クラスに CastShadow / ReceiveShadow のプロパティがあるらしい。
				pTempMat->SetMaterialName(pMaterialAnalyzer->GetMaterialNameW().c_str());
				// とりあえず、テクスチャはディフューズ マップと法線マップのみ対応。3D ツールや FBX SDK 仕様に依存しないよう、確実に相対パス化する。
				// Metasequoia はカラーマップ（ディフューズ マップ）・アルファ マップ・バンプ マップにしか対応してないので、
				// Metasequoia プラグインを使って FBX 出力したものを直に使う場合は、
				// 法線マップよりもバンプ マップのファイル名を利用したほうがよいかも。
				pTempMat->SetTexFileNameDiffuseMap(CreateLowerCaseMinimumRelativeFileName(
					pMaterialAnalyzer->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Diffuse)).c_str());
				pTempMat->SetTexFileNameNormalMap(CreateLowerCaseMinimumRelativeFileName(
					pMaterialAnalyzer->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_NormalMap)).c_str());
				outMaterialsArray.push_back(pTempMat);
			}

			if (m_isMaterialIndexDirectMapped)
			{
				// 面ごとにマテリアルが割り当てられている場合。
				int prevMatIndex = -1;
				int lastAttrId = -1;
				for (size_t j = 0; j < pMeshAnalyzer->GetMaterialIndexBuffer().size(); ++j)
				{
					const int currMatIndex = pMeshAnalyzer->GetMaterialIndexBuffer()[j];
					if (prevMatIndex != currMatIndex)
					{
						// 今回の用途では MyAttributeRange::VertexStart は常にゼロで OK。
						// また、MyAttributeRange::AttribId は ID3DX10Mesh を使う場合のみ利用される。
						++lastAttrId;
						MyMath::MyAttributeRange tempAttr;
						tempAttr.AttribId = lastAttrId;
						tempAttr.FaceStart = uint32_t(j);
						tempAttr.VertexCount = uint32_t(outVertexArray.size());
						outAttrRangeArray.push_back(tempAttr);
						outMaterialIndicesForAttrTable.push_back(currMatIndex);
						prevMatIndex = currMatIndex;
					}
					if (lastAttrId >= 0)
					{
						outAttrRangeArray[lastAttrId].FaceCount++;
					}
				}
			}
			else if (!outMaterialsArray.empty())
			{
				//outMaterialIndicesForAttrTable = pMeshAnalyzer->GetMaterialIndexBuffer();
				_ASSERTE(pMeshAnalyzer->GetMaterialIndexBuffer().size() == 1);
				_ASSERTE(pMeshAnalyzer->GetMaterialIndexBuffer()[0] == 0);
				_ASSERTE(outMaterialIndicesForAttrTable.empty());
				outMaterialIndicesForAttrTable.push_back(0);

				_ASSERTE(outAttrRangeArray.empty());
				MyMath::MyAttributeRange tempAttr;
				tempAttr.VertexCount = uint32_t(outVertexArray.size());
				tempAttr.FaceCount = uint32_t(outIndexArray.size() / 3);
				outAttrRangeArray.push_back(tempAttr);
			}
		}

		_ASSERTE(outIndexArray.size() == indexCount);

		return true;
	}

} // end of namespace
