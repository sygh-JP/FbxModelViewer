#include "stdafx.h"


#include "FbxNodeTree.h"
#include "MyUtil.h"
#include "MyFbx.h"
#include "DebugNew.h"


namespace
{
	inline CStringW GetConnectedNamesString(const FbxNode* node, LPCWSTR label)
	{
		CStringW str;
		str.Format(L"%s : \"%s\"", label, MyUtils::SafeConvertUtf8toUtf16(node->GetName()).c_str());
		return str;
	}

	// マテリアル属性出力
	void InsertMaterialProperty(CTreeCtrl* pTree, HTREEITEM hParent, LPCTSTR pNodeName,
		const MyMath::Vector3F& color, bool colorExists,
		float factor, bool factorExists,
		const std::wstring& inTexName, const std::wstring& inTexFileName, const std::wstring& inTexRelativeFileName)
	{
		HTREEITEM propLabel = pTree->InsertItem(pNodeName, hParent);

		if (colorExists)
		{
			CString label;
			label.Format(_T("Color : R=%f, G=%f, B=%f"), color.x, color.y, color.z);
			pTree->InsertItem(label.GetString(), propLabel);
		}
		if (factorExists)
		{
			CString label;
			label.Format(_T("Factor = %f"), factor);
			pTree->InsertItem(label.GetString(), propLabel);
		}
		const CString texName = CString(_T("TextureName : ")) + inTexName.c_str();
		pTree->InsertItem(texName, propLabel);
		const CString texFileName = CString(_T("TextureFileName : ")) + inTexFileName.c_str();
		pTree->InsertItem(texFileName, propLabel);
		const CString texRelFileName = CString(_T("TextureRelativeFileName : ")) + inTexRelativeFileName.c_str();
		pTree->InsertItem(texRelFileName, propLabel);
	}

	inline CString ConvertVector3ToString(const MyMath::Vector3F& color)
	{
		CString str;
		str.Format(_T("%f, %f, %f"), color.x, color.y, color.z);
		return str;
	}
} // end of namespace


namespace MyMfcFbx
{
	MyFbxNodeTree::MyFbxNodeTree(CTreeCtrl* pTreeCtrl, CRichEditCtrl* pEditCtrl)
		: m_pTreeCtrl(pTreeCtrl)
		, m_pEditCtrl(pEditCtrl)
	{
	}

	MyFbxNodeTree::~MyFbxNodeTree()
	{
	}

	// ツリー末尾へ出力し、スタックに登録する。
	void MyFbxNodeTree::InsertToLast(LPCWSTR pNodeLabelStr)
	{
		const CString label(pNodeLabelStr);

		// アイテムを追加
		HTREEITEM parent = this->GetLastTreeItem();
		HTREEITEM item = m_pTreeCtrl->InsertItem(label, parent);

		// TODO: I_CHILDRENCALLBACK と WM_NOTIFY を使って仮想化する。
		// https://docs.microsoft.com/en-us/windows/win32/controls/tvn-getdispinfo
		// https://docs.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-tvitemw

		m_itemStack.push_back(item);
	}

	void MyFbxNodeTree::OnPreAnalyze(FbxScene* scene)
	{
		CString label;

		// アニメーション スタックは FBX のルートノード（属性なしのノード）には接続せず、別のツリーを作る。
		this->InsertToLast(_T("FBX File"));

		HTREEITEM parent = this->GetLastTreeItem();

		// アニメーション情報は含まれているが、スケルトンは含まれていない FBX も理論的にはありえる。
		// スキニングせずに、ルートのグローバル行列のみアニメーションするとか。
		// そんな FBX ファイルが実用的なのか知らないが……カメラのモーションを記録しただけのファイルとか？
		HTREEITEM hAnimStackLabel = m_pTreeCtrl->InsertItem(_T("AnimStacks"), parent);
		const auto& animStackNames = this->GetAnimTimeInfo().GetAnimStackNamesArray();
		if (animStackNames.empty())
		{
			m_pTreeCtrl->InsertItem(_T("None"), hAnimStackLabel);
		}
		else
		{
			label.Format(_T("FPS : %d"), this->GetAnimTimeInfo().GetFramesPerSec());
			m_pTreeCtrl->InsertItem(label, hAnimStackLabel);
			for (size_t i = 0; i < animStackNames.size(); ++i)
			{
				label.Format(_T("AnimStack[%Iu] : \"%s\""), i, CString(animStackNames[i].c_str()).GetString());
				m_pTreeCtrl->InsertItem(label, hAnimStackLabel);
			}
		}
	}

	void MyFbxNodeTree::OnPostAnalyze(FbxScene* scene)
	{
		this->OutputDetailInfoToEditCtrl();
	}

	void MyFbxNodeTree::OnPreAnalyzeChildren(FbxNode* parent)
	{
		__noop;
	}

	void MyFbxNodeTree::OnPostAnalyzeChildren(FbxNode* parent)
	{
		m_itemStack.pop_back();
	}

	void MyFbxNodeTree::OnCatchFbxError(FbxNode* errNode)
	{
	}

	void MyFbxNodeTree::OnFindFbxRootNode(FbxNode* root)
	{
		this->InsertToLast(GetConnectedNamesString(root, L"Root").GetString());

		MyMath::MatrixF localTransformMatrix;
		MyFbx::GetNodeLocalTransformMatrix(root, localTransformMatrix);

		HTREEITEM parent = this->GetLastTreeItem();

		CString label;

		// ローカル座標系の平行移動成分を出力してみる。
		label.Format(_T("LT.T(%f, %f, %f)"), localTransformMatrix._41, localTransformMatrix._42, localTransformMatrix._43);
		m_pTreeCtrl->InsertItem(label, parent);
	}

	void MyFbxNodeTree::OnFindFbxUnidentifiedNode(FbxNode* node)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Unidentified").GetString());
	}

	void MyFbxNodeTree::OnFindFbxNullNode(FbxNode* node, FbxNull* nullNode)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Null").GetString());
	}

	void MyFbxNodeTree::OnFindFbxMarkerNode(FbxNode* node, FbxMarker* marker)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Marker").GetString());
	}

	void MyFbxNodeTree::OnFindFbxSkeletonNode(FbxNode* node, FbxSkeleton* skeleton)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Skeleton").GetString());
	}

	void MyFbxNodeTree::OnFindFbxMeshNode(FbxNode* node, FbxMesh* mesh, const MyFbx::MyFbxMeshAnalyzer* pMeshAnalyzer)
	{
		// MSVC CRT およびそれに依存するライブラリ等では C99 の "%zd", "%zu", "%td", "%tu" が使えない。
		// ただし、MS 拡張としては、ptrdiff_t に対しては "%Id"、size_t に対しては "%Iu" などが指定できる。
		// VC2013 までは t, z が実装されていなかったが、VC2015 以降は実装されている模様。
		// ATLTRACE() や CString::Format() でも使用可能。
		// https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2013/tcxf1dw6%28v=vs.120%29
		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/format-specification-syntax-printf-and-wprintf-functions?view=msvc-140

		using namespace MyFbx;

		this->InsertToLast(GetConnectedNamesString(node, L"Mesh").GetString());

		MyMath::MatrixF localTransformMatrix;
		MyFbx::GetNodeLocalTransformMatrix(node, localTransformMatrix);

		HTREEITEM parent = this->GetLastTreeItem();

		CString label;

		// ローカル座標系の平行移動成分を出力してみる。
		label.Format(_T("LT.T(%f, %f, %f)"), localTransformMatrix._41, localTransformMatrix._42, localTransformMatrix._43);
		m_pTreeCtrl->InsertItem(label, parent);

		if (pMeshAnalyzer == nullptr)
		{
			return;
		}

		// 頂点数
		label.Format(_T("Vertex : %lu"), static_cast<ULONG>(pMeshAnalyzer->GetControlPointBuffer().size()));
		m_pTreeCtrl->InsertItem(label, parent);

		// ポリゴン数
		label.Format(_T("Polygon : %d"), pMeshAnalyzer->GetPolygonCount());
		HTREEITEM polygonLabel = m_pTreeCtrl->InsertItem(label, parent);

		// ポリゴン構成
		const auto& primitiveCountMap = pMeshAnalyzer->GetPrimitiveCountMap();

		for (auto it = primitiveCountMap.cbegin(); it != primitiveCountMap.cend(); ++it)
		{
			label.Format(_T("%d vertices prim. : %d"), it->first, it->second);
			m_pTreeCtrl->InsertItem(label, polygonLabel);
		}

		// 頂点インデックス数。
		label.Format(_T("IndexBuffer : %lu"), static_cast<ULONG>(pMeshAnalyzer->GetIndexBuffer().size()));
		m_pTreeCtrl->InsertItem(label, parent);

		// 法線数。
		label.Format(_T("Normal : %lu"), static_cast<ULONG>(pMeshAnalyzer->GetNormalBuffer().size()));
		m_pTreeCtrl->InsertItem(label, parent);

		label = CString(_T("NormalLayer.MappingMode : "))
			+ MyFbx::GetLayerMappingModeNameW(pMeshAnalyzer->GetNormalLayerModeData().MappingMode);
		m_pTreeCtrl->InsertItem(label, parent);
		label = CString(_T("NormalLayer.ReferenceMode : "))
			+ MyFbx::GetLayerReferenceModeNameW(pMeshAnalyzer->GetNormalLayerModeData().ReferenceMode);
		m_pTreeCtrl->InsertItem(label, parent);

		// UV レイヤー数。
		label.Format(_T("UVLayer : %lu"), static_cast<ULONG>(pMeshAnalyzer->GetUVAnalyzerCount()));
		HTREEITEM hItemUVLayer = m_pTreeCtrl->InsertItem(label, parent);

		// DiffuseUV[0] の座標数。
		label.Format(_T("DiffuseUV[0] : %lu"), static_cast<ULONG>(pMeshAnalyzer->GetDiffuseUVCoordCountOfLayer(0)));
		m_pTreeCtrl->InsertItem(label, hItemUVLayer);

#pragma region // マテリアル情報。//
		HTREEITEM materialInfoLabel = m_pTreeCtrl->InsertItem(_T("Materials"), parent);

		// マテリアル数。
		const size_t materialNum = pMeshAnalyzer->GetMaterialAnalyzerCount();
		label.Format(_T("MaterialNum : %Iu"), materialNum);
		m_pTreeCtrl->InsertItem(label, materialInfoLabel);
		label.Format(_T("MaterialIndexBuffer : %Iu"), pMeshAnalyzer->GetMaterialIndexBuffer().size());
		m_pTreeCtrl->InsertItem(label, materialInfoLabel);

		label = CString(_T("MaterialLayer.MappingMode : "))
			+ MyFbx::GetLayerMappingModeNameW(pMeshAnalyzer->GetMaterialLayerModeData().MappingMode);
		m_pTreeCtrl->InsertItem(label, parent);
		label = CString(_T("MaterialLayer.ReferenceMode : "))
			+ MyFbx::GetLayerReferenceModeNameW(pMeshAnalyzer->GetMaterialLayerModeData().ReferenceMode);
		m_pTreeCtrl->InsertItem(label, parent);

		// マテリアル詳細情報。
		for (size_t i = 0; i < materialNum; ++i)
		{
			auto material = pMeshAnalyzer->GetMaterialAnalyzer(i);
			if (material.get() == nullptr)
			{
				continue;
			}
			label.Format(_T("Material[%Iu]"), i);
			HTREEITEM materialLabel = m_pTreeCtrl->InsertItem(label, materialInfoLabel);

			// マテリアル名。
			label = _T("MaterialName : ");
			label += material->GetMaterialNameW().c_str();
			m_pTreeCtrl->InsertItem(label, materialLabel);

			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("Ambient"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_Ambient), true,
				material->GetFbxAmbientFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_Ambient),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Ambient),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_Ambient));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("Diffuse"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_Diffuse), true,
				material->GetFbxDiffuseFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_Diffuse),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Diffuse),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_Diffuse));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("Specular"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_Specular), true,
				material->GetFbxSpecularFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_Specular),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Specular),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_Specular));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("Emissive"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_Emissive), true,
				material->GetFbxEmissiveFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_Emissive),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Emissive),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_Emissive));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("Bump"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_Bump), true,
				material->GetFbxBumpFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_Bump),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Bump),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_Bump));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("NormalMap"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_NormalMap), true,
				0, false,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_NormalMap),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_NormalMap),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_NormalMap));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("Reflection"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_Reflection), true,
				material->GetFbxReflectionFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_Reflection),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_Reflection),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_Reflection));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("TransparentColor"),
				material->GetFbxRgbColorRGB(MyFbxMaterialAnalyzer::PropertyType_TransparentColor), true,
				0, false,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_TransparentColor),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_TransparentColor),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_TransparentColor));
			InsertMaterialProperty(m_pTreeCtrl, materialLabel, _T("TransparencyFactor"),
				MyMath::ZERO_VECTOR3F, false,
				material->GetFbxTransparencyFactor(), true,
				material->GetTextureName(MyFbxMaterialAnalyzer::PropertyType_TransparencyFactor),
				material->GetTextureFileName(MyFbxMaterialAnalyzer::PropertyType_TransparencyFactor),
				material->GetTextureRelativeFileName(MyFbxMaterialAnalyzer::PropertyType_TransparencyFactor));

			label.Format(_T("Shininess : %f"), material->GetFbxShininess());
			m_pTreeCtrl->InsertItem(label, materialLabel);
			label.Format(_T("Roughness : %f"), material->GetFbxRoughness());
			m_pTreeCtrl->InsertItem(label, materialLabel);
			label.Format(_T("Opacity : %f"), material->GetFbxOpacity());
			m_pTreeCtrl->InsertItem(label, materialLabel);
			label.Format(_T("Reflectivity : %f"), material->GetFbxReflectivity());
			m_pTreeCtrl->InsertItem(label, materialLabel);
		}
#pragma endregion

		// スキン情報を取得
		HTREEITEM skinLabel = m_pTreeCtrl->InsertItem(_T("Skin"), parent);

		// スキン数
		const size_t skinNum = pMeshAnalyzer->GetSkinAnalyzerCount();
		label.Format(_T("SkinNum : %Iu"), skinNum);
		m_pTreeCtrl->InsertItem(label, skinLabel);

		for (size_t i = 0; i < skinNum; ++i)
		{
			// スキンラベル
			label.Format(_T("Skin[%Iu]"), i);
			HTREEITEM subskinLabel = m_pTreeCtrl->InsertItem(label, skinLabel);

			// ボーン数
			auto pSkinAnalyzer = pMeshAnalyzer->GetSkinAnalyzer(i);
			const size_t boneNum = pSkinAnalyzer->GetBoneSkeletonInfoArray().size();
			label.Format(_T("BoneNum : %Iu"), boneNum);
			m_pTreeCtrl->InsertItem(label, subskinLabel);

			// ボーンごとの情報
			HTREEITEM hBoneRootLabel = m_pTreeCtrl->InsertItem(_T("Bones"), subskinLabel);
			for (size_t b = 0; b < boneNum; ++b)
			{
				auto boneSkeletonInfo = pSkinAnalyzer->GetBoneSkeletonInfoArray().at(b);
				label.Format(_T("%Iu : "), b);
				label += boneSkeletonInfo->GetBoneNameW().c_str();
				HTREEITEM hBoneNodeLabel = m_pTreeCtrl->InsertItem(label, hBoneRootLabel);
			}
			label.Format(_T("BoneInflInfoNum : %Iu"), pSkinAnalyzer->GetBoneInflInfoNum());
			m_pTreeCtrl->InsertItem(label, subskinLabel);
		}
	}

	void MyFbxNodeTree::OnFindFbxNurbsNode(FbxNode* node, FbxNurbs* nurbsNode)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"NURBs").GetString());
	}

	void MyFbxNodeTree::OnFindFbxPatchNode(FbxNode* node, FbxPatch* patch)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Patch").GetString());
	}

	void MyFbxNodeTree::OnFindFbxCameraNode(FbxNode* node, FbxCamera* camera)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Camera").GetString());
	}

	void MyFbxNodeTree::OnFindFbxCameraStereoNode(FbxNode* node, FbxCameraStereo* cameraStereo)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"CameraStereo").GetString());
	}

	void MyFbxNodeTree::OnFindFbxCameraSwitcherNode(FbxNode* node, FbxCameraSwitcher* cameraSwitcher)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"CameraSwitcher").GetString());
	}

	void MyFbxNodeTree::OnFindFbxLightNode(FbxNode* node, FbxLight* lignt)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Light").GetString());
	}

	void MyFbxNodeTree::OnFindFbxOpticalReferenceNode(FbxNode* node, FbxOpticalReference* optRef)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"OpticalReference").GetString());
	}

	void MyFbxNodeTree::OnFindFbxOpticalMarkerNode(FbxNode* node)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"OpticalMarker").GetString());
	}

	void MyFbxNodeTree::OnFindFbxNurbsCurveNode(FbxNode* node, FbxNurbsCurve* nurbsCurve)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"NURBsCurve").GetString());
	}

	void MyFbxNodeTree::OnFindFbxTrimNurbsSurfaceNode(FbxNode* node, FbxTrimNurbsSurface* trimNurbsSurface)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"TrimNURBsSurface").GetString());
	}

	void MyFbxNodeTree::OnFindFbxBoundaryNode(FbxNode* node, FbxBoundary* boundary)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Boundary").GetString());
	}

	void MyFbxNodeTree::OnFindFbxNurbsSurfaceNode(FbxNode* node, FbxNurbsSurface* nurbsSurface)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"NURBsSurface").GetString());
	}

	void MyFbxNodeTree::OnFindFbxShapeNode(FbxNode* node, FbxShape* shape)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Shape").GetString());
	}

	void MyFbxNodeTree::OnFindFbxLodGroupNode(FbxNode* node, FbxLODGroup* lodGroup)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"LODGroup").GetString());
	}

	void MyFbxNodeTree::OnFindFbxSubDivNode(FbxNode* node, FbxSubDiv* subDiv)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"SubDiv").GetString());
	}

	void MyFbxNodeTree::OnFindFbxCachedEffectNode(FbxNode* node, FbxCachedEffect* cachedEffect)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"CachedEffect").GetString());
	}

	void MyFbxNodeTree::OnFindFbxLineNode(FbxNode* node, FbxLine* line)
	{
		this->InsertToLast(GetConnectedNamesString(node, L"Line").GetString());
	}

#if 0
	inline void OutputColorInfo(std::ostream& ofs, const MyMath::Vector3F& color, const char* pName)
	{
		ofs
			<< color.x << "\t"
			<< color.y << "\t"
			<< color.z << "\t"
			<< std::endl
			<< pName << std::endl;
	}
#endif

	void MyFbxNodeTree::OutputDetailInfoToEditCtrl() const
	{
		if (!m_pEditCtrl)
		{
			return;
		}

		//std::ofstream ofs;
		//std::ostringstream ofs;

		// もしログ情報をファイル出力する場合、
		// 日本語を使うならば内部処理にワイド文字を使って、エンコーディングには UTF-8/UTF-16 を使うべき。

		//this->AddStringLineToEditCtrlAndScrollToLastLine(_T("日本語と Unicode のテスト©♡♤"));

		CString temp;

		// アニメーション情報。
		const auto& animStackNames = this->GetAnimTimeInfo().GetAnimStackNamesArray();
		temp.Format(_T("AnimStacks(%Iu):"), animStackNames.size());
		this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
		for (size_t i = 0; i < animStackNames.size(); ++i)
		{
			temp.Format(_T("AnimStack[%Iu] = \"%s\""), i, CString(animStackNames[i].c_str()).GetString());
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
		}
		if (!animStackNames.empty())
		{
			temp.Format(_T("AnimPeriod = %I64d"), this->GetAnimTimeInfo().GetPeriod().Get());
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
		}

		// メッシュ情報。
		const size_t meshNum = GetMeshAnalyzerArray().size();
		temp.Format(_T("Meshes(%Iu):"), meshNum);
		this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
		int totalPolygonCount = 0;
		for (size_t m = 0; m < meshNum; ++m)
		{

			auto mesh = GetMeshAnalyzerArray()[m];

			temp.Format(_T("Mesh[%Iu] = \"%s\""), m, CString(mesh->GetMeshNameW().c_str()).GetString());
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);

			// ポリゴン数やアニメーション フレーム数が多くなるとデータが爆発するので、それらに比例するような情報は出力しない。
			// Win32 エディット コントロールは大量のテキストを扱うと異様に動作が重くなる。
			// リッチ エディットはもう少しマシ。

			const int meshPolygonCount = mesh->GetPolygonCount();
			temp.Format(_T("Mesh Polygons = %d"), meshPolygonCount);
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
			totalPolygonCount += meshPolygonCount;
#if 1
			// 頂点インデックス。
			const auto& idxBuf = mesh->GetIndexBuffer();
			temp.Format(_T("Vertex Indices(%Iu):"), idxBuf.size());
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
#if 0
			for (size_t i = 0; i < idxBuf.size(); ++i)
			{
				temp.Format(_T("\t%d"), idxBuf[i]);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
			}
#endif

			// コントロール ポイント（頂点位置座標）。
			const auto& ctrlPtsBuf = mesh->GetControlPointBuffer();
			temp.Format(_T("Vertex Positions(%Iu):"), ctrlPtsBuf.size());
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
#if 0
			for (size_t i = 0; i < ctrlPtsBuf.size(); ++i)
			{
				const auto vPos = ctrlPtsBuf[i];
				temp.Format(_T("\t%f, %f, %f, %f"), vPos.x, vPos.y, vPos.z, vPos.w);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
			}
#endif

			// 法線ベクトル。
			const auto& normBuf = mesh->GetNormalBuffer();
			temp.Format(_T("Normals(%Iu):"), normBuf.size());
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
#if 0
			for (size_t i = 0; i < normBuf.size(); ++i)
			{
				const auto vNormal = normBuf[i];
				temp.Format(_T("\t%f, %f, %f"), vNormal.x, vNormal.y, vNormal.z);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
			}
#endif

			// テクスチャ UV 座標。
			const size_t uvCount = mesh->GetUVAnalyzerCount();
			for (size_t i = 0; i < uvCount; ++i)
			{
				auto pUVAnalyzer = mesh->GetUVAnalyzer(i);
				const auto& uvArray = pUVAnalyzer->GetDiffuseUVCoordArray();
				temp.Format(_T("DiffuseTexUV[%d](%Iu):"), i, uvArray.size());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
#if 0
				for (size_t u = 0; u < uvArray.size(); ++u)
				{
					const auto vUV = uvArray[u];
					temp.Format(_T("\t%f, %f"), vUV.x, vUV.y);
					this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				}
#endif
			}
#endif

			// マテリアル情報。
			const size_t materialNum = mesh->GetMaterialAnalyzerCount();

			temp.Format(_T("Materials(%Iu):"), materialNum);
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
			for (size_t i = 0; i < materialNum; ++i)
			{
				auto material = mesh->GetMaterialAnalyzer(i);
				temp.Format(_T("Material[%Iu] = \"%s\""), i, CString(material->GetMaterialNameW().c_str()).GetString()); // マテリアル番号と名前。
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
#if 0
				const MyMath::Vector3F& ambient = material->GetFbxRgbAmbient();
				const MyMath::Vector3F& diffuse = material->GetFbxRgbDiffuse();
				const MyMath::Vector3F& specular = material->GetFbxRgbSpecular();
				const MyMath::Vector3F& emissive = material->GetFbxRgbEmissive();
				const MyMath::Vector3F& bump = material->GetFbxRgbBump();
				const float reflectivity = material->GetFbxReflectionFactor();
				const float shininess = material->GetFbxShininess();
				const float transparency = material->GetFbxTransparencyFactor();

				const char* ambientTexName = material->GetTextureName(MyFbx::MyFbxMaterialAnalyzer::PropertyType_Ambient);
				const char* diffuseTexName = material->GetTextureName(MyFbx::MyFbxMaterialAnalyzer::PropertyType_Diffuse);
				const char* specularTexName = material->GetTextureName(MyFbx::MyFbxMaterialAnalyzer::PropertyType_Specular);
				const char* emissiveTexName = material->GetTextureName(MyFbx::MyFbxMaterialAnalyzer::PropertyType_Emissive);
				const char* bumpTexName = material->GetTextureName(MyFbx::MyFbxMaterialAnalyzer::PropertyType_Bump);

				temp.Format(_T("TexName = \"%s\", RGB=(%s)"),
					CString(MyUtils::SafeConvertUtf8toUtf16(ambientTexName).c_str()).GetString(), ConvertVector3ToString(ambient).GetString());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				temp.Format(_T("TexName = \"%s\", RGB=(%s)"),
					CString(MyUtils::SafeConvertUtf8toUtf16(diffuseTexName).c_str()).GetString(), ConvertVector3ToString(diffuse).GetString());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				temp.Format(_T("TexName = \"%s\", RGB=(%s)"),
					CString(MyUtils::SafeConvertUtf8toUtf16(specularTexName).c_str()).GetString(), ConvertVector3ToString(specular).GetString());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				temp.Format(_T("TexName = \"%s\", RGB=(%s)"),
					CString(MyUtils::SafeConvertUtf8toUtf16(emissiveTexName).c_str()).GetString(), ConvertVector3ToString(emissive).GetString());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				temp.Format(_T("TexName = \"%s\", RGB=(%s)"),
					CString(MyUtils::SafeConvertUtf8toUtf16(bumpTexName).c_str()).GetString(), ConvertVector3ToString(bump).GetString());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);

				temp.Format(_T("Reflectivity = %f"), reflectivity);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				temp.Format(_T("Shininess = %f"), shininess);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				temp.Format(_T("Transparency = %f"), transparency);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
#endif
			}

#if 1
			// スキン情報。
			const size_t skinNum = mesh->GetSkinAnalyzerCount();
			temp.Format(_T("Skins(%Iu):"), skinNum);
			this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
			for (size_t i = 0; i < skinNum; ++i)
			{
				auto skin = mesh->GetSkinAnalyzer(i);
				temp.Format(_T("Skin[%Iu] = \"%s\""), i, CString(skin->GetSkinNameW().c_str()).GetString());
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);

				// ボーン影響度情報。
				const size_t boneInflInfoNum = skin->GetBoneInflInfoNum();
				temp.Format(_T("BoneInfluenceInfos(%Iu):"), boneInflInfoNum);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				for (size_t j = 0; j < boneInflInfoNum; ++j)
				{
					auto boneInflInfo = skin->GetBoneInflInfo(static_cast<int>(j));
					if (boneInflInfo && boneInflInfo->size() > MyVertexTypes::MaxBoneNumPerVertex)
					{
						temp.Format(_T("BoneInfluenceInfo[%Iu]: OverCapacity(%Iu)"), j, boneInflInfo->size());
						this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
					}
				}

				// ボーン スケルトン情報。
				const size_t boneSkeletonInfoNum = skin->GetBoneSkeletonInfoArray().size();
				temp.Format(_T("BoneSkeletonInfos(%Iu):"), boneSkeletonInfoNum);
				this->AddStringLineToEditCtrlAndScrollToLastLine(temp);

				for (size_t b = 0; b < boneSkeletonInfoNum; ++b)
				{
					auto boneSkeletonInfo = skin->GetBoneSkeletonInfoArray().at(b);

					temp.Format(_T("BoneSkeletonInfo[%Iu]:"), b);
					this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
					temp.Format(_T("BoneName = \"%s\""),
						CString(boneSkeletonInfo->GetBoneNameW().c_str()).GetString());
					this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
				}
			}
#endif
		} // end of mesh loop

		temp.Format(_T("Total Polygons = %d"), totalPolygonCount);
		this->AddStringLineToEditCtrlAndScrollToLastLine(temp);

		temp.Format(_T("Hierarchy Level = %d"), this->GetHierarchyLevel());
		this->AddStringLineToEditCtrlAndScrollToLastLine(temp);
	}

} // end of namespace
