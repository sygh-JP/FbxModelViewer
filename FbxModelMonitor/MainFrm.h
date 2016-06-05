// この MFC サンプル ソース コードでは、MFC Microsoft Office Fluent ユーザー インターフェイス 
// ("Fluent UI") の使用方法を示します。このコードは、MFC C++ ライブラリ ソフトウェアに 
// 同梱されている Microsoft Foundation Class リファレンスおよび関連電子ドキュメントを
// 補完するための参考資料として提供されます。
// Fluent UI を複製、使用、または配布するためのライセンス条項は個別に用意されています。
// Fluent UI ライセンス プログラムの詳細については、Web サイト
// http://msdn.microsoft.com/officeui を参照してください。
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// MainFrm.h : CMainFrame クラスのインターフェイス
//

#pragma once


#include "MyD3D.h"
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
#include "MyOGL.h"
#endif
#include "MyFbx.h"
#include "FileView.h"
#include "ClassView.h"
#include "OutputWnd.h"
#include "PropertiesWnd.h"
#include "MySplitterWnd.h"
#include "MyDWriteWrapper.hpp"
#include "FbxNodeAnalyzerBase.h"

#include "../MyWpfGraphLibMfc/Wrappers/GradientEditorWrapper.hpp"


class CMainFrame : public CFrameWndEx
{

protected: // シリアル化からのみ作成します。
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

	// 属性
protected:

public:
	static CMainFrame* GetTheMainFrame()
	{ return dynamic_cast<CMainFrame*>(AfxGetMainWnd()); }

private:
	MyFbx::TFbxSdkManagerPtr m_pFbxManager;
	// スマートポインタで管理するオブジェクトは MFC アプリケーション スレッド クラスのコンポジションにしてはいけない。
	// アプリケーション スレッド クラスのインスタンスはグローバル変数として生成されるため、
	// デストラクタで自動的に delete されるようにしておくと、
	// 終了時に CRT 解放のタイミングが原因と思われるランタイム エラーが発生する。

	// HACK: FBX および D3D 関連のオブジェクトをそれぞれ集めた管理クラスを別途作成して、そのインスタンスをコンポジションするようにする。

	BOOL m_isSplitterCreated = false;
	CMySplitterWnd m_wndSplitter;
	CWnd* m_pWndHostD3D = nullptr;
	MyD3D::MyD3DManager* m_pD3DManager = nullptr;
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	CWnd* m_pWndHostOGL = nullptr;
	MyOGL::MyOGLManager* m_pOGLManager = nullptr;
#endif
	// TODO: 異なる視点から同じシーンをレンダリングするなどのマルチビューを実装する場合、
	// ウィンドウごとにスワップチェーンを作成して個別にレンダリングするのではなく、
	// グラフィックス API 側でビューポートを分けて、視点の異なるシーンをレンダリングするようにしたほうが効率的。

public:
#if 0
	CSplitterWnd& GetViewSplitterWnd()
	{ return m_wndSplitter; }
#endif
	void InvalidateAllViews()
	{
		const int rowCount = m_wndSplitter.GetRowCount();
		const int columnCount = m_wndSplitter.GetColumnCount();
		for (int r = 0; r < rowCount; ++r)
		{
			for (int c = 0; c < columnCount; ++c)
			{
				m_wndSplitter.GetPane(r, c)->Invalidate(false);
			}
		}
	}

private:
	MyCommon::TMyModelMeshDetailInfoPtrsArray m_modelMeshInfoArray;
	MyCommon::MyAnimTrackInfoTable m_animTrackInfoTable;
	MyCommon::TMyAnimEndCallbackFuncsArray m_animEndCallbackFuncsArray; // シーンに複数のアニメーション オブジェクトを描画する場合、複数生成する必要がある。
	MyCommon::TMyAnimMixerPtrsArrayPtrsArray m_animMixerArrayArray; // シーンに複数のアニメーション オブジェクトを描画する場合、複数生成する必要がある。
	bool m_isCurrentAnimEnded = false;
	bool m_isLoadingResourcesNow = false;

private:
	MyTextureHelper::FontTextureDataPack m_hudFontTexData;
	MyTextureHelper::TextureDataPack m_toonShadingDiffuseCoefRefTexData;

private:
	MyMath::MyGlobalMaterialTable m_globalMaterialTable;

public:
	const MyMath::TMyMaterialPtrsArray& GetGlobalMaterialsArray() const { return m_globalMaterialTable.GetGlobalMaterialsArray(); }
	const MyMath::MyMaterial* GetCurrentTargetMaterial() const { return m_globalMaterialTable.GetCurrentTargetMaterial(); }
	MyMath::MyMaterial* GetCurrentTargetMaterial() { return m_globalMaterialTable.GetCurrentTargetMaterial(); }
	void SetIsLoadingResourcesNow(bool isLoading) { m_isLoadingResourcesNow = isLoading; }

public:
#if 0
	void BindTargetMaterial(MyMath::MyMaterial::TSharedPtr targetMaterial)
	{ m_wndProperties.BindTargetMaterial(targetMaterial); }

	void UnbindTargetMaterial()
	{ m_wndProperties.UnbindTargetMaterial(); }
#endif
	void SetTargetMaterialIndex(int index)
	{
		m_globalMaterialTable.SetTargetMaterialIndex(index);
		const auto* currMat = m_globalMaterialTable.GetCurrentTargetMaterial();
		if (currMat)
		{
			m_wndProperties.BindTargetMaterial(*currMat);
		}
		else
		{
			m_wndProperties.UnbindTargetMaterial();
		}
	}
	const MyMath::MyMaterial* GetGlobalMaterialByIndex(int index) const
	{
		return m_globalMaterialTable.GetGlobalMaterialByIndex(index);
	}

private:
	void CreateGlobalMaterialsArray(const MyMath::TMyNameToMaterialTable& nameToMaterialTable)
	{
		m_globalMaterialTable.CreateGlobalMaterialsArray(nameToMaterialTable);

	}

	void ClearGlobalMaterialsArray()
	{
		m_globalMaterialTable.ClearGlobalMaterialsArray();
	}

private:
	void UpdateMaterialViews()
	{
		m_wndClassView.UpdateListItemCount();
		m_wndProperties.UnbindTargetMaterial();
	}

private:
	bool CreateFontTextureDibs();
	void CreateToonShadingDiffuseCoefRefTextureDibs(const std::vector<MyMath::TMyGradientColorStopArray>& toonGradientColorStopsArray);
private:
	static CSize GetSafeRenderingHostWndClientRectSize(const CWnd* pHostWnd)
	{
		_ASSERTE(pHostWnd != nullptr);
		CRect clrect;
		pHostWnd->GetClientRect(&clrect);
		// ゼロ以下や、ハードウェア性能を超えるようなあまりに大きい値は却下。
		// ちなみに Direct3D 10.x をサポートするハードウェアの最大テクスチャ サイズ（Max Texture Dimension）は D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION = 8192 であり、
		// Direct3D 11.x をサポートするハードウェアの最大テクスチャ サイズ（Max Texture Dimension）は D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION = 16384 となっている。
		// グラフィックス カードによっては、Direct3D API と OpenGL API のサポート バージョン（世代）に違いがあるものもあるが、
		// その場合リソースの制約はやはり API バージョンに左右される？
		// OpenGL の場合、UBO などのリソースの上限・下限は API バージョンによって明確に決まっていない模様。
		// 要するにハードウェア依存。MSAA レベルに関しても、Direct3D 10.1 以降のような明確な下限はないらしい。
		// http://wlog.flatlib.jp/item/1634
		return CSize(
			MyUtil::Clamp<int>(clrect.Width(), 1, 4096),
			MyUtil::Clamp<int>(clrect.Height(), 1, 4096));
	}
	CSize GetSafeDirect3DHostWndClientRectSize() const
	{
		_ASSERTE(m_pWndHostD3D != nullptr);
		return GetSafeRenderingHostWndClientRectSize(m_pWndHostD3D);
	}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	CSize GetSafeOpenGLHostWndClientRectSize() const
	{
		_ASSERTE(m_pWndHostOGL != nullptr);
		return GetSafeRenderingHostWndClientRectSize(m_pWndHostOGL);
	}
#endif

	// HACK: 下記はすべてインターフェイスの抽象メソッドにして共通化するべき。
	// もしくはパラメータ インスタンスを共通化して、D3D/GL レンダラーにはポインタ経由で共有させるべき。
public:
	void GetRotationAmount(MyMath::Vector3F* pAmount) const
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->GetRotationAmount(pAmount);
		}
		else
		{
			ATLASSERT(false);
		}
	}

	void SetRotationAmount(const MyMath::Vector3F* pAmount)
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->SetRotationAmount(pAmount);
		}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (m_pOGLManager)
		{
			m_pOGLManager->SetRotationAmount(pAmount);
		}
#endif
	}


	void GetCameraEye(MyMath::Vector3F* pEyePos) const
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->GetCameraEye(pEyePos);
		}
		else
		{
			ATLASSERT(false);
		}
	}

	void SetCameraEye(const MyMath::Vector3F* pEyePos)
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->SetCameraEye(pEyePos);
		}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (m_pOGLManager)
		{
			m_pOGLManager->SetCameraEye(pEyePos);
		}
#endif
	}


	void GetMainLightRotationAmount(MyMath::Vector3F* pAmount) const
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->GetMainLightRotationAmount(pAmount);
		}
		else
		{
			ATLASSERT(false);
		}
	}

	void SetMainLightRotationAmount(const MyMath::Vector3F* pAmount)
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->SetMainLightRotationAmount(pAmount);
		}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (m_pOGLManager)
		{
			m_pOGLManager->SetMainLightRotationAmount(pAmount);
		}
#endif
	}

	void PanCamera(MyMath::Vector2F shiftInPix)
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->PanCamera(shiftInPix);
		}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (m_pOGLManager)
		{
			m_pOGLManager->PanCamera(shiftInPix);
		}
#endif
	}

private:
	bool GetDisplaysCoordAxes() const
	{
		if (m_pD3DManager)
		{
			return m_pD3DManager->GetCommonSettings().DisplaysCoordAxes;
		}
		else
		{
			ATLASSERT(false);
			return false;
		}
	}

	void SetDisplaysCoordAxes(bool displays)
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->GetCommonSettings().DisplaysCoordAxes = displays;
		}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (m_pOGLManager)
		{
			m_pOGLManager->GetCommonSettings().DisplaysCoordAxes = displays;
		}
#endif
	}

	void SetBackColor(const MyMath::Vector4F& colorVal)
	{
		if (m_pD3DManager)
		{
			m_pD3DManager->GetCommonSettings().BackColor = colorVal;
		}
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (m_pOGLManager)
		{
			m_pOGLManager->GetCommonSettings().BackColor = colorVal;
		}
#endif
	}

	// 操作
public:
	bool GetSplitterWndClientSize(CSize& size) const;
	bool UpdateMySplitterWndRowColumnSize(int row, int col, int cxIdeal, int cyIdeal, int cxMin, int cyMin);
	bool RecalcSplitterWndLayout();

	bool InitDirect3DAndOpenGL();

	bool RenderMyScene(bool advancesFrame, CWnd* pHostWnd = nullptr);
	bool ResizeMyView(CWnd* pHostWnd = nullptr);

	MyFbx::MyFbxNodeAnalyzerBase::TSharedPtr LoadFbxFile(CStringW strFilePath);
	bool CreateDeviceMeshFromFbx(CStringW strFilePath, const MyFbx::MyFbxNodeAnalyzerBase& nodeAnalyzer);

	void ClearMeshTree();

	// オーバーライド
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	// 実装
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:  // コントロール バー用メンバ
	CMFCRibbonBar     m_wndRibbonBar;
	CMFCRibbonApplicationButton m_MainButton;
	CMFCToolBarImages m_PanelImages;
	CMFCRibbonStatusBar  m_wndStatusBar;
	CFileView         m_wndFileView;
	CClassView        m_wndClassView;
	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;
	CMFCCaptionBar    m_wndCaptionBar;

private:
	CFont m_ribbonFont;

	// Windows メッセージのタイマーは精度も悪く、フレーム スキップもできないが、簡単のため。
private:
	static const UINT_PTR RenderingTimerID = 1;
	static const UINT_PTR ResizeCheckTimerID = 2;
	static const UINT FrameRate = 60;
	static const UINT RenderingPeriodPerFrameMs = 1000 / FrameRate;
	static const UINT ResizeCheckPeriodMs = 1000;

	UINT_PTR m_renderingTimerID = 0;
	UINT_PTR m_resizeCheckTimerID = 0;

private:
	template<typename T> T* GetRibbonInnerControl(UINT id)
	{
		static_assert((std::is_base_of<CMFCRibbonBaseElement, T>::value), "No inheritance relationship!!");
		return dynamic_cast<T*>(m_wndRibbonBar.FindByID(id));
	}
	template<typename T> const T* GetRibbonInnerControl(UINT id) const
	{
		static_assert((std::is_base_of<CMFCRibbonBaseElement, T>::value), "No inheritance relationship!!");
		return dynamic_cast<T*>(m_wndRibbonBar.FindByID(id));
	}

	CMFCRibbonCheckBox* GetRibbonCheckBox(UINT id)
	{ return this->GetRibbonInnerControl<CMFCRibbonCheckBox>(id); }
	const CMFCRibbonCheckBox* GetRibbonCheckBox(UINT id) const
	{ return this->GetRibbonInnerControl<CMFCRibbonCheckBox>(id); }

	CMFCRibbonColorButton* GetRibbonColorButton(UINT id)
	{ return this->GetRibbonInnerControl<CMFCRibbonColorButton>(id); }
	const CMFCRibbonColorButton* GetRibbonColorButton(UINT id) const
	{ return this->GetRibbonInnerControl<CMFCRibbonColorButton>(id); }

	CMFCRibbonComboBox* GetRibbonComboBox(UINT id)
	{ return this->GetRibbonInnerControl<CMFCRibbonComboBox>(id); }
	const CMFCRibbonComboBox* GetRibbonComboBox(UINT id) const
	{ return this->GetRibbonInnerControl<CMFCRibbonComboBox>(id); }

public:
	bool GetIsTimerValid() const { return m_renderingTimerID != 0; }

private:
	void CreateAnimTrackComboBoxItems();
	void ClearAnimTrackComboBoxItems();

private:
	int GetRibbonComboBoxSelectedIndex(UINT id) const
	{
		const auto* pCombo = this->GetRibbonComboBox(id);
		ATLASSERT(pCombo);
		return pCombo->GetCurSel();
	}

	// 生成された、メッセージ割り当て関数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnViewCaptionBar();
	afx_msg void OnUpdateViewCaptionBar(CCmdUI* pCmdUI);
	afx_msg void OnOptions();

	afx_msg void OnCheckClassPane();
	afx_msg void OnUpdateCheckClassPane(CCmdUI* pCmdUI);
	afx_msg void OnCheckFilePane();
	afx_msg void OnUpdateCheckFilePane(CCmdUI* pCmdUI);
	afx_msg void OnCheckOutputPane();
	afx_msg void OnUpdateCheckOutputPane(CCmdUI* pCmdUI);
	afx_msg void OnCheckPropertiesPane();
	afx_msg void OnUpdateCheckPropertiesPane(CCmdUI* pCmdUI);

	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

	void InitializeRibbon();
	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
	BOOL CreateCaptionBar();
private:
	afx_msg LRESULT OnUdwmInvokeSimpleDelegateByUIThread(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnMyButtonAnimPlay();
	afx_msg void OnMyButtonAnimStop();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCheckShowWaveFront();
	afx_msg void OnUpdateCheckShowWaveFront(CCmdUI *pCmdUI);
	afx_msg void OnCheckImageBasedFur();
	afx_msg void OnUpdateCheckImageBasedFur(CCmdUI *pCmdUI);
	afx_msg void OnCheckBloomEffect();
	afx_msg void OnUpdateCheckBloomEffect(CCmdUI *pCmdUI);
	afx_msg void OnCheckToonShading();
	afx_msg void OnUpdateCheckToonShading(CCmdUI *pCmdUI);
	afx_msg void OnCheckToonInk();
	afx_msg void OnUpdateCheckToonInk(CCmdUI *pCmdUI);
	afx_msg void OnCheckShowCoordAxes();
	afx_msg void OnUpdateCheckShowCoordAxes(CCmdUI *pCmdUI);
	afx_msg void OnMyButtonBackColor();
	afx_msg void OnMyButtonFireProjectile();
	afx_msg void OnMyButtonLoadEnvMapImg();
	afx_msg void OnMyComboAnimTrack();
};


