#include "stdafx.h"
#include "MyD3D.h"
#include "MyUtil.h"
#include "MyCustomVertexTypes.hpp"
#include "MyHLSL/CommonDefs.hlsli"
#include "MyStopwatch.hpp"
#include "CustomTrace.hpp"
#include "MyAppCommonConsts.hpp"
using namespace MyAppCommonConsts;

#include "DebugNew.h"

#include <d3dcompiler.inl>
//#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// TODO: Direct3D 11.0 をサポートしない場合はリソースの作成を中止し、画面の塗りつぶしだけを行なうようにする。


//#define USE_GDI_COMPATIBLE

#define USE_D2D


namespace
{
#ifdef USE_GDI_COMPATIBLE
	const DXGI_FORMAT renderTargetTexFormat = DXGI_FORMAT_B8G8R8A8_UNORM; // GDI 相互運用。通例バック バッファにも使われる。
	// GDI や Direct2D とともに Direct3D 相互運用を行なう場合は、フォーマットに制約がある。
#else
	const DXGI_FORMAT renderTargetTexFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // FP16。
#endif
}

namespace MyD3D
{

	static bool CreateRWStructuredBuffer(ID3D11Device* pDevice, uint32_t elementSize, uint32_t elementCount, _In_opt_ const void* pInitData, Microsoft::WRL::ComPtr<ID3D11Buffer>& pBufOut)
	{
		_ASSERTE(pDevice != nullptr);
		_ASSERTE(pBufOut == nullptr);

		HRESULT hr = E_FAIL;

		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		bufDesc.ByteWidth = elementSize * elementCount;
		bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufDesc.StructureByteStride = elementSize;
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		// D3D11_USAGE_DEFAULT を指定したバッファは初期化時以外に CPU から RW アクセスすることができないが、RW バッファとして使うには必須。

		if (pInitData)
		{
			D3D11_SUBRESOURCE_DATA resData = {};
			resData.pSysMem = pInitData;
			hr = pDevice->CreateBuffer(&bufDesc, &resData, pBufOut.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = pDevice->CreateBuffer(&bufDesc, nullptr, pBufOut.ReleaseAndGetAddressOf());
		}
		return SUCCEEDED(hr);
	}

	static bool CreateRWByteAddressBuffer(ID3D11Device* pDevice, uint32_t elementSize, uint32_t elementCount, _In_opt_ const void* pInitData, D3D11_BIND_FLAG optFlag, bool enablesIndirectArgs, Microsoft::WRL::ComPtr<ID3D11Buffer>& pBufOut)
	{
		_ASSERTE(pDevice != nullptr);
		_ASSERTE(pBufOut == nullptr);

		HRESULT hr = E_FAIL;

		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.BindFlags = optFlag | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		bufDesc.ByteWidth = elementSize * elementCount;
		bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		if (enablesIndirectArgs)
		{
			// ID3D11DeviceContext::DispatchIndirect()
			// ID3D11DeviceContext::DrawInstancedIndirect()
			// ID3D11DeviceContext::DrawIndexedInstancedIndirect()
			// の引数に指定するバッファとして作成する場合は必須。
			bufDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		}
		bufDesc.Usage = D3D11_USAGE_DEFAULT;

		if (pInitData)
		{
			D3D11_SUBRESOURCE_DATA resData = {};
			resData.pSysMem = pInitData;
			hr = pDevice->CreateBuffer(&bufDesc, &resData, pBufOut.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = pDevice->CreateBuffer(&bufDesc, nullptr, pBufOut.ReleaseAndGetAddressOf());
		}
		return SUCCEEDED(hr);
	}

	//! @brief  H/W が持っている、最高品質の MSAA プロパティを取得する。<br>
	static bool GetBestMSAAProperties(ID3D11Device* pDevice, DXGI_FORMAT format,
		_In_opt_ const UINT pCandSampleCounts[], UINT candSampleCounts,
		_Out_ UINT& msaaSampleCount, _Out_ UINT& msaaNumQualityLevels, _Out_ UINT& msaaQuality)
	{
		HRESULT hr = E_FAIL;

		msaaSampleCount = 1;
		msaaNumQualityLevels = 0;
		msaaQuality = 0;

		_ASSERTE(pDevice);

		if (candSampleCounts == 0)
		{
			for (UINT i = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i >= 1; --i)
			{
				// 1つずつデクリメントするのではなく、2で割っていったほうがよいかも。開始値が2のべき乗であることが前提だが……
				const UINT tempSampleCount = i;
				UINT tempNumQualityLevels = 0;
				hr = pDevice->CheckMultisampleQualityLevels(format, tempSampleCount, &tempNumQualityLevels);
				if (SUCCEEDED(hr) && tempNumQualityLevels != 0)
				{
					msaaSampleCount = tempSampleCount;
					msaaNumQualityLevels = tempNumQualityLevels;
					break;
				}
			}
		}
		else
		{
			_ASSERTE(pCandSampleCounts);
			// pCandSampleCounts に降順で候補サンプル数値が格納されていることが前提。
			for (UINT i = 0; i < candSampleCounts; ++i)
			{
				const UINT tempSampleCount = pCandSampleCounts[i];
				UINT tempNumQualityLevels = 0;
				hr = pDevice->CheckMultisampleQualityLevels(format, tempSampleCount, &tempNumQualityLevels);
				if (SUCCEEDED(hr) && tempNumQualityLevels != 0)
				{
					msaaSampleCount = tempSampleCount;
					msaaNumQualityLevels = tempNumQualityLevels;
					break;
				}
			}
		}

		msaaQuality = (msaaNumQualityLevels > 0) ? (msaaNumQualityLevels - 1) : 0;

		return true;
	}



	//! @brief  レンダーステートのセットアップ。<br>
	bool MyD3DManager::SetupRenderState()
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		D3D11_RASTERIZER_DESC rasterizerState = {};
		rasterizerState.FillMode = D3D11_FILL_SOLID;
		rasterizerState.CullMode = D3D11_CULL_BACK;
		//rasterizerState.CullMode = D3D11_CULL_NONE; // デバッグするとき便利。
		//rasterizerState.CullMode = D3D11_CULL_FRONT;
#ifdef USE_LEFT_HAND_COORD_SYS
		rasterizerState.FrontCounterClockwise = false;
#else
		rasterizerState.FrontCounterClockwise = true; // true で右手系。false で左手系。
#endif
		rasterizerState.DepthBias = 0;
		rasterizerState.DepthBiasClamp = 0;
		rasterizerState.SlopeScaledDepthBias = 0;
		rasterizerState.DepthClipEnable = true;
		rasterizerState.ScissorEnable = false;
		rasterizerState.MultisampleEnable = true; // MSAA 有効化。
		rasterizerState.AntialiasedLineEnable = false;

		hr = m_pD3DDevice->CreateRasterizerState(&rasterizerState, m_pRasterStateSolidMsaa.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create rasterizer state!!"), hr);
			return false;
		}

		rasterizerState.FillMode = D3D11_FILL_WIREFRAME;

		hr = m_pD3DDevice->CreateRasterizerState(&rasterizerState, m_pRasterStateWireMsaa.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create rasterizer state!!"), hr);
			return false;
		}

		rasterizerState.FillMode = D3D11_FILL_SOLID;
		//rasterizerState.CullMode = D3D11_CULL_NONE;
		// Setting the slope scale depth bias greatly decreases surface acne and incorrect self shadowing.
		// DirectX SDK サンプル VarianceShadows11 では 1.0 のバイアスが指定されてあったが、
		// PCF 実装である CascadedShadowMaps11 の名残か？　バリアンス シャドウではバイアス不要のはず。
		// なお、D3D11_CULL_NONE が指定されていたのは、ライトに背を向けている板ポリゴンに遮蔽されうる部分の影のため？
		//rasterizerState.SlopeScaledDepthBias = 1.0f;
		//rasterizerState.SlopeScaledDepthBias = 0.00001f;
		rasterizerState.MultisampleEnable = false;

		hr = m_pD3DDevice->CreateRasterizerState(&rasterizerState, m_pRasterStateShadow.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create rasterizer state!!"), hr);
			return false;
		}

		// HACK: 深度ステンシル ステートは現在エフェクト ファイルで管理しているが、
		// 将来的にエフェクト ファイル（.fx）の廃止に備えて、C++ コードで作成して、std::wstring をキーとするオブジェクト テーブルで管理する。
		// テーブルで管理するようにすれば、独自エフェクト ファイルにて名前で参照する仕組みが作れる。

		return true;
	}

	bool MyD3DManager::CreateEffect(const void* pBytecode, size_t bytecodeLength, _Out_ Microsoft::WRL::ComPtr<ID3DX11Effect>& pEffect)
	{
		_ASSERTE(m_pD3DDevice);
		// コンパイル済みエフェクトの BLOB を渡す。エフェクト ファイルのソースコードではないので注意。
		HRESULT hr = D3DX11CreateEffectFromMemory(pBytecode, bytecodeLength,
			0, // FXFlags パラメータは、常にゼロしか指定できないらしい（D3DX11_EFFECT_RUNTIME_VALID_FLAGS）。
			m_pD3DDevice.Get(),
			pEffect.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("D3DX11CreateEffectFromMemory() Failed!!"), hr);
			return false;
		}
		DXTRACE_MSG(_T("D3DX11CreateEffectFromMemory() Succeeded."));
		return true;
	}

	bool MyD3DManager::CreateInputVertexLayout(const D3D11_INPUT_ELEMENT_DESC pElementDescArray[], UINT numElements, const void* pBytecode, size_t bytecodeLength, _Out_ Microsoft::WRL::ComPtr<ID3D11InputLayout>& pLayout)
	{
		HRESULT hr = E_FAIL;
		_ASSERTE(m_pD3DDevice);

		hr = m_pD3DDevice->CreateInputLayout(
			pElementDescArray, numElements,
			pBytecode, bytecodeLength,
			pLayout.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create input layout!!"), hr);
			return false;
		}

		return true;
	}

#if 0
	//! @brief  P4CT 入力頂点レイアウトを定義（作成）する。<br>
	bool MyD3DManager::CreateInputVertexLayoutP4CT()
	{
		_ASSERTE(m_pMainEffect != nullptr);

		HRESULT hr = E_FAIL;

		_ASSERTE(m_effectTechs.TechFont != nullptr);

		// エフェクトのリフレクションが使える場合。
		// もしリフレクションを切っていたら頂点シェーダーのバイトコードは取得できない。
		D3D_PASS_DESC passDesc = {};
		hr = m_effectTechs.TechFont->GetPassByIndex(0)->GetDesc(&passDesc);

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to get pass desc of effect tech!!"), hr);
			return false;
		}

		hr = m_pD3DDevice->CreateInputLayout(
			c_layoutDescArrayP4CT,
			c_layoutElemCountP4CT,
			passDesc.pIAInputSignature,
			passDesc.IAInputSignatureSize,
			&m_pLayoutP4CT);

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create input layout!!"), hr);
			return false;
		}

		return true;
	}
#endif


	//! @brief  レンダーターゲット カラーテクスチャおよびそのビューを作成する。<br>
	//! スクリーン サイズに左右されるため、メイン ウィンドウのリサイズを行なうときには作り直す必要がある。<br>
	bool MyD3DManager::CreateRenderTargetColorTexAndView(DXGI_FORMAT format, UINT width, UINT height, UINT msaaSampleCount, UINT msaaQuality)
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		// UNDONE: エラー発生時に途中 return すると、RTV や SRV が不正なまま（作成した Texture と食い違う状態）になってしまう。
		// ロールバックできるようにするか、RTV や SRV を先にいったん解放しておく必要がある。

		HRESULT hr = E_FAIL;

#pragma region // レンダーターゲット カラーテクスチャ（MSAA ターシャリ バッファ）を作成する。//

		D3D11_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.MipLevels = 1;
		description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		// レンダーターゲットであり、シェーダーのリソースでもある。
		description.Format = format;
		description.Width = width;
		description.Height = height;
		description.SampleDesc.Count = msaaSampleCount;
		description.SampleDesc.Quality = msaaQuality;
#ifdef USE_GDI_COMPATIBLE
		// GDI Layered Window と連携させるならば、GDI 互換のリソースとして作成する。
		// ターシャリ バッファからレイヤード ウィンドウ（のバックバッファ）へ転送するようにすれば不要。
		// ちなみにレイヤード ウィンドウを直接スワップ チェーンにできるのか？
		description.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
#endif

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pSubColorBufferTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

#pragma endregion

#pragma region // レンダーターゲット カラーテクスチャ（MSAA ファーマップ）を作成する。//

		// ファーマップは本来マルチサンプリングしないでもいいが、マルチレンダーターゲットを使う場合は揃える必要がある。
		description.SampleDesc.Count = msaaSampleCount;
		description.SampleDesc.Quality = msaaQuality;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pMSAAFurGeoMapTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

#pragma endregion

#pragma region // レンダーターゲット カラーテクスチャ（ファーマップ）を作成する。//

		// 頂点シェーダーおよびジオメトリ シェーダーでは MSAA テクスチャを扱えないので、
		// VTF する場合は非 MSAA テクスチャにいったん転送する必要がある。
		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pFurGeoMapTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

#pragma endregion

#pragma region // レンダーターゲット カラーテクスチャ（MSAA 法線・深度マップ）を作成する。//

		// 法線マップはマルチサンプリングしないでもいいが、マルチレンダーターゲットを使う場合は揃える必要がある。
		description.SampleDesc.Count = msaaSampleCount;
		description.SampleDesc.Quality = msaaQuality;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pMSAANormalDepthMapTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

#pragma endregion

#pragma region // レンダーターゲット カラーテクスチャ（法線・深度マップ）を作成する。//

		// 法線・深度マップから輪郭線抽出のアルゴリズムを実行する際、MSAA テクスチャのままだと不都合なので、
		// いったん非 MSAA テクスチャに転送する。
		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pNormalDepthMapTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

#pragma endregion

		// UNDONE: ダウンサンプル ワーク テクスチャもスクリーン サイズに応じてサイズ変更するかどうかは TBD。
		// AAA ゲームのようにオプションでクオリティを設定できるようにするならば、専用のメソッドを用意しておいたほうがよい。

#pragma region // レンダーターゲット カラーテクスチャ（ダウンサンプル）を作成する。//

		// 水平・垂直ぼかし用のワーク領域として2つ用意する。
		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;

		description.Width = DOWN_SAMPLED_TEX_SIZE;
		description.Height = DOWN_SAMPLED_TEX_SIZE;
		description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		// コンピュート シェーダーでぼかしを実行するため UAV を作り、
		// RWTexture2D として使用できるようにする。
		// ピクセル シェーダーとのパフォーマンス比較のため、RTV も作る。
		// ピクセル シェーダーを使わないならばレンダーターゲットにする必要はない。

		for (int i = 0; i < 2; ++i)
		{
			hr = m_pD3DDevice->CreateTexture2D(
				&description,
				nullptr, // no initial data
				m_ppDownSampledTempWorkTex[i].ReleaseAndGetAddressOf());

			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
				return false;
			}
		}
#pragma endregion

		// UNDONE: ダウンサンプリングした後でトーン マッピングを行なう場合、
		// リダクション用のバッファのサイズはスクリーン サイズではなくダウンサンプル テクスチャに合わせる。

#pragma region // 並列リダクション用の 2D/1D ワーク テクスチャを作成する。//

		// 最初のリダクションで半分あるいはそれ以下になる。
		description.Format = DXGI_FORMAT_R32_FLOAT;
		description.Width = COMPUTING_TEMP_WORK_SIZE / 2;
		description.Height = COMPUTING_TEMP_WORK_SIZE / 2;
		description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		for (int i = 0; i < 2; ++i)
		{
			hr = m_pD3DDevice->CreateTexture2D(
				&description,
				nullptr, // no initial data
				m_ppPSReductionWorkTex[i].ReleaseAndGetAddressOf());

			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
				return false;
			}
		}

		D3D11_TEXTURE1D_DESC desc1d = {};
		desc1d.ArraySize = 1;
		desc1d.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc1d.Format = DXGI_FORMAT_R32_FLOAT;
		desc1d.Width = (COMPUTING_TEMP_WORK_SIZE / MY_CS_REDUCTION_TILE_SIZE_X) * (COMPUTING_TEMP_WORK_SIZE / MY_CS_REDUCTION_TILE_SIZE_Y);
		desc1d.MipLevels = 1;

		for (int i = 0; i < 2; ++i)
		{
			hr = m_pD3DDevice->CreateTexture1D(
				&desc1d,
				nullptr, // no initial data
				m_ppCSReductionWorkTex[i].ReleaseAndGetAddressOf());

			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create Texture1D!!"), hr);
				return false;
			}
		}

#pragma endregion

		// レンダーターゲット ビューを作成する。

		if (!this->CreateMyRenderTargetView(m_pSubColorBufferTex.Get(), m_pSubColorBufferRTV))
		{
			return false;
		}

		if (!this->CreateMyRenderTargetView(m_pMSAAFurGeoMapTex.Get(), m_pMSAAFurGeoMapRTV))
		{
			return false;
		}

		if (!this->CreateMyRenderTargetView(m_pFurGeoMapTex.Get(), m_pFurGeoMapRTV))
		{
			return false;
		}

		if (!this->CreateMyRenderTargetView(m_pMSAANormalDepthMapTex.Get(), m_pMSAANormalDepthMapRTV))
		{
			return false;
		}

		if (!this->CreateMyRenderTargetView(m_pNormalDepthMapTex.Get(), m_pNormalDepthMapRTV))
		{
			return false;
		}

		for (int i = 0; i < 2; ++i)
		{
			if (!this->CreateMyRenderTargetView(m_ppDownSampledTempWorkTex[i].Get(), m_ppDownSampledTempWorkRTV[i]))
			{
				return false;
			}

			if (!this->CreateMyRenderTargetView(m_ppPSReductionWorkTex[i].Get(), m_ppPSReductionWorkRTV[i]))
			{
				return false;
			}
		}

		// シェーダーリソース ビューを作成する。

		if (!this->CreateMyShaderResourceView(m_pSubColorBufferTex.Get(), m_pSubColorBufferSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pMSAAFurGeoMapTex.Get(), m_pMSAAFurGeoMapSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pFurGeoMapTex.Get(), m_pFurGeoMapSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pMSAANormalDepthMapTex.Get(), m_pMSAANormalDepthMapSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pNormalDepthMapTex.Get(), m_pNormalDepthMapSRV))
		{
			return false;
		}

		for (int i = 0; i < 2; ++i)
		{
			if (!this->CreateMyShaderResourceView(m_ppDownSampledTempWorkTex[i].Get(), m_ppDownSampledTempWorkSRV[i]))
			{
				return false;
			}

			if (!this->CreateMyShaderResourceView(m_ppPSReductionWorkTex[i].Get(), m_ppPSReductionWorkSRV[i]))
			{
				return false;
			}

			if (!this->CreateMyShaderResourceView(m_ppCSReductionWorkTex[i].Get(), m_ppCSReductionWorkSRV[i]))
			{
				return false;
			}
		}

		// コンピュート シェーダーと他のシェーダーで相互運用するための、
		// RWTexture2D/RWTexture1D のアンオーダード アクセス ビューを作成する。
		// RWTexture2D/RWTexture1D が使えるのは Direct3D 11 世代以降の GPU のみなので、
		// その UAV も機能レベル 11.0 以上のデバイスでないと作成できないと思われる。
		// もしくはシェーダーステージへのバインド時に拒否されるはず。

		for (int i = 0; i < 2; ++i)
		{
			if (!this->CreateMyUnorderedAccessView(m_ppDownSampledTempWorkTex[i].Get(), m_ppDownSampledTempWorkUAV[i]))
			{
				return false;
			}

			if (!this->CreateMyUnorderedAccessView(m_ppCSReductionWorkTex[i].Get(), m_ppCSReductionWorkUAV[i]))
			{
				return false;
			}
		}

		return true;
	}


	bool MyD3DManager::CreateBackBufferRenderTargetView(UINT width, UINT height)
	{
		// バック バッファのレンダーターゲット ビューおよび関連インターフェイスを再構築する。
		_ASSERTE(m_pSwapChain != nullptr);
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_dwriteFactory != nullptr);

		HRESULT hr = E_FAIL;

		// DXGI_SWAP_CHAIN_DESC ではなく DXGI_SWAP_CHAIN_DESC1 を使って作成されたバック バッファでも、GetDesc() 呼び出しは一応成功する模様。
		// 本来は GetDesc1() を使うほうがよい。
#if 0
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		hr = m_pSwapChain->GetDesc(&swapChainDesc);
		if (FAILED(hr))
		{
			return false;
		}
		const UINT oldBackBufferWidth = swapChainDesc.BufferDesc.Width;
		const UINT oldBackBufferHeight = swapChainDesc.BufferDesc.Height;
		const auto oldBackBufferFormat = swapChainDesc.BufferDesc.Format;
#else
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		hr = m_pSwapChain->GetDesc1(&swapChainDesc);
		if (FAILED(hr))
		{
			return false;
		}
		const UINT oldBackBufferWidth = swapChainDesc.Width;
		const UINT oldBackBufferHeight = swapChainDesc.Height;
		const auto oldBackBufferFormat = swapChainDesc.Format;
#endif
		const UINT oldBackBufferFlags = swapChainDesc.Flags;

		if (oldBackBufferWidth != width || oldBackBufferHeight != height)
		{
			// フロント バッファとバック バッファの2つをリサイズする。
			// その前に、取得した D3D テクスチャ インターフェイスおよび D2D ビットマップ インターフェイスはあらかじめ解放しておく必要がある。
			// でないとリサイズできない（リソースが内部で共有されていて、参照カウントが増加しているため）。
			m_pBackBufferTex.Reset();
			m_pBackBufferRTV.Reset();
			m_d2dTargetBitmap.Reset();
			m_pSwapChain->ResizeBuffers(2, width, height, oldBackBufferFormat, oldBackBufferFlags);
			if (FAILED(hr))
			{
				return false;
			}
		}

		// バック バッファを Direct3D テクスチャとして取得し、そのレンダーターゲット ビューを作成する。
		//hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(m_pBackBufferTex.ReleaseAndGetAddressOf()));
		hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(m_pBackBufferTex.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		if (!this->CreateMyRenderTargetView(m_pBackBufferTex.Get(), m_pBackBufferRTV))
		{
			return false;
		}

		// バックバッファの DXGI フォーマットの確認。
		{
			D3D11_TEXTURE2D_DESC backBufferDesc = {};
			m_pBackBufferTex->GetDesc(&backBufferDesc);
			ATLTRACE("Is DXGI format of back buffer B8G8R8A8_UNORM? : %d\n", (backBufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM));
		}

		// DirectWrite 関連。
		{
			_ASSERTE(m_textFormat != nullptr);
			const std::wstring sampleText = L"→π……――これは DirectWrite テキストです!!♡";
			hr = m_dwriteFactory->CreateTextLayout(
				sampleText.c_str(),
				UINT32(sampleText.length()),
				m_textFormat.Get(),
				float(width), // maxWidth.
				float(height), // maxHeight.
				m_textLayout.ReleaseAndGetAddressOf()
				);
			if (FAILED(hr) || !m_textLayout)
			{
				DXTRACE_ERR(_T("Failed to create DWrite text layout!!"), hr);
				return false;
			}
		}
#ifdef USE_D2D
		// Direct2D 関連。
		{
			_ASSERTE(m_d2dContext != nullptr);
			// DXGI スワップ チェーン バック バッファに関連付けられた Direct2D ターゲット ビットマップを作成し、
			// それを現在の Direct2D ターゲットとして設定する。
			// 基本的な考え方は D3D 10.1 + D2D 1.0 時代と同じ。
			const D2D1_BITMAP_PROPERTIES1 bitmapProperties = 
				D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				m_dpi,
				m_dpi
				);

			Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
			hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(dxgiBackBuffer.GetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}

			hr = m_d2dContext->CreateBitmapFromDxgiSurface(
				dxgiBackBuffer.Get(),
				&bitmapProperties,
				m_d2dTargetBitmap.ReleaseAndGetAddressOf()
				);
			if (FAILED(hr))
			{
				return false;
			}
		}
#endif

		return true;
	}


	//! @brief  深度ステンシル テクスチャおよびそのビューを作成する。<br>
	bool MyD3DManager::CreateDepthStencilAndView(UINT width, UINT height, UINT msaaSampleCount, UINT msaaQuality)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

#pragma region // 深度ステンシル テクスチャを作成する。//
		// Direct3D 10/11 においては、ステンシル バッファもテクスチャ リソースでしかない。
		// バインド フラグとビュー（用途）が異なるだけ。
		// なお、カラーバッファが MSAA の場合、深度ステンシルも同カウントの MSAA にする必要がある。

		D3D11_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		//description.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		description.Format = DXGI_FORMAT_D32_FLOAT;
		description.Width = width;
		description.Height = height;
		description.MipLevels = 1;
		description.SampleDesc.Count = msaaSampleCount;
		description.SampleDesc.Quality = msaaQuality;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pSubDepthStencilTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

#pragma endregion

		// 深度ステンシル テクスチャのビューを作成する。
		if (!this->CreateMyDepthStencilView(m_pSubDepthStencilTex.Get(), m_pSubDSV))
		{
			return false;
		}

		return true;
	}

	static void CreateWaveSimInitSrc(std::vector<MyMath::Vector4F>& tempBuf0, std::vector<MyMath::Vector4F>& tempBuf1, uint32_t width, uint32_t height)
	{
		// HACK: 波の外乱はユーザー操作の結果として追加されるとよい。また、波の減衰も実装する。
		// ユーザー指定位置に沈むあるいは浮く物体を投下する？　衝突シミュレーションや浮力の計算が必要。流体自体の粘性、密度、表面張力、重力加速度の影響は？
		// Blender に搭載されている流体シミュレーションのような感じ。問題領域をドメインとしてメッシュ化し、有限要素解析を行なう。
		// CAPCOM の鬼武者3は PS2 の時点ですでに Emotion Engine による流体シミュレーションを実装していたらしい。さすがとしか言いようがない。
		// http://www.capcom.co.jp/onimusha/ja/special/tech03.html
		tempBuf0.resize(width * height, MyMath::ZERO_VECTOR4F);
		tempBuf1.resize(width * height, MyMath::ZERO_VECTOR4F);
		const uint32_t minIndex = COMPUTING_TEMP_WORK_SIZE / 2 - 10;
		const uint32_t maxIndex = COMPUTING_TEMP_WORK_SIZE / 2 + 10;
		const float iniPeakHeight = 40.0f;
		// 適当な矩形の形状を持つ初期条件を与える。
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				auto& targetPoint = tempBuf0[y * width + x];
				targetPoint.y = 1.0f;
				if (minIndex < x && x < maxIndex && minIndex < y && y < maxIndex)
				{
					targetPoint.w = iniPeakHeight;
				}
			}
		}
		const float WaveVelocity = 1.0f;
		const float DeltaT = 0.1f;
		const float Delta = 2.0f;
		const float WaveVelocityC = WaveVelocity * WaveVelocity * DeltaT * DeltaT / (Delta * Delta);
		for (uint32_t y = 1; y < height - 1; ++y)
		{
			for (uint32_t x = 1; x < width - 1; ++x)
			{
				auto& targetPoint = tempBuf1[y * width + x];
				const float t0 = tempBuf0[y * width + x].w;
				const float t1 = tempBuf0[y * width + (x - 1)].w;
				const float t2 = tempBuf0[y * width + (x + 1)].w;
				const float t3 = tempBuf0[(y - 1) * width + x].w;
				const float t4 = tempBuf0[(y + 1) * width + x].w;
				targetPoint.y = 1.0f;
				// (t1 - t0) + (t2 - t0) + (t3 - t0) + (t4 - t0)
				// で差分計算を行なう。
				targetPoint.w = t0 + 0.5f * WaveVelocityC * (t1 + t2 + t3 + t4 - 4 * t0);
			}
		}
		// HACK: 本来は高さ設定後に改めて Frame#0 と Frame#1 の法線を（周囲4近傍を使って）計算するべき。
		// 頂点バッファに情報を含めないで、頂点シェーダーで計算してしまったほうがいい。
	}

	// 波面シミュレーション用ワーク テクスチャを作成する。
	bool MyD3DManager::CreateWaveSimWorkTexAndView()
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		// HACK: グリッドを任意形状にできるように、2D テクスチャではなく構造化バッファと隣接インデックスを使って各グリッドをリンクするようにしておいたほうがいいかも。

		D3D11_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.MipLevels = 1;

		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;

		// HACK: 法線を格納せず頂点シェーダーで毎回計算するならば、R32_FLOAT で十分。
		description.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

		description.Width = COMPUTING_TEMP_WORK_SIZE;
		description.Height = COMPUTING_TEMP_WORK_SIZE;

		description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		std::vector<MyMath::Vector4F> tempBuf0;
		std::vector<MyMath::Vector4F> tempBuf1;
		CreateWaveSimInitSrc(tempBuf0, tempBuf1, description.Width, description.Height);

		for (uint32_t i = 0; i < WaveSimWorkBufferCount; ++i)
		{
			D3D11_SUBRESOURCE_DATA subData = {};
			subData.SysMemPitch = description.Width * sizeof(MyMath::Vector4F);
			if (i == 0)
			{
				subData.pSysMem = &tempBuf0[0];
			}
			else if (i == 1)
			{
				subData.pSysMem = &tempBuf1[0];
			}

			hr = m_pD3DDevice->CreateTexture2D(
				&description,
				(i == 2) ? nullptr : &subData,
				m_ppWaveSimWorkTex[i].ReleaseAndGetAddressOf());

			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
				return false;
			}

			// SRV と UAV を作る。

			if (!this->CreateMyShaderResourceView(m_ppWaveSimWorkTex[i].Get(), m_ppWaveSimWorkSRV[i]))
			{
				return false;
			}

			if (!this->CreateMyUnorderedAccessView(m_ppWaveSimWorkTex[i].Get(), m_ppWaveSimWorkUAV[i]))
			{
				return false;
			}
		}

		// グリッドの有効／無効を定義するマスク画像。
		description.Format = DXGI_FORMAT_R8_UNORM;
		description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		description.Usage = D3D11_USAGE_IMMUTABLE;

		const uint32_t strideInBytes = MyMath::CalcStrideInBytes(description.Width, 8);

		std::vector<uint8_t> tempMaskBuf(strideInBytes * description.Height);
		const double activeRadius = 120;
		const double activeRadiusSq = MyUtil::SquareVal(activeRadius);
		for (uint32_t y = 0; y < description.Height; ++y)
		{
			for (uint32_t x = 0; x < description.Width; ++x)
			{
				const double radiusSq = MyUtil::SquareVal(x - 0.5 * description.Width) + MyUtil::SquareVal(y - 0.5 * description.Height);
				if (radiusSq <= activeRadiusSq)
				{
					tempMaskBuf[y * strideInBytes + x] = 0xFF;
				}
			}
		}

		{
			D3D11_SUBRESOURCE_DATA subData = {};
			subData.SysMemPitch = strideInBytes;
			subData.pSysMem = &tempMaskBuf[0];

			hr = m_pD3DDevice->CreateTexture2D(
				&description,
				&subData,
				m_pWaveSimMaskTex.ReleaseAndGetAddressOf());

			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
				return false;
			}

			if (!this->CreateMyShaderResourceView(m_pWaveSimMaskTex.Get(), m_pWaveSimMaskSRV))
			{
				return false;
			}
		}

		return true;
	}

#if 0
	bool MyD3DManager::CreateWaveSimWorkBufferAndView()
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		const uint32_t width = COMPUTING_TEMP_WORK_SIZE;
		const uint32_t height = COMPUTING_TEMP_WORK_SIZE;
		std::vector<MyMath::Vector4F> tempBuf0;
		std::vector<MyMath::Vector4F> tempBuf1;
		CreateWaveSimInitSrc(tempBuf0, tempBuf1, width, height);

		for (uint32_t i = 0; i < WaveSimWorkBufferCount; ++i)
		{
			const void* pInitData = nullptr;
			if (i == 0)
			{
				pInitData = &tempBuf0[0];
			}
			else if (i == 1)
			{
				pInitData = &tempBuf1[0];
			}
			if (!CreateRWStructuredBuffer(m_pD3DDevice.Get(), sizeof(MyMath::Vector4F), width * height, pInitData, m_ppWaveSimWorkStructBuffers[i]))
			{
				return false;
			}

			// SRV と UAV を作る。

			if (!this->CreateMyShaderResourceView(m_ppWaveSimWorkStructBuffers[i].Get(), m_ppWaveSimWorkSRV[i]))
			{
				return false;
			}

			if (!this->CreateMyUnorderedAccessView(m_ppWaveSimWorkStructBuffers[i].Get(), m_ppWaveSimWorkUAV[i]))
			{
				return false;
			}
		}

		return true;
	}
#endif

	bool MyD3DManager::CreateRandomTableBufferAndView()
	{
		// とりあえず Direct3D 版と OpenGL 版との比較をするため、同じ乱数シードを与える。
		std::srand(0);
		std::vector<MyMath::Vector4UI> initialRandomNumbers(RANDOM_NUM_TABLE_DATA_COUNT);
		std::for_each(initialRandomNumbers.begin(), initialRandomNumbers.end(),
			[](MyMath::Vector4UI& x) { x = MyMath::Xorshift128Random::CreateInitialNumber(std::rand()); });

		if (!CreateRWStructuredBuffer(m_pD3DDevice.Get(), sizeof(MyMath::Vector4UI), RANDOM_NUM_TABLE_DATA_COUNT, &initialRandomNumbers[0], m_pRandomTableBuf))
		{
			return false;
		}

		// Input 用に SRV を作成。
		if (!this->CreateMyBufferShaderResourceView(m_pRandomTableBuf.Get(), m_pRandomTableSRV))
		{
			return false;
		}

		// Output 用に UAV を作成。
		if (!this->CreateMyBufferUnorderedAccessView(m_pRandomTableBuf.Get(), m_pRandomTableUAV))
		{
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateFlakeParticleBufferAndView()
	{
		//m_flakeParticleCount = 25000;
		m_flakeParticleCount = 1000;
		//m_flakeParticleCount = 10;

		std::vector<MyVertexTypes::MyFlakeVertex> iniFlakeData(m_flakeParticleCount);
		{
			std::mt19937 randomEngine;
			std::uniform_real_distribution<float> uniformDist(-10.0f, +10.0f);
			std::for_each(iniFlakeData.begin(), iniFlakeData.end(),
				[&](decltype(iniFlakeData[0])& x)
				{
					x.Position.x = uniformDist(randomEngine);
					x.Position.y = uniformDist(randomEngine);
					x.Position.z = uniformDist(randomEngine);
					// TODO: 姿勢を表すクォータニオンも乱数を使って初期化する。
				}
			);
		}

		if (!CreateRWStructuredBuffer(m_pD3DDevice.Get(), sizeof(MyVertexTypes::MyFlakeVertex), m_flakeParticleCount, &iniFlakeData[0], m_pFlakeParticleBuf))
		{
			return false;
		}

		if (!this->CreateMyBufferShaderResourceView(m_pFlakeParticleBuf.Get(), m_pFlakeParticleSRV))
		{
			return false;
		}

		if (!this->CreateMyBufferUnorderedAccessView(m_pFlakeParticleBuf.Get(), m_pFlakeParticleUAV))
		{
			return false;
		}

		return true;
	}

	void MyD3DManager::InitializeCameraSettings()
	{
		m_myCameraSettings.Initialize();
	}

	static void ApplyTestLightColor(CBufferLightParamsPack& lightParam)
	{
		//const float ambientIntensity = 0.5f;
		//const float ambientIntensity = 0.2f;
		const float ambientIntensity = 32.0f / 255.0f;
		lightParam.AmbientLight = MyMath::Vector4F(ambientIntensity, ambientIntensity, ambientIntensity, 1);
		lightParam.LightColor = MyMath::Vector4F(1, 1, 1, 1);
	}

	void MyD3DManager::UpdateEffectMatrices()
	{
		// シェーダーで何度も使用される行列 [World x View x Proj] はあらかじめ CPU で乗算しておく。
		// 定数バッファに直接書き込む場合、シェーダーコード側でそのまま使えるようにするには、明示的に CPU 側で転置しておくとよい。
		// なお、CPU 側で転置したものを渡せば、ワールド行列およびビュー行列に関してはシェーダー側は 3x4 行列でよくなるので、スロットを節約できる。
		// ボーン行列のように大量の行列を使用する場合に効果を発揮する。
		// D3DX Math, XNA Math, DirectXMath の行列は行優先（平行移動成分が4行目に入っている）なので、
		// 転置して列優先（平行移動成分が4列目に入っている）に変換してしまい、GPU 転送時に4行目を捨てて float3x4 を使うようにすればよい。
		// プロジェクション行列は4列目（{0,0,1,0}^T）をシェーダーで補完するよりも、最初から 4x4 のほうがよいかも。
		// GLSL には 3x4 行列がないので、vec4 を3つ使うことになる。ベクトル or 行列との積を計算する場合は一手間必要になる。
		// ちなみにエフェクト変数経由で行列を渡す場合、内部で暗黙的に転置されるらしい。
		// MSDN にも（DirectX 8.1 時代の古い記事ではあるが）、
		// 「定数レジスタにロードする行列は転置行列で無ければならない、エフェクト フレームワークはこれを自動的に行う。」という言及がある。
		// http://msdn.microsoft.com/ja-jp/library/dd188541.aspx
		CBufferViewParamsPack viewParam;
		m_myCameraSettings.GetTrMatrixWorldViewProj(
			&viewParam.UniWorldMatrix, &viewParam.UniViewMatrix, &viewParam.UniProjectionMatrix,
			&viewParam.UniWorldViewMatrix, &viewParam.UniViewProjMatrix, &viewParam.UniWorldViewProjMatrix);
		viewParam.UniEyePosition = m_myCameraSettings.m_vCameraEye;
		viewParam.UniScreenSize.x = m_myCameraSettings.m_screenWidth;
		viewParam.UniScreenSize.y = m_myCameraSettings.m_screenHeight;
		this->UpdateCBuffer(&viewParam);

		CBufferLightParamsPack lightParam;
		m_myCommonSettings.GetRotatedLightDir(&lightParam.LightDir);
		// グローバル原点をライトの注視点とし、グローバル原点からライトのワールド位置までの距離を指定する。
		// 平行ライトだと絶対的な位置は関係ないが、鏡面反射やシャドウマップを実装する場合はライト位置が必要。
		// なお、カスケード シャドウにおける視錐台クリッピングを行なう際、シーン AABB の内部にライト視点が入っている必要がある模様。
		// 詳細は DirectX SDK サンプルの初期化コードを参照のこと。
		const float lightRadius = 400;
		MyMath::MultiplyVector3(lightParam.LightPos, -lightRadius, lightParam.LightDir);

		ApplyTestLightColor(lightParam);
		this->UpdateCBuffer(&lightParam);

		// class にダミー float4 以外のメンバーを含まず、メソッドだけを定義しているので、定数バッファの更新は不要。
		//this->UpdateCBuffer(CBufferClassInstanceSelectorTable());

		MyMath::Matrix4x4F matSceneView, matSceneProj, matLightView;
		m_myCameraSettings.GetMatrixView(&matSceneView);
		m_myCameraSettings.GetMatrixProjection(&matSceneProj);
		// HACK: シーン初期化時やマップ（ゲームレベル）の遷移時に AABB は変動しうる。
		const float sceneRadius = 600;
		const MyMath::Vector3F vSceneAABBMin(-sceneRadius, -sceneRadius, -sceneRadius);
		const MyMath::Vector3F vSceneAABBMax(+sceneRadius, +sceneRadius, +sceneRadius);
#ifdef USE_LEFT_HAND_COORD_SYS
		MyMath::CreateMatrixLookAtLH(
#else
		MyMath::CreateMatrixLookAtRH(
#endif
			&matLightView, &lightParam.LightPos, &MyMath::ZERO_VECTOR3F, &MyMath::Vector3F(0, 1, 0));
		m_shadowMapManager.InitFrame(m_shadowCascadeConfig,
			vSceneAABBMin, vSceneAABBMax,
			matSceneView, matSceneProj,
			m_myCameraSettings.ClipPlaneNearZ, m_myCameraSettings.ClipPlaneFarZ,
			matLightView);
		CBufferShadowRenderingInfoPack shadowRenderParam;
		m_shadowMapManager.GetShadowRenderingInfo(shadowRenderParam);
		this->UpdateCBuffer(shadowRenderParam);
		m_shadowMapManager.GetShadowSamplingInfo(m_tempShadowSamplingInfo, 0.00001f);
		this->UpdateCBuffer(m_tempShadowSamplingInfo);
	}


	bool MyD3DManager::CreateMyDynamicFontRects()
	{
		return false;
	}


	bool MyD3DManager::CreateSinglePointVertexBufferP()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pSinglePointVertexBufferP == nullptr);

		HRESULT hr = E_FAIL;

		const UINT vertexCount = 1U;

		const MyVertexTypes::MyVertexP pVerticesArray[vertexCount] =
		{
			MyMath::Vector3F(0, 0, 0),
		};

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(*pVerticesArray);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = pVerticesArray;

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, &m_pSinglePointVertexBufferP);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	//! @brief  X-Y-Z 座標軸ライン用のライティング済み・未トランスフォーム頂点バッファを作成する。<br>
	bool MyD3DManager::CreateCoordAxisLineVertexBufferPC()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pCoordAxisLineVertexBufferPC == nullptr);

		HRESULT hr = E_FAIL;

		const UINT vertexCount = 6U;
		const float axisLength = 300;
		const MyVertexTypes::MyVertexPC pVerticesArray[vertexCount] =
		{
			// { Pos, Color }
			{ MyMath::Vector3F(0, 0, 0),          MyMath::Vector4F(1, 0, 0, 1) },
			{ MyMath::Vector3F(axisLength, 0, 0), MyMath::Vector4F(1, 0, 0, 1) },
			{ MyMath::Vector3F(0, 0, 0),          MyMath::Vector4F(0, 1, 0, 1) },
			{ MyMath::Vector3F(0, axisLength, 0), MyMath::Vector4F(0, 1, 0, 1) },
			{ MyMath::Vector3F(0, 0, 0),          MyMath::Vector4F(0, 0, 1, 1) },
			{ MyMath::Vector3F(0, 0, axisLength), MyMath::Vector4F(0, 0, 1, 1) },
		};

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(*pVerticesArray);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = pVerticesArray;

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, &m_pCoordAxisLineVertexBufferPC);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}


	//! @brief  （シェーディングのテスト描画で使われる）単一三角形用のライティング済み・未トランスフォーム頂点バッファを作成する。<br>
	bool MyD3DManager::CreateOneTriangleVertexBuffer()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pOneTriangleVertexBufferPCNT == nullptr);

		HRESULT hr = E_FAIL;

		const UINT vertexCount = 3U;
		const MyVertexTypes::MyVertexPCNT pVerticesArray[vertexCount] =
		{
			// { Pos, Color, Normal, Tex }
			{ MyMath::Vector3F( 0, +5, +5), MyMath::Vector4F(1, 0, 0, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(-5,  0, +5), MyMath::Vector4F(0, 1, 0, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+5,  0, +5), MyMath::Vector4F(0, 0, 1, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			// 右手系の定義順。
		};

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(*pVerticesArray);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = pVerticesArray;

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, &m_pOneTriangleVertexBufferPCNT);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}


	//! @brief  （シェーディングのテスト描画で使われる）単一矩形用のライティング済み・未トランスフォーム頂点バッファを作成する。<br>
	bool MyD3DManager::CreateOneSquareVertexBufferPCNT()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pOneSquareVertexBufferPCNT == nullptr);

		HRESULT hr = E_FAIL;

		const UINT vertexCount = 4U;
		const MyVertexTypes::MyVertexPCNT pVerticesArray[vertexCount] =
		{
			// { Pos, Color, Normal, Tex }
			{ MyMath::Vector3F(-5, +5, -5), MyMath::Vector4F(1, 0, 0, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+5, +5, -5), MyMath::Vector4F(0, 1, 0, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(-5, -5, -5), MyMath::Vector4F(0, 0, 1, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+5, -5, -5), MyMath::Vector4F(1, 1, 0, 1), MyMath::Vector3F(0, 0, +1), MyMath::Vector2F(0, 0) },
			// LT, RT, LB, RB.（左手系の定義順）
		};

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(*pVerticesArray);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = pVerticesArray;

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, &m_pOneSquareVertexBufferPCNT);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}


	bool MyD3DManager::CreateWaveFrontPlaneVertexBufferPCNT()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pWaveFrontPlaneVertexBufferPCNT == nullptr);

		HRESULT hr = E_FAIL;

		const UINT vertexCount = 4U;
		const float planeLength = 1;
		const float planeHeight = 0;
		// HACK: 水面の法線はシミュレーションで分割単位（グリッド）ごとに計算するので、メッシュ頂点法線は不要？
		// 海面の盛り上がりなどの大きな変形（ディスプレースメント マッピング）でテッセレーションを活用する場合も、
		// 頂点法線は GPU 側で計算したほうが良さげ。
		// 水の透明度や色は頂点カラーではなく定数バッファでマテリアルとして制御できたほうがよい？
		const MyVertexTypes::MyVertexPCNT pVerticesArray[vertexCount] =
		{
			// { Pos, Color, Normal, Tex }
			{ MyMath::Vector3F(-planeLength, planeHeight, -planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+planeLength, planeHeight, -planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(1, 0) },
			{ MyMath::Vector3F(-planeLength, planeHeight, +planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(0, 1) },
			{ MyMath::Vector3F(+planeLength, planeHeight, +planeLength),
			MyMath::Vector4F(1, 1, 1, 1), MyMath::Vector3F(0, +1, 0), MyMath::Vector2F(1, 1) },
			// LT, RT, LB, RB.（左手系の定義順）
		};

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(*pVerticesArray);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = pVerticesArray;

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, &m_pWaveFrontPlaneVertexBufferPCNT);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateWaveFrontPlaneGridBufferAndView()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pWaveFrontPlaneGridBuf == nullptr);

		HRESULT hr = E_FAIL;

		// HACK: レベル エディタで設定した水面のグリッド情報（必要な部分だけ生成）を使う。
		// インスタンシングやジオメトリ シェーダーを利用するよりも、あらかじめ全頂点を用意しておいたほうが速度面では有利？
		// それとも4点だけならば頂点バッファやインデックス バッファがまるごとキャッシュに乗ることになるので、インスタンシングのほうが有利？
		// メモリ効率を考えれば、頂点バッファをだらだら生成するよりもインスタンシングのほうが良い。
		// DirectX 12 では抽象化レイヤーのオーバーヘッドが低減されるらしいので、インスタンシングを真の意味で活用できるようになるはず。
		const UINT gridCount = COMPUTING_TEMP_WORK_SIZE * COMPUTING_TEMP_WORK_SIZE;
		m_waveFrontPlaneGridCount = gridCount;
		std::vector<MyMath::Vector2I> gridsArray(gridCount);
		for (uint32_t y = 0; y < COMPUTING_TEMP_WORK_SIZE; ++y)
		{
			for (uint32_t x = 0; x < COMPUTING_TEMP_WORK_SIZE; ++x)
			{
				gridsArray[y * COMPUTING_TEMP_WORK_SIZE + x] = MyMath::Vector2I(x, y);
			}
		}

		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.ByteWidth = gridCount * sizeof(gridsArray[0]);
		bufDesc.Usage = D3D11_USAGE_IMMUTABLE; // シーンの初期化時に生成するだけなので、Immutable を指定する。
		bufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // 頂点バッファでもインデックス バッファでもない。

		D3D11_SUBRESOURCE_DATA subData = {};
		subData.pSysMem = &gridsArray[0];

		hr = m_pD3DDevice->CreateBuffer(&bufDesc, &subData, &m_pWaveFrontPlaneGridBuf);
		if (FAILED(hr))
		{
			return false;
		}

		if (!this->CreateMyBufferShaderResourceView(m_pWaveFrontPlaneGridBuf.Get(), DXGI_FORMAT_R32G32_SINT, gridCount, m_pWaveFrontPlaneGridSRV))
		{
			return false;
		}

		return true;
	}


	//! @brief  （ダウンサンプルなどで使われる）単一矩形用のライティング済み・トランスフォーム済み頂点バッファを作成する。<br>
	bool MyD3DManager::CreateOneSquareVertexBufferPCT()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pOneSquareVertexBufferPCT == nullptr);

		HRESULT hr = E_FAIL;

		// 四隅の頂点カラーを変えることで、補間によるグラデーションがかかる。
		const UINT vertexCount = 4U;
		const MyVertexTypes::MyVertexPCT pVerticesArray[vertexCount] =
		{
			// { Pos, Color, Tex }
			// Direct3D の UV は左上原点。OpenGL だと左下原点なので注意（頂点データを共通にする場合、GLSL シェーダーで反転させるとよい）。
			// 色はとりあえずデバッグ用途も兼ねて適当に指定している。
			// ダウンサンプルやフォーマット変換は頂点シェーダー＋ピクセル シェーダーを使った従来の方法よりも、
			// コンピュート シェーダーを使ったほうが効率的だが、画面サイズを変更した場合の調整（スレッド数の設定）が煩雑になるのが欠点。
#if 1
			{ MyMath::Vector3F(-1, +1, 0), MyMath::Vector4F(1.0f, 0.5f, 0.5f, 1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+1, +1, 0), MyMath::Vector4F(1.0f, 0.5f, 0.5f, 1), MyMath::Vector2F(1, 0) },
			{ MyMath::Vector3F(-1, -1, 0), MyMath::Vector4F(0.5f, 0.5f, 1.0f, 1), MyMath::Vector2F(0, 1) },
			{ MyMath::Vector3F(+1, -1, 0), MyMath::Vector4F(0.5f, 0.5f, 1.0f, 1), MyMath::Vector2F(1, 1) },
			// LT, RT, LB, RB.（左手系の定義順）
#else
			{ MyMath::Vector3F(-0.5f, +0.5f, 0), MyMath::Vector4F(1.0f, 0.5f, 0.5f, 1), MyMath::Vector2F(0, 0) },
			{ MyMath::Vector3F(+0.5f, +0.5f, 0), MyMath::Vector4F(1.0f, 0.5f, 0.5f, 1), MyMath::Vector2F(1, 0) },
			{ MyMath::Vector3F(-0.5f, -0.5f, 0), MyMath::Vector4F(0.5f, 0.5f, 1.0f, 1), MyMath::Vector2F(0, 0.3f) },
			{ MyMath::Vector3F(+0.5f, -0.5f, 0), MyMath::Vector4F(0.5f, 0.5f, 1.0f, 1), MyMath::Vector2F(1, 0.3f) },
#endif
		};

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(*pVerticesArray);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = pVerticesArray;

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, &m_pOneSquareVertexBufferPCT);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}


	//! @brief  四辺形用のインデックス バッファを作成する。<br>
	bool MyD3DManager::CreateOneQuadIndexBuffer()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pOneQuadIndexBuffer == nullptr);

		HRESULT hr = E_FAIL;

		const uint32_t indexCount = MyMath::OneQuadIndexCount;
		const uint16_t* pIndicesArray =
#ifdef USE_LEFT_HAND_COORD_SYS
			MyMath::OneQuadIndicesArray012;
#else
			MyMath::OneQuadIndicesArray021;
#endif

		D3D11_BUFFER_DESC ibDesc = {};
		ibDesc.ByteWidth = indexCount * sizeof(*pIndicesArray);
		ibDesc.Usage = D3D11_USAGE_DEFAULT;
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA ibSubrData = {};
		ibSubrData.pSysMem = pIndicesArray;

		hr = m_pD3DDevice->CreateBuffer(&ibDesc, &ibSubrData, &m_pOneQuadIndexBuffer);
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}


	//! @brief  イメージ ベース ファーシェーダーで使われるライティング済み・トランスフォーム済み頂点バッファを作成する。<br>
	bool MyD3DManager::CreateImageBasedFurVertexBuffer(UINT width, UINT height)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		// スクリーン サイズに合わせて頂点数を増減するのではなく、上限を決めて最大数で作成するか、
		// 頂点数1でインスタンス描画するようにしたほうがよい。
#if 1
		//const UINT width = m_myCameraSettings.m_screenWidth;
		//const UINT height = m_myCameraSettings.m_screenHeight;
		const UINT skip = 3; // 間引きする。
		const UINT vertexCount = (width / skip) * (height / skip);
		_ASSERTE(vertexCount > 0);
		std::vector<MyVertexTypes::MyVertexPCT> vertexArray(vertexCount);

		for (UINT h = 0; (h / skip) < (height / skip); h += skip)
		{
			for (UINT w = 0; (w / skip) < (width / skip); w += skip)
			{
				// シェーダー側の計算負荷を少しでも減らすため、静的なデータはあらかじめ CPU 側で計算しておく。
				const UINT index = (h / skip) * (width / skip) + (w / skip);
				vertexArray[index].Position.x = (+2.0f * w) / (width - 1) - 1.0f;
				vertexArray[index].Position.y = (-2.0f * h) / (height - 1) + 1.0f;
				vertexArray[index].Position.z = 0;
				vertexArray[index].Color = MyMath::ZERO_VECTOR4F; // HACK: 別になくてもよくね？
				vertexArray[index].TexCoord.x = static_cast<float>(w) / (width - 1);
				vertexArray[index].TexCoord.y = static_cast<float>(h) / (height - 1);
			}
		}
#else
		const UINT vertexCount = 1;
		std::vector<MyMath::MyVertexPCT> vertexArray(vertexCount);
#endif
		m_imageBasedFurVertexCount = vertexCount;

		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = vertexCount * sizeof(vertexArray[0]);
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vbSubrData = {};
		vbSubrData.pSysMem = &vertexArray[0];

		hr = m_pD3DDevice->CreateBuffer(&vbDesc, &vbSubrData, m_pImageBasedFurVertexBufferPCT.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateMyFontTexture()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pHudFontTexData != nullptr);
		_ASSERTE(m_pHudFontAlphaTex == nullptr);

		if (!MyD3D::CreateFontAlphaTexture(m_pD3DDevice.Get(), m_pHudFontTexData->TextureData, m_pHudFontAlphaTex))
		{
			return false;
		}

		// デバッグ用にアルファ テクスチャをファイル保存してみる。
		this->DumpTextureToFile(m_pHudFontAlphaTex.Get(), L"font_alpha_map.dds");

		return true;
	}


	bool MyD3DManager::DumpTextureToFile(ID3D11Texture2D* pTexture, LPCWSTR pFileName)
	{
		HRESULT hr = E_FAIL;

		// EXE のあるフォルダーに出力される。デバッグ用途だが、本来はユーザードキュメント フォルダーに保存するべき。
		CPathW pathOutputImageFile = m_pathLogDir;
		pathOutputImageFile += pFileName;
		DirectX::ScratchImage tempImage;
		hr = DirectX::CaptureTexture(m_pD3DDevice.Get(), this->GetMainThreadDeviceContext(), pTexture, tempImage);
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to capture texture!!"), hr);
			return false;
		}
		// D3DX ヘルパー（D3DX11SaveTextureToFile() 関数など）であれば、
		// DDS ファイルに保存するときにはデフォルトでミップマップも自動でまとめて保存することになるはず。
		// DirectXTex はまとめて保存する以外に、ミップマップを個別に保存できるようになっているらしい。
		// これにより、DDS フォーマット以外に WIC 経由で PNG 保存もできるようになっている。
#if 0
		// 1スライスのみ。
		auto image0 = tempImage.GetImage(0, 0, 0);
		_ASSERTE(image0 != nullptr);
		hr = DirectX::SaveToDDSFile(*image0, DirectX::DDS_FLAGS_NONE, pathOutputImageFile);
#else
		// 全スライス。
		hr = DirectX::SaveToDDSFile(tempImage.GetImages(), tempImage.GetImageCount(), tempImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, pathOutputImageFile);
#endif
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to save texture to file!!"), hr);
			return false;
		}
		return true;
	}


	bool MyD3DManager::CreateMyDummyWhiteTexture()
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		const uint32_t texW = MyTextureHelper::DUMMY_WHITE_TEX_SIZE;
		const uint32_t texH = MyTextureHelper::DUMMY_WHITE_TEX_SIZE;

		D3D11_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		description.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 各チャンネル 8bit, 0～255 の範囲。
		description.Width = texW;
		description.Height = texH;
		description.MipLevels = 1;
		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;

		// デフォルトで白色。
		const std::vector<uint32_t> textureBuf(texW * texH, 0xFFFFFFFF);
		//const std::vector<uint32_t> textureBuf(texW * texH, 0xFF0000FF);

		const uint32_t stride = MyMath::CalcStrideInBytes(texW, 32);
		D3D11_SUBRESOURCE_DATA subresData = {};
		subresData.pSysMem = &textureBuf[0];
		subresData.SysMemPitch = stride;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			&subresData,
			m_pDummyWhiteTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

		return true;
	}

	//! @brief  トゥーン シェーディング グラデーション参照テクスチャを作成する。<br>
	bool MyD3DManager::CreateMyToonShadingRefTexture()
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		_ASSERTE(m_pToonShadingDiffuseCoefRefTexData != nullptr);

		HRESULT hr = E_FAIL;

#if 0
		const uint32_t texW = MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;
		const uint32_t texH = MyTextureHelper::TOON_SHADING_REF_TEX_SIZE;
#else
		const uint32_t texW = m_pToonShadingDiffuseCoefRefTexData->TextureWidth;
		const uint32_t texH = m_pToonShadingDiffuseCoefRefTexData->TextureHeight;
#endif

		D3D11_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		description.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 各チャンネル 8bit, 0～255 の範囲。
		description.Width = texW;
		description.Height = texH;
		description.MipLevels = 1;
		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;
		// テクスチャを CPU で Map/Unmap 可能にする。
		// HACK: エディターとは違い、シーン初期化時にのみ作成すればいいゲームなどのアプリケーションでは、
		// CPU アクセスできないようにしたほうが効率が向上する。
		description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		description.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA subresData = {};
		subresData.pSysMem = &m_pToonShadingDiffuseCoefRefTexData->TextureDib[0];
		subresData.SysMemPitch = m_pToonShadingDiffuseCoefRefTexData->GetStrideInBytes();

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			&subresData,
			m_pToonShadingRefTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

		// TODO: ID3D11DeviceContext::Map()/Unmap() による更新メソッドを追加する。
		this->DumpTextureToFile(m_pToonShadingRefTex.Get(), L"toon_shading_ref.dds");

		return true;
	}


	//! @brief  シェーダーリソース ビューを作成する。<br>
	bool MyD3DManager::CreateMyShaderResourceView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& pSRV, _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC* pViewDesc)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		// HACK: ID3D11Resource::GetType() で D3D11_RESOURCE_DIMENSION 種別を取得し、
		// バッファもしくはテクスチャの Description からバインド フラグを事前チェックすると確実。

		// Texture2DArray の場合、pDesc に NULL を指定してリソースから得た情報をもとにビューを作成すると、
		// すべてのスライスにアクセス可能なビューとなる。
		// 特定のスライスのみに部分アクセス可能なビューを作成する場合、Description の明示的指定が必要。
		// SRV, RTV, DSV, UAV いずれも同様。
		hr = m_pD3DDevice->CreateShaderResourceView(pResource, pViewDesc, pSRV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create shader resource view!!"), hr);
			return false;
		}

		return true;
	}

	//! @brief  レンダーターゲット ビューを作成する。<br>
	bool MyD3DManager::CreateMyRenderTargetView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& pRTV, _In_opt_ const D3D11_RENDER_TARGET_VIEW_DESC* pViewDesc)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		hr = m_pD3DDevice->CreateRenderTargetView(pResource, pViewDesc, pRTV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create render target view!!"), hr);
			return false;
		}

		return true;
	}

	//! @brief  深度ステンシル ビューを作成する。<br>
	bool MyD3DManager::CreateMyDepthStencilView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& pDSV, _In_opt_ const D3D11_DEPTH_STENCIL_VIEW_DESC* pViewDesc)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		hr = m_pD3DDevice->CreateDepthStencilView(pResource, pViewDesc, pDSV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create depth stencil view!!"), hr);
			return false;
		}

		return true;
	}

	//! @brief  アンオーダード アクセス ビューを作成する。<br>
	bool MyD3DManager::CreateMyUnorderedAccessView(ID3D11Resource* pResource, _Out_ Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& pUAV, _In_opt_ const D3D11_UNORDERED_ACCESS_VIEW_DESC* pViewDesc)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		hr = m_pD3DDevice->CreateUnorderedAccessView(pResource, pViewDesc, pUAV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create depth stencil view!!"), hr);
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateMyBufferShaderResourceView(ID3D11Buffer* pBuffer, DXGI_FORMAT format, UINT numElements, _Out_ Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& pSRV)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		D3D11_BUFFER_DESC bufDesc = {};
		pBuffer->GetDesc(&bufDesc);

		_ASSERTE(bufDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE);
		_ASSERTE(!(bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED));
		_ASSERTE(!(bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS));

		// HLSL の Buffer オブジェクトに対応するビュー。
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; // EX ではないほう。
		srvDesc.Format = format;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;

		hr = m_pD3DDevice->CreateShaderResourceView(pBuffer, &srvDesc, pSRV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create shader resource view of buffer!!"), hr);
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateMyBufferUnorderedAccessView(ID3D11Buffer* pBuffer, DXGI_FORMAT format, UINT numElements, _Out_ Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& pUAV)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		D3D11_BUFFER_DESC bufDesc = {};
		pBuffer->GetDesc(&bufDesc);

		_ASSERTE(bufDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS);
		_ASSERTE(!(bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED));
		_ASSERTE(!(bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS));

		// HLSL の RWBuffer オブジェクトに対応するビュー。
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Format = format;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = numElements;

		hr = m_pD3DDevice->CreateUnorderedAccessView(pBuffer, &uavDesc, pUAV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create unordered access view of buffer!!"), hr);
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateMyBufferShaderResourceView(ID3D11Buffer* pBuffer, _Out_ Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& pSRV)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		D3D11_BUFFER_DESC bufDesc = {};
		pBuffer->GetDesc(&bufDesc);

		_ASSERTE(bufDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE);

		// Direct3D 10 以降で使える Buffer リソースと差別化するために、StructuredBuffer 用に BufferEx というメンバーが導入されたらしい。
		// Buffer の使い方は DirectX SDK 付属の Direct3D 10 サンプル MultiStreamRendering や PipesGS を参照のこと。
		// Feature Level 11 が使える場合、StructuredBuffer があれば Buffer は別になくてもよさげだが……
		// MSDN では「The Buffer type supports most texture object methods except GetDimensions.」とあるが、これはおそらく間違い。
		// http://msdn.microsoft.com/en-us/library/bb509700.aspx
		// Buffer はテクスチャではないので、Load() メソッドとインデクサしか使えない模様。当然サンプラーは使えない。
		// ちなみに StructuredBuffer は GetDimensions() とインデクサしか使えないので、あまり大差ない。
		// ただし RWBuffer はないので、読み書き両方を行なう場合は必ず StructuredBuffer/RWStructuredBuffer が必要。

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if (bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファの場合は必須。
			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.NumElements = bufDesc.ByteWidth / bufDesc.StructureByteStride;
		}
		else if (bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // BAB の場合は必須。
			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.NumElements = bufDesc.ByteWidth / sizeof(uint32_t);
			srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		}
		else
		{
			_ASSERTE(false);
		}

		hr = m_pD3DDevice->CreateShaderResourceView(pBuffer, &srvDesc, pSRV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create shader resource view of buffer!!"), hr);
			return false;
		}

		return true;
	}

	bool MyD3DManager::CreateMyBufferUnorderedAccessView(ID3D11Buffer* pBuffer, _Out_ Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& pUAV)
	{
		_ASSERTE(m_pD3DDevice != nullptr);

		HRESULT hr = E_FAIL;

		D3D11_BUFFER_DESC bufDesc = {};
		pBuffer->GetDesc(&bufDesc);

		_ASSERTE(bufDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS);

		// MSDN の API ヘルプ (D3D11_BUFFER_UAV) には構造化バッファの場合の説明はあるが、BAB の場合の説明がない。
		// http://msdn.microsoft.com/ja-jp/library/ee416051.aspx
		// ただ、DX SDK 付属の BasicCompute11 サンプルでは BAB でも NumElements が指定されている。
		// BAB の場合、4バイト単位での要素数を指定すればよいらしい。

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		if (bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		{
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファの場合は必須。
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = bufDesc.ByteWidth / bufDesc.StructureByteStride;
		}
		else if (bufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		{
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS; // BAB の場合は必須。
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = bufDesc.ByteWidth / sizeof(uint32_t);
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		}
		else
		{
			_ASSERTE(false);
		}

		hr = m_pD3DDevice->CreateUnorderedAccessView(pBuffer, &uavDesc, pUAV.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create unordered access view of buffer!!"), hr);
			return false;
		}

		return true;
	}

	//! @brief  デバイスに（単一の）頂点バッファをセットする。<br>
	void MyD3DManager::SetMyVertexBuffer(ID3D11Buffer* pMyBuffer, UINT vertexSizeInBytes, D3D_PRIMITIVE_TOPOLOGY topologyType)
	{
		_ASSERTE(pMyBuffer != nullptr);

		auto* pDeviceContext = m_pD3DImmediateContext.Get();

		// 番兵などは要らないので、1つだけの場合は別に一時配列に入れなくても OK。
		const UINT numBuffers = 1;
		const UINT pStrides[numBuffers] = { vertexSizeInBytes };
		const UINT pOffsets[numBuffers] = { 0 };
		ID3D11Buffer* const ppVBuffers[numBuffers] = { pMyBuffer };
		pDeviceContext->IASetVertexBuffers(0, numBuffers, ppVBuffers, pStrides, pOffsets);
		pDeviceContext->IASetPrimitiveTopology(topologyType);
	}


	//! @brief  プリミティブを描画する。<br>
	//! @pre  頂点バッファが設定済み。<br>
	void MyD3DManager::DrawPrimitive(ID3DX11EffectTechnique* pTechnique, UINT vertexCount)
	{
		D3DX11_TECHNIQUE_DESC techDesc = {};
		pTechnique->GetDesc(&techDesc);

		auto* pDeviceContext = m_pD3DImmediateContext.Get();
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->Draw(vertexCount, 0);
		}
	}

	void MyD3DManager::DrawInstancedPrimitive(ID3DX11EffectTechnique* pTechnique, UINT vertexCountPerInstance, UINT instanceCount)
	{
		D3DX11_TECHNIQUE_DESC techDesc = {};
		pTechnique->GetDesc(&techDesc);

		auto* pDeviceContext = m_pD3DImmediateContext.Get();
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, 0, 0);
		}
	}

	//! @brief  インデックス付きプリミティブを描画する。<br>
	//! @pre  頂点バッファとインデックス バッファが設定済み。<br>
	void MyD3DManager::DrawIndexedPrimitive(ID3DX11EffectTechnique* pTechnique, UINT indexCount)
	{
		D3DX11_TECHNIQUE_DESC techDesc = {};
		pTechnique->GetDesc(&techDesc);

		auto* pDeviceContext = m_pD3DImmediateContext.Get();
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(indexCount, 0, 0);
		}
	}

	void MyD3DManager::DrawIndexedInstancedPrimitive(ID3DX11EffectTechnique* pTechnique, UINT indexCountPerInstance, UINT instanceCount)
	{
		D3DX11_TECHNIQUE_DESC techDesc = {};
		pTechnique->GetDesc(&techDesc);

		auto* pDeviceContext = m_pD3DImmediateContext.Get();
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexedInstanced(indexCountPerInstance, instanceCount, 0, 0, 0);
		}
	}

	void MyD3DManager::Dispatch(ID3DX11EffectTechnique* pTechnique, UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
	{
		D3DX11_TECHNIQUE_DESC techDesc = {};
		pTechnique->GetDesc(&techDesc);

		auto* pDeviceContext = m_pD3DImmediateContext.Get();
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
		}
	}


	namespace
	{

		void DrawMeshSubset(ID3D11DeviceContext* pDeviceContext, MyDeviceMeshPack* pMesh,
			ID3DX11EffectTechnique* pTechnique, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader* pPixelShader, size_t attribId,
			bool isToonShading, bool drawsToonInk, bool sproutsFur, MyMath::MySpecularType specularType, MyMath::MyReflexType reflexType)
		{
			D3DX11_TECHNIQUE_DESC techDesc = {};
			pTechnique->GetDesc(&techDesc);

			for (UINT p = 0; p < techDesc.Passes; ++p)
			{
				if (p == 0)
				{
					pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);

					// リフレクションなしで Dynamic Shader Linkage（動的シェーダーリンク）を使う場合、
					// つまり ID3DX11EffectInterfaceVariable::SetClassInstance() が機能しない場合、
					// ID3D11DeviceContext::XXSetShader() を明示的に呼び出す必要があるっぽい。
					// ちなみにリフレクションなしでも ID3D11ClassLinkage::GetClassInstance() は機能する。
					// なお、ID3DX11EffectPass::Apply() 内部で XXSetShader() が呼ばれるが、
					// このとき NumClassInstances パラメータにはゼロが渡されてしまうため、
					// interface を使っているシェーダーパスだと
					// "D3D11 ERROR: ID3D11DeviceContext::PSSetShader: Shader expects 1 class instances, application provided 0 [ STATE_SETTING ERROR #2097305: DEVICE_SETSHADER_INTERFACE_COUNT_MISMATCH]"
					// というエラーメッセージが出力されてしまう。
					// エフェクトのパスを使わずにすべて手動設定するのは不便なので、
					// 特定のシェーダーステージだけをホスト側で後から差し替えることができるようにするために
					// ダミーシェーダーやシェーダープールをエフェクト ファイル中に用意しておくとよい。
					// ちなみに interface を使っていないシェーダーに対して NumClassInstances パラメータに非ゼロを渡すと、
					// "D3D11 ERROR: ID3D11DeviceContext::PSSetShader: NumClassInstances should be zero for shaders that don't have interfaces. [ STATE_SETTING ERROR #2097305: DEVICE_SETSHADER_INTERFACE_COUNT_MISMATCH]"
					// というエラーメッセージが出力されてしまう。
					// 実際の描画時には最後に上書き設定したステートが使用されるため、致命的なエラーではないが、
					// デバッグ レイヤーからのエラーメッセージがうっとうしいので、
					// やはり interface の使用有無に応じて、ホスト側で常に適切なパラメータを渡してやる必要がある。

					if (pPixelShader && pClassLinkage)
					{
						// 屈折・反射の計算などは必要な場合のみ有効にするため、Dynamic Shader Linkage を使う。
						HRESULT hr = S_OK;
						// D3D11_CLASS_INSTANCE_DESC::InstanceId が cbuffer 内の定義順序に対応している模様。
						// ID3D11ClassLinkage::CreateClassInstance() でなく ID3D11ClassLinkage::GetClassInstance() で取得した場合、TypeId はゼロのままらしい。
						D3D11_CLASS_INSTANCE_DESC clsInstDesc = {};

						// HACK: ID3D11ClassInstance の取得（名前検索）は初期化時にやってしまう。

						static const char* const ppSpecularInstanceNames[] =
						{
							"UniInstanceSpecularDummy",
							"UniInstanceSpecularBlinnPhong",
							"UniInstanceSpecularCookTorrance",
						};
						_ASSERTE(0 <= specularType && specularType < ARRAYSIZE(ppSpecularInstanceNames));
						Microsoft::WRL::ComPtr<ID3D11ClassInstance> pSpecularInstance;
						hr = pClassLinkage->GetClassInstance(ppSpecularInstanceNames[specularType], 0, &pSpecularInstance);
						_ASSERTE(SUCCEEDED(hr));
						pSpecularInstance->GetDesc(&clsInstDesc);

						static const char* const ppColorPickerInstanceNames[] =
						{
							"UniInstanceEnvMapColorPickerDummy",
							"UniInstanceEnvMapColorPickerReflect",
							"UniInstanceEnvMapColorPickerRefract",
							"UniInstanceEnvMapColorPickerFresnel",
						};
						_ASSERTE(0 <= reflexType && reflexType < ARRAYSIZE(ppColorPickerInstanceNames));
						Microsoft::WRL::ComPtr<ID3D11ClassInstance> pColorPickerInstance;
						hr = pClassLinkage->GetClassInstance(ppColorPickerInstanceNames[reflexType], 0, &pColorPickerInstance);
						_ASSERTE(SUCCEEDED(hr));
						pColorPickerInstance->GetDesc(&clsInstDesc);

						static const char* const ppMainLightingShaderInstanceNames[] =
						{
							"UniInstanceMainLightingShaderPhotoReal",
							"UniInstanceMainLightingShaderToon",
						};
						const char* const pMainLightingShaderInstanceName = isToonShading
							? ppMainLightingShaderInstanceNames[1]
							: ppMainLightingShaderInstanceNames[0];
						Microsoft::WRL::ComPtr<ID3D11ClassInstance> pMainLightingShaderInstance;
						hr = pClassLinkage->GetClassInstance(pMainLightingShaderInstanceName, 0, &pMainLightingShaderInstance);
						_ASSERTE(SUCCEEDED(hr));
						pMainLightingShaderInstance->GetDesc(&clsInstDesc);

						Microsoft::WRL::ComPtr<ID3D11ClassInstance> pInstanceHogeA;
						hr = pClassLinkage->GetClassInstance("UniInstanceHogeA", 0, &pInstanceHogeA);
						_ASSERTE(SUCCEEDED(hr));
						pInstanceHogeA->GetDesc(&clsInstDesc);
						Microsoft::WRL::ComPtr<ID3D11ClassInstance> pInstanceHogeB;
						hr = pClassLinkage->GetClassInstance("UniInstanceHogeB", 0, &pInstanceHogeB);
						_ASSERTE(SUCCEEDED(hr));
						pInstanceHogeB->GetDesc(&clsInstDesc);
						Microsoft::WRL::ComPtr<ID3D11ClassInstance> pInstanceHogeC;
						hr = pClassLinkage->GetClassInstance("UniInstanceHogeC", 0, &pInstanceHogeC);
						_ASSERTE(SUCCEEDED(hr));
						pInstanceHogeC->GetDesc(&clsInstDesc);

						// HLSL コード中のインターフェイス ポインタに渡す実体（インスタンス）のリスト。
						// 複数のインターフェイスに対してそれぞれの実体を渡す場合、cbuffer 内の定義順に合わせればよいらしい？
						// D3DCompiler 経由のリフレクションが使えない場合、順序は HLSL コードと相談しながら決めるしかないはず。
						// → HLSL の定数バッファにおけるクラス インスタンス定義順序は関係ないらしい。
						// 各インターフェイスに対応するスロット番号は、やはりシェーダーリフレクションを使って調べる必要がある。
						// 有効なインターフェイス スロットの数は ID3D11ShaderReflection::GetNumInterfaceSlots() で取得。
						// 各インターフェイスのスロット番号は ID3D11ShaderReflectionVariable::GetInterfaceSlot() で取得。
						// ちなみに複数のインターフェイスを使うとき、ココでインスタンス数の設定をミスすると
						// デバッグ レイヤーからエラーメッセージが表示されるだけでなくメモリーリークまでもが発生する。
						// スロット番号（順序）を間違えただけの場合はエラーが表示される。
						ID3D11ClassInstance* const ppClsInsts[] =
						{
							pSpecularInstance.Get(),
							pMainLightingShaderInstance.Get(),
							pColorPickerInstance.Get(),
							//pInstanceHogeA.Get(), // シェーダーパス中で使っていないものは放置でよさげ。というか逆に含めるとエラーになる模様。
						};
						pDeviceContext->PSSetShader(pPixelShader, ppClsInsts, ARRAYSIZE(ppClsInsts));
						pMesh->DrawSubset(pDeviceContext, attribId);
					}
					else if (pPixelShader && !pClassLinkage)
					{
						pDeviceContext->PSSetShader(pPixelShader, nullptr, 0);
						pMesh->DrawSubset(pDeviceContext, attribId);
					}
					else
					{
						pMesh->DrawSubset(pDeviceContext, attribId);
					}
				}
				else if (p == 1 && drawsToonInk)
				{
					// 隣接情報を使ったトゥーン インクの描画。詳細はエフェクト ファイルを参照。
					pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
					pMesh->DrawSubset(pDeviceContext, attribId, MyCommon::TopologyType_TriangleListAdj);
				}
				else if (p == 2 && sproutsFur)
				{
					// テッセレーションを使ったファーの描画。詳細はエフェクト ファイルを参照。
					pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
					pMesh->DrawSubset(pDeviceContext, attribId, MyCommon::TopologyType_TrianglePatchList);
				}
			}
		}
	}


	void MyD3DManager::DrawMeshArray(ID3DX11EffectTechnique* pTechnique, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader* pPixelShader)
	{
		// マテリアルを持たないメッシュ用のデフォルト ディフューズ色。
		//static const MyMath::Vector4F defaultGrayDiffuseColor(0.5f, 0.5f, 0.5f, 1);
		static const MyMath::Vector4F defaultGrayDiffuseColor(1, 1, 1, 0.5f);

		_ASSERTE(m_pModelMeshInfoArray != nullptr);
		_ASSERTE(m_pAnimMixerArrayArray != nullptr);
		_ASSERTE(m_mainMeshArray.size() == m_pModelMeshInfoArray->size());
		_ASSERTE(m_mainMeshArray.size() == m_pAnimMixerArrayArray->size());

		auto pDeviceContext = this->GetMainThreadDeviceContext();

		for (size_t i = 0; i < m_mainMeshArray.size(); ++i)
		{
			// モーフ ターゲット（トゥイーニング ターゲット）のメッシュは自身の直接の描画をスキップする。
			// HACK: 現状、モーフ ターゲットとなる条件はメッシュ名のプレフィックスで判断しているが、このプレフィックスを固定名でなく GUI で指定できるようにする。
			// FBX シェイプ アニメーションの機能をインポートできるようにする？

			const auto* pModelMeshInfo = (*m_pModelMeshInfoArray)[i].get();
			//if (strncmp(pModelMeshInfo->GetMeshName(), MyCommon::MorphTargetMeshPrefixName, MyCommon::MorphTargetMeshPrefixNameLen) == 0)
			if (MyCommon::CheckHasMeshMorphTargetPrefixName(pModelMeshInfo->GetMeshNameW().c_str()))
			{
				continue;
			}
#if 1
			// いったん配列のすべての要素を単位行列化する。
			// デュアル クォータニオン配列も単位化する。
			this->IdentifyAllSkinningBonePalette();
#endif

			// メッシュに対応するミキサーを使って、現在のブレンド済みボーン行列を取得する。
			// まず現在のフレームのデュアル クォータニオン パレットを取得。
			// フレーム／アニメーション間の補間やブレンドは行列よりもクォータニオンのほうが向いている（球面線形補間）。
			// シェーダー側をデュアル クォータニオン対応にするのであれば、行列への変換は不要。
			// しかし定数バッファに余裕があるのであれば、GPU 側で毎回計算するよりも、CPU 側で一度行列に変換してしまったほうが良い。
			const auto& pAnimMixerArray = (*m_pAnimMixerArrayArray)[i];
			_ASSERTE(pAnimMixerArray);
			pModelMeshInfo->GetCurrentFrameBoneQuatsArray(
				m_boneQuatPalette.BoneQuats, *pAnimMixerArray);
			m_boneQuatPalette.ConvertToMatrices(m_boneMatrixPalette.BoneMatrices);

			// HLSL 用に転置する。
			// HLSL は既定で Column-Major だが、D3DX Math, XNA Math, DirectXMath は Row-Major を使う。
			// HACK: HLSL で row_major を指定して、C++ 側に合わせる。
			// row_major でも column_major でも、性能に大きな差は出ないはず。
			// ただ、定数レジスタを節約する場合は column_major のほうがいいかも。
			// http://wlog.flatlib.jp/item/975
			//m_boneMatrixPalette.TransposeAllMatrices();

			const int mainTexSlotIndex = 0;
			// シェーダー側にボーン行列パレットを転送。
			this->UpdateCBuffer(&m_boneMatrixPalette);

			auto* pMesh = m_mainMeshArray[i].get();
			const size_t attrSize = pMesh->GetAttributeRangeArray().size();
			// 属性テーブルに応じて分岐する。ID3DX10Mesh::GetAttributeTable(nullptr, &attrSize) に相当。
			CBufferMeshPartAttributePack shaderMeshParam;
			const bool isToon = m_myEffectSettings.EnablesToonShading;
			if (attrSize == 0)
			{
				// 属性テーブルが空の場合。
				// 特定のパラメータは既定値を使い、他はすべてゼロとする。
				shaderMeshParam.MaterialColorDiffuse = defaultGrayDiffuseColor;
				shaderMeshParam.MaterialOpacityAlpha = 1;
				shaderMeshParam.MaterialSpecularPower = MyMath::MinSpecularPowerValue;
				shaderMeshParam.MaterialIndexOfRefraction = 1;
				const int toonMaterialIndex = 0;
				shaderMeshParam.ToonShadingRefTexV =
					MyTextureHelper::CalcBilinearToonShadingGradientRefTexCoordV(toonMaterialIndex);
				this->UpdateCBuffer(&shaderMeshParam);
				this->FindTextureAndSetToPS(mainTexSlotIndex, nullptr);
				DrawMeshSubset(pDeviceContext, pMesh, pTechnique, pClassLinkage, pPixelShader, 0, isToon, isToon, false, MyMath::MySpecularType_None, MyMath::MyReflexType_None);
			}
			else
			{
				for (size_t atr = 0; atr < attrSize; ++atr)
				{
					bool sproutsFur = false;
					auto reflexType = MyMath::MyReflexType_None;
					auto specularType = MyMath::MySpecularType_None;
					if (!pModelMeshInfo->GetMaterialsArray().empty())
					{
						// マテリアル インデックス配列が空でも、マテリアルは1個以上存在する場合を考慮する。
						// とりあえず先頭のマテリアルを使う。それともグレーのデフォルト マテリアルにしたほうがいいか？
						// なお、逆にマテリアル インデックス配列が空でないのにマテリアルがゼロということもありえるらしい。
						// FBX Converter 付属の LocalMotionBlend.fbx がその例。
						const int matIndex = (atr < pModelMeshInfo->GetMaterialIndicesArrayForAttribTable().size()) ?
							pModelMeshInfo->GetMaterialIndicesArrayForAttribTable()[atr] : 0;
						_ASSERTE(0 <= matIndex && static_cast<size_t>(matIndex) < pModelMeshInfo->GetMaterialsArray().size());
						const auto& mat = *pModelMeshInfo->GetMaterialsArray()[matIndex].get();
#if 0
						// 旧バージョンの MQO FBX Exporter 用コード。
						if (m_myEffectSettings.EnablesToonShading &&
							!mat.TexFileNameDiffuse.empty())
						{
							// ディフューズ マテリアル カラーは無視して、テクスチャ色をそのまま使う。
							shaderMeshParam.MaterialColorDiffuse =
								MyMath::Vector4F(1, 1, 1, mat.GetDiffuse().w);
						}
						else
						{
							shaderMeshParam.MaterialColorDiffuse = mat.GetDiffuse();
						}
						shaderMeshParam.MaterialColorSpecular = MyMath::Vector4F(1, 1, 1, 32); // テスト用。
#endif
						shaderMeshParam.MaterialColorDiffuse = mat.GetDiffuse();
						shaderMeshParam.MaterialColorAmbient = mat.GetAmbient();
						shaderMeshParam.MaterialColorSpecular = mat.GetSpecular();
						shaderMeshParam.MaterialColorEmissive = mat.GetEmissive();
						shaderMeshParam.MaterialOpacityAlpha = mat.GetOpacityAlpha();
						shaderMeshParam.MaterialSpecularPower = mat.GetSpecularPower();
						shaderMeshParam.UniMaterialRoughness = mat.GetRoughness();
						shaderMeshParam.MaterialReflectivity = mat.GetReflectivity();
						shaderMeshParam.MaterialIndexOfRefraction = mat.GetIndexOfRefraction();
						const int toonMaterialIndex = 1;
						shaderMeshParam.ToonShadingRefTexV =
							MyTextureHelper::CalcBilinearToonShadingGradientRefTexCoordV(toonMaterialIndex);
						this->FindTextureAndSetToPS(mainTexSlotIndex, &mat.GetTexFileNameDiffuseMap());
						sproutsFur = mat.GetSproutsFur();
						if (shaderMeshParam.MaterialColorSpecular.w != 0)
						{
							specularType = mat.GetSpecularType();
						}
						if (shaderMeshParam.MaterialReflectivity != 0)
						{
							reflexType = mat.GetReflexType();
						}
					}
					else
					{
						// マテリアルが存在しない場合。
						shaderMeshParam.MaterialColorDiffuse = defaultGrayDiffuseColor;
						shaderMeshParam.MaterialColorAmbient = MyMath::ZERO_VECTOR4F;
						shaderMeshParam.MaterialColorSpecular = MyMath::ZERO_VECTOR4F;
						shaderMeshParam.MaterialColorEmissive = MyMath::ZERO_VECTOR4F;
						shaderMeshParam.MaterialOpacityAlpha = 1;
						shaderMeshParam.MaterialSpecularPower = MyMath::MinSpecularPowerValue;
						shaderMeshParam.UniMaterialRoughness = 0;
						shaderMeshParam.MaterialReflectivity = 0;
						shaderMeshParam.MaterialIndexOfRefraction = 1;
						const int toonMaterialIndex = 1;
						shaderMeshParam.ToonShadingRefTexV =
							MyTextureHelper::CalcBilinearToonShadingGradientRefTexCoordV(toonMaterialIndex);
						this->FindTextureAndSetToPS(mainTexSlotIndex, nullptr);
					}
					this->UpdateCBuffer(&shaderMeshParam);
					DrawMeshSubset(pDeviceContext, pMesh, pTechnique, pClassLinkage, pPixelShader, atr, isToon, isToon, sproutsFur, specularType, reflexType);
				}
			}
		}
	}


	void MyD3DManager::GetTechniquesFromEffect()
	{
		// エフェクト中のテクニック名を指定してインターフェイスを取得する。

		_ASSERTE(m_pMainEffect != nullptr);
		_ASSERTE(m_pComputeEffect != nullptr);
		_ASSERTE(m_pScreenEffect != nullptr);
		_ASSERTE(m_pShadowEffect != nullptr);

		// GetTechniqueByName() の代わりに GetTechniqueByIndex() を使うこともできる。
		// インデックスはエフェクト ファイルでテクニックが定義されている順序。
		// technique が見つからない場合、NULL ではなく無効値を表すダミーオブジェクトへのポインタが返るため、IsValid() を調べる必要がある。

		m_effectTechs.TechLambertPCNT = m_pMainEffect->GetTechniqueByName("TechLambertPCNT");
		_ASSERTE(m_effectTechs.TechLambertPCNT->IsValid());

		m_effectTechs.TechRenderLightDirLine = m_pMainEffect->GetTechniqueByName("TechRenderLightDirLine");
		_ASSERTE(m_effectTechs.TechRenderLightDirLine->IsValid());

		m_effectTechs.TechTessBillboard = m_pMainEffect->GetTechniqueByName("TechTessBillboard");
		_ASSERTE(m_effectTechs.TechTessBillboard->IsValid());

		m_effectTechs.TechGeomBillboard = m_pMainEffect->GetTechniqueByName("TechGeomBillboard");
		_ASSERTE(m_effectTechs.TechGeomBillboard->IsValid());

		m_effectTechs.TechFlakeParticle = m_pMainEffect->GetTechniqueByName("TechFlakeParticle");
		_ASSERTE(m_effectTechs.TechFlakeParticle->IsValid());

		m_effectTechs.TechRenderCoordAxisLines = m_pMainEffect->GetTechniqueByName("TechRenderCoordAxisLines");
		_ASSERTE(m_effectTechs.TechRenderCoordAxisLines->IsValid());

		m_effectTechs.TechFont = m_pMainEffect->GetTechniqueByName("TechFont");
		_ASSERTE(m_effectTechs.TechFont->IsValid());

		m_effectTechs.TechShadowHudTest = m_pMainEffect->GetTechniqueByName("TechShadowHudTest");
		_ASSERTE(m_effectTechs.TechShadowHudTest->IsValid());

		m_effectTechs.TechSkinning = m_pMainEffect->GetTechniqueByName("TechSkinning");
		_ASSERTE(m_effectTechs.TechSkinning->IsValid());

		m_effectTechs.TechImageBasedFur = m_pMainEffect->GetTechniqueByName("TechImageBasedFur");
		_ASSERTE(m_effectTechs.TechImageBasedFur->IsValid());

		m_effectTechs.TechAddDownSampledTex = m_pMainEffect->GetTechniqueByName("TechAddDownSampledTex");
		_ASSERTE(m_effectTechs.TechAddDownSampledTex->IsValid());

		m_effectTechs.TechDisplaySimpleComputingTest = m_pMainEffect->GetTechniqueByName("TechDisplaySimpleComputingTest");
		_ASSERTE(m_effectTechs.TechDisplaySimpleComputingTest->IsValid());

		m_effectTechs.TechEdgeDetectColorSketch = m_pMainEffect->GetTechniqueByName("TechEdgeDetectColorSketch");
		_ASSERTE(m_effectTechs.TechEdgeDetectColorSketch->IsValid());

#pragma region // Compute
		m_effectTechs.CSTechReductionTexture2DTo1D = m_pComputeEffect->GetTechniqueByName("CSTechReductionTexture2DTo1D");
		_ASSERTE(m_effectTechs.CSTechReductionTexture2DTo1D->IsValid());

		m_effectTechs.CSTechReductionTexture1DTo1D = m_pComputeEffect->GetTechniqueByName("CSTechReductionTexture1DTo1D");
		_ASSERTE(m_effectTechs.CSTechReductionTexture1DTo1D->IsValid());

		m_effectTechs.CSTechSimpleComputingTest = m_pComputeEffect->GetTechniqueByName("CSTechSimpleComputingTest");
		_ASSERTE(m_effectTechs.CSTechSimpleComputingTest->IsValid());

		m_effectTechs.CSTechUpdateRandomTable = m_pComputeEffect->GetTechniqueByName("CSTechUpdateRandomTable");
		_ASSERTE(m_effectTechs.CSTechUpdateRandomTable->IsValid());

		m_effectTechs.CSTechUpdateFlakeParticle = m_pComputeEffect->GetTechniqueByName("CSTechUpdateFlakeParticle");
		_ASSERTE(m_effectTechs.CSTechUpdateFlakeParticle->IsValid());

		m_effectTechs.CSTechApplyGaussianBlurToDownSampledTex = m_pComputeEffect->GetTechniqueByName("CSTechApplyGaussianBlurToDownSampledTex");
		_ASSERTE(m_effectTechs.CSTechApplyGaussianBlurToDownSampledTex->IsValid());

		m_effectTechs.CSTechApplyBlurToShadowTex = m_pComputeEffect->GetTechniqueByName("CSTechApplyBlurToShadowTex");
		_ASSERTE(m_effectTechs.CSTechApplyBlurToShadowTex->IsValid());
#pragma endregion

#pragma region // Screen
		m_effectTechs.TechApplyHorizontalMovingAverageFilterToShadowTex = m_pScreenEffect->GetTechniqueByName("TechApplyHorizontalMovingAverageFilterToShadowTex");
		_ASSERTE(m_effectTechs.TechApplyHorizontalMovingAverageFilterToShadowTex->IsValid());

		m_effectTechs.TechApplyVerticalMovingAverageFilterToShadowTex = m_pScreenEffect->GetTechniqueByName("TechApplyVerticalMovingAverageFilterToShadowTex");
		_ASSERTE(m_effectTechs.TechApplyVerticalMovingAverageFilterToShadowTex->IsValid());

		m_effectTechs.TechApplyHorizontalGaussianBlurToDownSampledTex = m_pScreenEffect->GetTechniqueByName("TechApplyHorizontalGaussianBlurToDownSampledTex");
		_ASSERTE(m_effectTechs.TechApplyHorizontalGaussianBlurToDownSampledTex->IsValid());

		m_effectTechs.TechApplyVerticalGaussianBlurToDownSampledTex = m_pScreenEffect->GetTechniqueByName("TechApplyVerticalGaussianBlurToDownSampledTex");
		_ASSERTE(m_effectTechs.TechApplyVerticalGaussianBlurToDownSampledTex->IsValid());

		m_effectTechs.TechTransportFromMSAA8 = m_pScreenEffect->GetTechniqueByName("TechTransportFromMSAA8");
		_ASSERTE(m_effectTechs.TechTransportFromMSAA8->IsValid());
		m_effectTechs.TechTransportFromMSAA4 = m_pScreenEffect->GetTechniqueByName("TechTransportFromMSAA4");
		_ASSERTE(m_effectTechs.TechTransportFromMSAA4->IsValid());
		m_effectTechs.TechTransportFromMSAA2 = m_pScreenEffect->GetTechniqueByName("TechTransportFromMSAA2");
		_ASSERTE(m_effectTechs.TechTransportFromMSAA2->IsValid());
		m_effectTechs.TechTransportFromNonMSAA = m_pScreenEffect->GetTechniqueByName("TechTransportFromNonMSAA");
		_ASSERTE(m_effectTechs.TechTransportFromNonMSAA->IsValid());

		m_effectTechs.TechExtractHighIntensityFromMSAA8 = m_pScreenEffect->GetTechniqueByName("TechExtractHighIntensityFromMSAA8");
		_ASSERTE(m_effectTechs.TechExtractHighIntensityFromMSAA8->IsValid());
		m_effectTechs.TechExtractHighIntensityFromMSAA4 = m_pScreenEffect->GetTechniqueByName("TechExtractHighIntensityFromMSAA4");
		_ASSERTE(m_effectTechs.TechExtractHighIntensityFromMSAA4->IsValid());
		m_effectTechs.TechExtractHighIntensityFromMSAA2 = m_pScreenEffect->GetTechniqueByName("TechExtractHighIntensityFromMSAA2");
		_ASSERTE(m_effectTechs.TechExtractHighIntensityFromMSAA2->IsValid());
		m_effectTechs.TechExtractHighIntensityFromNonMSAA = m_pScreenEffect->GetTechniqueByName("TechExtractHighIntensityFromNonMSAA");
		_ASSERTE(m_effectTechs.TechExtractHighIntensityFromNonMSAA->IsValid());

		m_effectTechs.TechReduction2x2 = m_pScreenEffect->GetTechniqueByName("TechReduction2x2");
		_ASSERTE(m_effectTechs.TechReduction2x2->IsValid());
		m_effectTechs.TechReduction4x4 = m_pScreenEffect->GetTechniqueByName("TechReduction4x4");
		_ASSERTE(m_effectTechs.TechReduction4x4->IsValid());
		m_effectTechs.TechReduction8x8 = m_pScreenEffect->GetTechniqueByName("TechReduction8x8");
		_ASSERTE(m_effectTechs.TechReduction8x8->IsValid());
#pragma endregion

		m_effectTechs.TechShadowRender = m_pShadowEffect->GetTechniqueByName("TechShadowRender");
		_ASSERTE(m_effectTechs.TechShadowRender->IsValid());

		// Effects11 を使わない場合、ID3D11Device::CreateClassLinkage() を使って明示的に生成した ID3D11ClassLinkage を、
		// シェーダーを生成するときに CreatePixelShader() などの生成メソッドの第3引数に渡す必要がある。
		// http://msdn.microsoft.com/ja-jp/library/ee422095.aspx
		// Effects11 を使う場合は、暗黙的に生成が行なわれるので、それを GetClassLinkage() を使って後から取得する。
		// 
		// GetTechniqueByName() や GetVariableByName() の戻り値で取得した D3DX のインターフェイスは解放不要（参照カウントを増やさない）だが、
		// GetClassLinkage() は別物らしい（参照カウントが増える）。
		// 取得した D3D インターフェイスの Release() を呼んで解放しないとデバッグ レイヤーがメモリーリークを報告する。
		// 紛らわしいので戻り値ではなく引数にして欲しいものだが……
		// 戻り値が ID3DX11xxx の場合（D3DX11 ライブラリのインターフェイスの場合）は解放不要で、
		// 戻り値が ID3D11xxx の場合（D3D11 コア ライブラリのインターフェイスの場合）は解放が必要、ということらしい。

#if 0
		_ASSERTE(!m_pMainEffectClassLinkage);
		m_pMainEffectClassLinkage.Attach(m_pMainEffect->GetClassLinkage());
		_ASSERTE(m_pMainEffectClassLinkage);
#endif

#if 0
		{
			// 指定された名前のエフェクト変数が定義されていない場合、
			// Effects 11 ライブラリは "ID3DX11Effect::GetVariableByName: Variable [XXX] not found"
			// というデバッグ メッセージを出力するが、GetVariableByName() の返却値は有効値（非 NULL）が返る。
			// AsInterface() や AsVector() などを使ってキャストしても定義・未定義を判定できない。
			// 実際に定義されているかどうかを判定するには、IsValid() を使う。
			// http://msdn.microsoft.com/ja-jp/library/ee416201.aspx
			// D3DCompiler 依存を切ってシェーダーリフレクションが無効になっている場合、
			// SetClassInstance() その他はうまく動作しない模様。
			// http://msdn.microsoft.com/ja-jp/library/ee416155.aspx

			auto pInstanceVariable0 = m_pMainEffect->GetVariableByName("UniInstanceMainLightingShaderPhotoReal")->AsClassInstance();
			if (!pInstanceVariable0->IsValid())
			{
				ATLTRACE("NG!\n");
			}
			auto pInstanceVariable1 = m_pMainEffect->GetVariableByName("UniInstanceMainLightingShaderToon")->AsClassInstance();
			if (!pInstanceVariable1->IsValid())
			{
				ATLTRACE("NG!\n");
			}
			auto pInterfaceVariable = m_pMainEffect->GetVariableByName("AbstractMainLightingShader")->AsInterface();
			if (!pInterfaceVariable->IsValid())
			{
				ATLTRACE("NG!\n");
			}
			//pInterfaceVariable->SetClassInstance(pInstanceVariableDummy);
		}
#endif
	}


	//! @brief  初期化処理（ポスト コンストラクタ）。<br>
	bool MyD3DManager::Create(UINT width, UINT height, HWND hWnd)
	{
		HRESULT hr = E_FAIL;

		hr = m_myUIAnimCenter.InitializeAnimationInterfaces();
		if (FAILED(hr))
		{
			return false;
		}

#pragma region // D3D デバイスとスワップ チェーンを作成する。//
		// ここで指定するスワップ チェーンのフォーマットが、バック バッファとして使われるテクスチャのフォーマットになる。
		// FP16 フォーマットなどを指定することで、バック バッファに直接 HDR レンダリングすることもできるが、
		// 最終的に従来の 32bit カラーモニターに表示する際の発色が異なるので注意。
		// オフスクリーン描画に HDR 浮動小数フォーマットを使うときでも、バック バッファには従来の
		// B8G8R8A8_UNORM に相当する互換フォーマットを使って、
		// 最後にシェーダーで HDR --> LDR 転送するようにしておいたほうがいい。

		// Direct2D との相互運用を行なう場合、
		// Direct3D のスワップ チェーン バッファに直接 Direct2D で書き込むことも可能だが、
		// Direct2D コンテンツを DXGI サーフェイス経由でいったん Direct3D レンダーターゲット テクスチャに書き込んで、
		// それを Direct3D レンダリングに利用する方法もある。
		// どのみち Direct3D スワップ チェーンは必要。

		// Windows ストア アプリで DirectX と XAML の相互運用を行なう場合、
		// パフォーマンスを重視するならば SwapChainPanel を使うらしい。
		// DXGI 1.2 の IDXGIFactory2::CreateSwapChainForComposition() がキモらしい。
		// http://msdn.microsoft.com/ja-jp/library/windows/apps/hh825871.aspx
		// ストア アプリとデスクトップ アプリとで、レンダリングの準備に関するソースコードも部分的に共有するつもりならば、
		// デスクトップにおいてもスワップ チェーンを明示的に作成するようにしておいたほうがよい。
		// XAML 連携を行なわない場合、
		// デスクトップでは D3D11CreateDevice() と IDXGIFactory2::CreateSwapChainForHwnd() の組み合わせを用いる。
		// ストア アプリでは D3D11CreateDevice() と IDXGIFactory2::CreateSwapChainForCoreWindow() の組み合わせを用いる。

		// DXGI アダプターを列挙して、複数の GPU が接続されている場合、デフォルトではプライマリ アダプターを使えばよいが、
		// オプションでユーザーが描画に使用する GPU を選択できるとよい。Windows ストア アプリではどうなる？

		// マルチ GPU やマルチ モニター環境で動作する DirectX Graphics プログラムの挙動をカスタマイズにするためには、
		// アダプターとアウトプットを列挙して選択する。
		// GPU に直結しているモニターだけでなく、直結していないモニターに対しても出力を指示するようなこともできるらしい。
		// http://wlog.flatlib.jp/item/1326

#if 1
		Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory1_1; // DXGI 1.1 ファクトリ。
		//hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory.ReleaseAndGetAddressOf()));
		hr = CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory1_1.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create DXGI factory!!"), hr);
			return false;
		}
		// DXGI 1.2 対応をチェック。機能レベルではなくインターフェイス（ランタイム）のテストのみ。
		Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory1_2; // DXGI 1.2 ファクトリ。
		hr = dxgiFactory1_1.As(&dxgiFactory1_2);
		DXTRACE_ERR(_T("IDXGIFactory2 from IDXGIFactory1?"), hr);
		if (FAILED(hr) || !dxgiFactory1_2)
		{
			return false;
		}
#if (WINVER >= 0x0603) // Windows 8.1 以降。
#ifdef _DEBUG
		const UINT DxgiFlag = DXGI_CREATE_FACTORY_DEBUG;
#else
		const UINT DxgiFlag = 0;
#endif
		Microsoft::WRL::ComPtr<IDXGIFactory3> dxgiFactory1_3; // DXGI 1.3 ファクトリ。
		// CreateDXGIFactory2() は、Windows 7 SP1 + Platform Update および Windows 8 の dxgi.dll には実装されていない。
		hr = CreateDXGIFactory2(DxgiFlag, IID_PPV_ARGS(dxgiFactory1_3.ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create DXGI factory!!"), hr);
			return false;
		}
#endif
		Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiPrimaryAdapter1_1;
		dxgiFactory1_1->EnumAdapters1(0, &dxgiPrimaryAdapter1_1);
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to get primary DXGI adapter!!"), hr);
			return false;
		}
		Microsoft::WRL::ComPtr<IDXGIAdapter2> dxgiPrimaryAdapter1_2;
		hr = dxgiPrimaryAdapter1_1.As(&dxgiPrimaryAdapter1_2);
		DXTRACE_ERR(_T("IDXGIAdapter2 from IDXGIAdapter1?"), hr);
		if (FAILED(hr) || !dxgiPrimaryAdapter1_2)
		{
			return false;
		}
#endif

#if 0
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.Windowed = true;
#else
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif
		// Web 上のサンプルではなぜかバック バッファに浮動小数バッファ DXGI_FORMAT_R16G16B16A16_FLOAT を直接使っているものがあるが、
		// 発色がかなり異なるものになる。浮動小数レンダリングは通例専用のレンダーターゲット テクスチャを明示的に作成し、
		// テクスチャ レンダリングによって実施するべきもの。
		// バックバッファに MSAA を使う場合、まずハードウェアのマルチサンプル能力を調べる必要があるので、
		// デバイスとスワップ チェーンの作成を分離する。
		// なお、バック バッファに直接 MSAA を使わない場合、デバイスとスワップ チェーンの作成を分離する必要はない。
		// 今回は、バック バッファへのレンダリング時に MSAA は不要。
		// 明示的に作成したターシャリ バッファ（レンダーターゲット テクスチャ）への描画時に MSAA を適用し、
		// テクスチャの内容をバック バッファへ転送する際にシェーダーでダウンサンプルする。
		// つまり、スワップ チェーンの作成後に MSAA 能力を調べることができるので、デバイス作成を分離する必要はない。
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1; // バックバッファは1つだけ。
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#else
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#endif
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		// ウィンドウ モード専用の場合は、DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH を特に設定する必要はない。
		// ちなみに Windows ストア アプリでは、DXGI_SWAP_EFFECT_DISCARD や DXGI_SWAP_EFFECT_SEQUENTIAL が使えず、
		// DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL しか使えないらしい。
		// DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL を使う場合、Format にも制約が発生する。
		// http://msdn.microsoft.com/en-us/library/windows/desktop/hh404528.aspx
		// DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL は DXGI 1.2 で追加されたメンバーで、IDXGISwapChain1::Present1() 専用のはず。
		// http://msdn.microsoft.com/en-us/library/windows/desktop/bb173077.aspx
		// また、IDXGISwapChain::Present() メソッドの第一引数で VSync を制御することができないらしい。
		// http://shikihuiku.wordpress.com/2012/11/13/storeapp%E3%81%AE%E5%9E%82%E7%9B%B4%E5%90%8C%E6%9C%9F/

		// Direct3D 11 デバイスの初期化。
		// BGRA サポートのチェックは Direct3D リソースとの相互運用を Direct2D で実現するために必要。
		// Direct2D がサポートする DXGI フォーマット自体は BGRA 以外に RGBA もある。
#ifdef _DEBUG
		const UINT devCreationFlag = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
		const UINT devCreationFlag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif
		IDXGIAdapter* pAdapter = dxgiPrimaryAdapter1_2.Get();

		// Direct3D 10 では非 NULL の DXGI Adapter を指定する場合も D3D_DRIVER_TYPE_HARDWARE を指定することができていたが、
		// Direct3D 11 では非 NULL の DXGI Adapter を指定する場合は D3D_DRIVER_TYPE_UNKNOWN を指定する必要があるらしい。
		//const auto driverType = D3D_DRIVER_TYPE_HARDWARE;
		//const auto driverType = D3D_DRIVER_TYPE_WARP;
		const auto driverType = pAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;

		auto featureLevel = D3D_FEATURE_LEVEL();

		// Direct3D デバイスを作成。
		// ココでアダプターに NULL を渡すと、内部で DXGI ファクトリが自動作成されるらしい。
		// デフォルトの NULL アダプターを使って D3D10CreateDevice1() により Direct3D 10.1 デバイスを作成し、さらに
		// DXGI ファクトリ 1.0 を使って IDXGIFactory::CreateSwapChain() によりスワップ チェーンを作成する場合、
		// SwapChain 作成時にデバイス生成とスワップ チェーン生成とで異なるファクトリが使用された、という旨の DXGI Warning が発生していたが、
		// DXGI ファクトリ 1.1 と Direct3D 11.0 デバイスの組み合わせであれば、そういった警告は出ない模様。
		// ただ、一応明示的にアダプターを指定しておいたほうがいいかも。
		// ただしアダプターを明示的に列挙するコードを実装するならば、マルチ GPU をテストできる環境でないと、真の意味でテストにならない。

		// pFeatureLevels は候補の機能レベルの配列を指定する箇所だが、
		// pFeatureLevels に NULL を指定すると、D3D_FEATURE_LEVEL 列挙型の中から、ハードウェアが対応する最高レベルを自動選択してくれる。
		// ただし、D3D_FEATURE_LEVEL_11_1 は含まれないらしい。
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476082.aspx
		// ターゲット環境の機能レベルが不明で、なおかつ
		// MRT、ジオメトリ シェーダー、テッセレーションなどを使うために特定の機能レベルを確保したい場合は、
		// 必ず適切な機能レベル値の配列を明示指定したり、出力である pFeatureLevel の結果をチェックしたりすること。
#if 0
		hr = D3D11CreateDeviceAndSwapChain(
			pAdapter,
			driverType,
			nullptr,
			devCreationFlag,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			m_pSwapChain.ReleaseAndGetAddressOf(),
			m_pD3DDevice.ReleaseAndGetAddressOf(),
			&featureLevel,
			m_pD3DImmediateContext.ReleaseAndGetAddressOf());
#else
		hr = D3D11CreateDevice(
			pAdapter,
			driverType,
			nullptr,
			devCreationFlag,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			m_pD3DDevice.ReleaseAndGetAddressOf(),
			&featureLevel,
			m_pD3DImmediateContext.ReleaseAndGetAddressOf());
#endif
		ATLTRACE(__FILE__"(%d): Max Feature Level = 0x%08x\n", __LINE__, featureLevel);
		if (featureLevel < D3D_FEATURE_LEVEL_11_0)
		{
			DXTRACE_ERR(_T("H/W does not support Direct3D 11.0!!"), hr);
			return false;
		}

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create D3D device!!"), hr);
			return false;
		}

		// Direct3D デバイスの基になっている DXGI デバイスを取得する。
		Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice1_1;
		hr = m_pD3DDevice.As(&dxgiDevice1_1);
		if (FAILED(hr) || !dxgiDevice1_1)
		{
			DXTRACE_ERR(_T("Failed to get DXGI device!!"), hr);
			return false;
		}
		{
			Microsoft::WRL::ComPtr<IDXGIDevice2> dxgiDevice1_2;
			HRESULT hr = dxgiDevice1_1.As(&dxgiDevice1_2);
			DXTRACE_ERR(_T("IDXGIDevice2 from IDXGIDevice1?"), hr);
			if (FAILED(hr) || !dxgiDevice1_2)
			{
				return false;
			}
		}

		{
			UINT msaaNumQualityLevels = 0;
			// 候補となる MSAA サンプル数の列挙。最も高品位なものが使われる。
			const UINT candSampleCounts[] = { 8, 4, 2, 1 };
			//const UINT candSampleCounts[] = { 4, 2, 1 };
			//const UINT candSampleCounts[] = { 2, 1 };
			//const UINT candSampleCounts[] = { 1 };

			m_msaaSampleCount = 1;
			m_msaaQuality = 0;

			// とりあえずメインとなる FP16 レンダーターゲットに対する MSAA 能力を調べる。
			if (!GetBestMSAAProperties(m_pD3DDevice.Get(), renderTargetTexFormat,
#if 0
				nullptr, 0,
#else
				candSampleCounts, ARRAYSIZE(candSampleCounts),
#endif
				m_msaaSampleCount, msaaNumQualityLevels, m_msaaQuality))
			{
				return false;
			}
		}
#if 0
		hr = dxgiFactory1_1->CreateSwapChain(m_pD3DDevice.Get(), &swapChainDesc, m_pSwapChain.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create DXGI swap chain!!"), hr);
			return false;
		}
#else
		hr = dxgiFactory1_2->CreateSwapChainForHwnd(m_pD3DDevice.Get(), hWnd, &swapChainDesc, nullptr, nullptr, m_pSwapChain.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create DXGI swap chain!!"), hr);
			return false;
		}
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#else
		dxgiDevice1_1->SetMaximumFrameLatency(1);
#endif

		// ATL::CComQIPtr にある暗黙のキャスト演算子による QueryInterface() 機能は、WRL::ComPtr には存在しないらしい。
		// ATL::CComQIPtr を使うと、
		//   ATL::CComPtr<IUnknown> pUnknown;
		//   SomeFactoryFunc(&pUnknown);
		//   ATL::CComQIPtr<TDerived> pDerived = pUnknown; // dynamic_cast 相当。
		// という書き方が可能だった。
		// また、ComPtr 経由の QueryInterface() 呼び出しも隠蔽されることになるが、
		// その代わり、同等機能として ComPtr::CopyTo() および ComPtr::As() メソッド テンプレートが使えるらしい。
		// 後者は C# の as 演算子に似せたものらしい？
		// なお、DXGI 1.2 / D3D 11.1 のバックポートを行なう Win7 プラットフォーム更新プログラム KB2670838 がインストールされていない場合、
		// もしくは Windows Vista の場合、
		// DXGI 1.2 / D3D 11.1 インターフェイスの取得（≒dynamic_cast）は E_NOINTERFACE で失敗する。

		// D3D 11.1 対応をチェック。
		{
			Microsoft::WRL::ComPtr<ID3D11Device1> pD3D11Device1;
			HRESULT hr = m_pD3DDevice.As(&pD3D11Device1);
			DXTRACE_ERR(_T("ID3D11Device1 from ID3D11Device?"), hr);
			if (FAILED(hr) || !pD3D11Device1)
			{
				return false;
			}

			// 名前が分かりにくいが、下記は D3D 11.1, 11.2 の Features チェック用の構造体と列挙値らしい。
			// Direct2D 1.1 と連携するだけならば、D3D 11.1 オプションは不要。
			// ちなみに Windows SDK 8.1 では、D3D 11.2 Feature のチェック用構造体 D3D11_FEATURE_DATA_D3D11_OPTIONS1 が追加されている。
			// やはり非常に分かりづらい。なぜ末尾番号をマイナーバージョンに合わせないのか……
			// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476124.aspx

			// Fermi 世代ではドライバーを更新しても D3D 11.1 オプションは一切サポートされないらしい？
			// Windows 7 だとダメ？
			// Kepler であれば ClearView などはサポートしている模様。
			// Kepler 第1世代（GK 106, GK 104）でも低精度シェーダーはサポートしてないらしい。
			// http://sfpgmr.hatenablog.jp/entry/geforce-650-ti-boost%E3%81%AE-directx11-1%E3%82%B5%E3%83%9D%E3%83%BC%E3%83%88%E5%BA%A6%E5%90%88%E3%82%92%E8%AA%BF%E3%81%B9%E3%82%8B
			// 頂点シェーダー／ジオメトリ シェーダー／テッセレーション シェーダーにおける UAV は DX 11.1 Feature で、
			// Fermi/Kepler においてはハードウェア レベルではサポートするものの、DirectX 11.1 API 経由では使えない模様。
			// DirectX 11.2 が確実に使えると思われる Maxwell 第2世代を待つしかなさげ。
			// http://nvidia.custhelp.com/app/answers/detail/a_id/3196/~/fermi-and-kepler-directx-api-support
			D3D11_FEATURE_DATA_D3D11_OPTIONS options11_1 = {};
			pD3D11Device1->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &options11_1, sizeof(options11_1));
			ATLTRACE("D3D11_FEATURE_DATA_D3D11_OPTIONS.OutputMergerLogicOp = %d\n", options11_1.OutputMergerLogicOp);
			ATLTRACE("D3D11_FEATURE_DATA_D3D11_OPTIONS.ClearView = %d\n", options11_1.ClearView);
			D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT minPrecision11_1 = {};
			pD3D11Device1->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &minPrecision11_1, sizeof(minPrecision11_1));
			ATLTRACE("D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT.PixelShaderMinPrecision = %u\n", minPrecision11_1.PixelShaderMinPrecision);
			ATLTRACE("D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT.AllOtherShaderStagesMinPrecision = %u\n", minPrecision11_1.AllOtherShaderStagesMinPrecision);
		}
		{
			Microsoft::WRL::ComPtr<ID3D11DeviceContext1> pD3D11ImmContext1;
			HRESULT hr = m_pD3DImmediateContext.As(&pD3D11ImmContext1);
			DXTRACE_ERR(_T("ID3D11DeviceContext1 from ID3D11DeviceContext?"), hr);
			if (FAILED(hr) || !pD3D11ImmContext1)
			{
				return false;
			}
		}

		// DirectWrite 関連。
		{
			// DirectWrite 1.1 ファクトリ。
			hr = DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				//__uuidof(IDWriteFactory),
				__uuidof(IDWriteFactory1),
				&m_dwriteFactory);
			if (FAILED(hr) || !m_dwriteFactory)
			{
				DXTRACE_ERR(_T("Failed to create DWrite factory!!"), hr);
				return false;
			}

			hr = m_dwriteFactory->CreateTextFormat(
				//L"Segoe UI",
				//L"メイリオ",
				L"Consolas",
				nullptr,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				42.0f,
				//L"en-US",
				L"",
				//L"ja-JP",
				m_textFormat.GetAddressOf()
				);
			if (FAILED(hr) || !m_textFormat)
			{
				DXTRACE_ERR(_T("Failed to create DWrite text format!!"), hr);
				return false;
			}
			// デフォルトのアライメントは左上（Leading & Near）。
			// ちなみにアラビア語圏などの多言語対応のため、GDI 時代のような Left & Top という表現はしないらしい。
#if 0
			m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
#endif
			m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		}
#ifdef USE_D2D
		// Direct2D 関連。
		{
			// Direct2D 1.1 ファクトリ。
			D2D1_FACTORY_OPTIONS d2dOptions = {};
#ifdef _DEBUG
			// D2D 1.1 用のデバッグ レイヤーは、VS 2012/WinSDK 8.0 と同時にインストールされる d2d1debug1.dll。
			// D2D 1.2 用のデバッグ レイヤーは、VS 2013/WinSDK 8.1 と同時にインストールされる d2d1debug2.dll。
			// VS 2013 があれば VS 2012 はほとんど不要だが、余裕があれば両方インストールしておいたほうがよい。

			d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
			hr = D2D1CreateFactory<ID2D1Factory1>(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				d2dOptions,
				m_d2dFactory.GetAddressOf());
			if (FAILED(hr) || !m_d2dFactory)
			{
				DXTRACE_ERR(_T("Failed to create D2D factory!!"), hr);
				return false;
			}

			// Direct3D 11 の DXGI デバイスを使って、
			// Direct2D デバイス オブジェクトと、対応するコンテキストを作成する。
			hr = m_d2dFactory->CreateDevice(dxgiDevice1_1.Get(), m_d2dDevice.GetAddressOf());
			if (FAILED(hr) || !m_d2dDevice)
			{
				DXTRACE_ERR(_T("Failed to create D2D device!!"), hr);
				return false;
			}

			hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dContext.GetAddressOf());
			if (FAILED(hr) || !m_d2dContext)
			{
				DXTRACE_ERR(_T("Failed to create D2D device context!!"), hr);
				return false;
			}

			hr = m_d2dContext->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				m_d2dBlackBrush.GetAddressOf()
				);

			if (FAILED(hr) || !m_d2dBlackBrush)
			{
				DXTRACE_ERR(_T("Failed to create D2D brush!!"), hr);
				return false;
			}

			// 「すべての Windows ストア アプリで、グレースケール テキストのアンチエイリアシングをお勧めします。」
			// というコメントが Visual Studio 2012 Direct2D ストア アプリのテンプレート プロジェクトのソースに記述されている。
			// ClearType は半透明の背景と相性が悪いというのはよく知られているが、なぜストア アプリで ClearType が非推奨なのか不明。
			m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
		}
#endif

#pragma endregion

		this->InitializeCameraSettings();
		if (!this->ResizeScreen(width, height))
		{
			return false;
		}
		const uint32_t shadowResolution = 1024;
		//const uint32_t shadowResolution = 2048;
		const uint32_t shadowCascadeCount = 3;
		if (!this->ResizeShadowBuffer(shadowResolution, shadowResolution, shadowCascadeCount))
		{
			return false;
		}
		m_shadowCascadeConfig.m_iBufferSize = shadowResolution;
		m_shadowCascadeConfig.m_nCascadeLevels = shadowCascadeCount;

		// レンダーステートのセットアップ。
		if (!this->SetupRenderState())
		{
			return false;
		}

		std::vector<uint8_t> shaderBytecode;
		const CPathW pathMediaDir = m_pathMediaDir;

		CPathW pathCompiledShaderFilePath;

		// エフェクトの読込。
		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyShaders.fxbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateEffect(&shaderBytecode[0], shaderBytecode.size(), m_pMainEffect))
		{
			return false;
		}

#if 0
		// エフェクト全体に対するリフレクション インターフェイスは取得できないらしい。
		// D3DReflect() は D3DERR_INVALIDCALL で失敗する。
		// 個別のシェーダーステージごとでないとダメ。
		ComPtr<ID3D11ShaderReflection> pMainEffectReflector;
		//hr = D3DReflect(&shaderBytecode[0], shaderBytecode.size(), IID_PPV_ARGS(pMainEffectReflector.ReleaseAndGetAddressOf()));
		hr = D3D11Reflect(&shaderBytecode[0], shaderBytecode.size(), pMainEffectReflector.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			DXTRACE_ERR(L"Failed to create shader reflection!!", hr);
			return false;
		}
		const UINT numInterfaceSlots = pMainEffectReflector->GetNumInterfaceSlots();
		ATLTRACE("NumInterfaceSlots = %d\n", numInterfaceSlots);
#endif
		{
			hr = m_pD3DDevice->CreateClassLinkage(m_pClassLinkagePSMainLighting.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(L"Failed to create class linkage!!", hr);
				return false;
			}

			pathCompiledShaderFilePath = pathMediaDir;
			pathCompiledShaderFilePath += L"psMainLighting.cso";
			if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
				FAILED(m_pD3DDevice->CreatePixelShader(&shaderBytecode[0], shaderBytecode.size(),
				m_pClassLinkagePSMainLighting.Get(),
				m_pPSMainLighting.ReleaseAndGetAddressOf())))
			{
				return false;
			}
			// インターフェイス スロットの数やスロット番号を調べるためにはリフレクションが必要。
			// HLSL 側で明示的に指定するすべはない。
			// HACK: デバッグ ビルドやツールで事前に調べておいて、本番環境では決め打ちする？

			Microsoft::WRL::ComPtr<ID3D11ShaderReflection> pShaderReflector;
			hr = D3D11Reflect(&shaderBytecode[0], shaderBytecode.size(), pShaderReflector.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(L"Failed to create shader reflection!!", hr);
				return false;
			}
			const UINT numInterfaceSlots = pShaderReflector->GetNumInterfaceSlots();
			ATLTRACE("NumInterfaceSlots = %d\n", numInterfaceSlots);
			const LPCSTR varNamesArray[] =
			{
				"AbstractMainLightingShader",
				"AbstractMainLightingSpecular",
				"AbstractEnvMapColorPicker",
			};
			for (int varIndex = 0; varIndex < ARRAYSIZE(varNamesArray); ++varIndex)
			{
				const char* varName = varNamesArray[varIndex];
				auto pShaderVarName = pShaderReflector->GetVariableByName(varName);
				const UINT ifSlot = pShaderVarName->GetInterfaceSlot(0);
				ATLTRACE("I/F Slot of '%s' = %u\n", varName, ifSlot);
			}
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyComputeShaders.fxbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateEffect(&shaderBytecode[0], shaderBytecode.size(), m_pComputeEffect))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyScreenShaders.fxbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateEffect(&shaderBytecode[0], shaderBytecode.size(), m_pScreenEffect))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyShadowRender.fxbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateEffect(&shaderBytecode[0], shaderBytecode.size(), m_pShadowEffect))
		{
			return false;
		}

		this->GetTechniquesFromEffect();

#pragma region // 頂点シェーダーごとの頂点データの入力レイアウト定義。//

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexT.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayT, ARRAYSIZE(VertexInputLayoutElemDescArrayT),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutT))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexP.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayP, ARRAYSIZE(VertexInputLayoutElemDescArrayP),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutP))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexPC.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayPC, ARRAYSIZE(VertexInputLayoutElemDescArrayPC),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutPC))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexPCT.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayPCT, ARRAYSIZE(VertexInputLayoutElemDescArrayPCT),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutPCT))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexPCNT.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayPCNT, ARRAYSIZE(VertexInputLayoutElemDescArrayPCNT),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutPCNT))
		{
			return false;
		}

		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexPNTIW.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayPNTIW, ARRAYSIZE(VertexInputLayoutElemDescArrayPNTIW),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutPNTIW))
		{
			return false;
		}

#if 0
		pathCompiledShaderFilePath = pathMediaDir;
		pathCompiledShaderFilePath += L"MyVertexFlake.vsbin";
		if (!MyUtil::LoadBinaryFromFile(pathCompiledShaderFilePath, shaderBytecode) ||
			!this->CreateInputVertexLayout(
			VertexInputLayoutElemDescArrayFlake, ARRAYSIZE(VertexInputLayoutElemDescArrayFlake),
			&shaderBytecode[0], shaderBytecode.size(), m_pInputLayoutFlake))
		{
			return false;
		}
#endif
#pragma endregion

#if 0
		// 環境マッピング用のキューブマップの作成。
		if (!m_envCubeMap.Create(m_pD3DDevice.Get()))
		{
			return false;
		}
#endif

		if (!this->CreateMyDummyWhiteTexture())
		{
			return false;
		}

		if (!this->CreateMyToonShadingRefTexture())
		{
			return false;
		}

		// フォント テクスチャの作成。
		if (!this->CreateMyFontTexture())
		{
			return false;
		}

		if (!m_fontRects.CreateEx(m_pD3DDevice.Get()))
		{
			return false;
		}

#if 0
		if (!this->CreateWaveSimWorkBufferAndView())
		{
			return false;
		}
#else
		if (!this->CreateWaveSimWorkTexAndView())
		{
			return false;
		}
#endif

		// コンピュート シェーダーで使用するバッファ（およびそのビュー）を作成する。

		if (!this->CreateRandomTableBufferAndView())
		{
			return false;
		}

		if (!this->CreateFlakeParticleBufferAndView())
		{
			return false;
		}

#pragma region // テクスチャのシェーダーリソース ビューを作成。//

		if (!this->CreateMyShaderResourceView(m_pDummyWhiteTex.Get(), m_pDummyWhiteSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pToonShadingRefTex.Get(), m_pToonShadingRefSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pHudFontAlphaTex.Get(), m_pHudFontAlphaTexSRV))
		{
			return false;
		}

		// D3DCompiler ランタイムへの依存を断つ場合、サンプラーステートおよび定数バッファはホスト側で明示的に作成する必要がある。
		{
			// CD3D11_SAMPLER_DESC コンストラクタに D3D11_DEFAULT 定数を渡すと、デフォルトの
			// D3D11_SAMPLER_DESC 構造体を返してくれるらしい。
			// ヘッダーでのインライン実装を見る限り、MSDN のデフォルト値の説明とは若干異なるが……
			// http://msdn.microsoft.com/ja-jp/library/ee416271.aspx

			hr = m_pD3DDevice->CreateSamplerState(&CD3D11_SAMPLER_DESC(
				D3D11_FILTER_MIN_MAG_MIP_POINT,
				//D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
				//D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				//D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
				//D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
				D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
				0, 1, D3D11_COMPARISON_NEVER, &MyMath::COLOR4F_WHITE.x, -FLT_MAX, +FLT_MAX), &m_pSamplerPointWrap);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateSamplerState(&CD3D11_SAMPLER_DESC(
				D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
				0, 1, D3D11_COMPARISON_NEVER, &MyMath::COLOR4F_WHITE.x, -FLT_MAX, +FLT_MAX), &m_pSamplerLinearWrap);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateSamplerState(&CD3D11_SAMPLER_DESC(
				D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
				0, 1, D3D11_COMPARISON_NEVER, &MyMath::COLOR4F_WHITE.x, -FLT_MAX, +FLT_MAX), &m_pSamplerLinearClamp);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateSamplerState(&CD3D11_SAMPLER_DESC(
				D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
				0, 1, D3D11_COMPARISON_NEVER, &MyMath::COLOR4F_BLACK.x, -FLT_MAX, +FLT_MAX), &m_pSamplerShadowLinearClamp);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateSamplerState(&CD3D11_SAMPLER_DESC(
				D3D11_FILTER_MIN_MAG_MIP_POINT,
				D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER,
				0, 1, D3D11_COMPARISON_NEVER, &MyMath::COLOR4F_TRANSPARENT.x, -FLT_MAX, +FLT_MAX), &m_pSamplerPointBorderTransparent);
			_ASSERTE(SUCCEEDED(hr));
		}

		{
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferBoneMatrixPalettePack), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferBoneMatrixPalette);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferViewParamsPack), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferViewParams);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferMeshPartAttributePack), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferMeshPartAttribute);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferLightParamsPack), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferLightParams);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferClassInstanceSelectorTable), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferClassInstanceSelectorTable);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferShadowRenderingInfoPack), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferShadowRenderingInfo);
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateBuffer(
				&CD3D11_BUFFER_DESC(sizeof(CBufferShadowSamplingInfoPack), D3D11_BIND_CONSTANT_BUFFER), nullptr,
				&m_pCBufferShadowSamplingInfo);
			_ASSERTE(SUCCEEDED(hr));
		}

#pragma endregion

#pragma region // 頂点バッファ・インデックス バッファの作成。//

		if (!this->CreateSinglePointVertexBufferP())
		{
			return false;
		}

		if (!this->CreateCoordAxisLineVertexBufferPC())
		{
			return false;
		}

		if (!this->CreateOneTriangleVertexBuffer())
		{
			return false;
		}

		if (!this->CreateOneSquareVertexBufferPCNT())
		{
			return false;
		}

		if (!this->CreateWaveFrontPlaneVertexBufferPCNT())
		{
			return false;
		}

		if (!this->CreateWaveFrontPlaneGridBufferAndView())
		{
			return false;
		}

		if (!this->CreateOneSquareVertexBufferPCT())
		{
			return false;
		}

		// 四角形の描画にはインデックス バッファを使う。
		if (!this->CreateOneQuadIndexBuffer())
		{
			return false;
		}
#pragma endregion

		m_isInitialized = true;

		return true;
	}


	static void SetupSingleViewport(ID3D11DeviceContext* pDeviceContext, float width, float height)
	{
		// デバイスのビューポートを設定する。
		D3D11_VIEWPORT viewport = {};
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		pDeviceContext->RSSetViewports(1, &viewport);
	}

	static void SetupSingleViewport(ID3D11DeviceContext* pDeviceContext, uint32_t width, uint32_t height)
	{
		SetupSingleViewport(pDeviceContext, float(width), float(height));
	}

	namespace
	{
		void ClearRtvSlots(ID3D11DeviceContext* pDeviceContext, UINT numViews)
		{
			static ID3D11RenderTargetView* const ppRTViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { };

			_ASSERTE(numViews <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

			pDeviceContext->OMSetRenderTargets(numViews, ppRTViews, nullptr);
		}

		void ClearSrvSlots(ID3D11DeviceContext* pDeviceContext, UINT numViews)
		{
			static ID3D11ShaderResourceView* const ppSRViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

			_ASSERTE(numViews <= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

			// テッセレーション ステージやジオメトリ シェーダーでは現状テクスチャ参照はないので放置。
			// UNDONE: ディスプレースメント マッピングを実行する場合は必要になるかもしれない。
			pDeviceContext->VSSetShaderResources(0, numViews, ppSRViews);
			pDeviceContext->PSSetShaderResources(0, numViews, ppSRViews);
			pDeviceContext->CSSetShaderResources(0, numViews, ppSRViews);
		}

		void ClearUavSlots(ID3D11DeviceContext* pDeviceContext, UINT numViews)
		{
			static ID3D11UnorderedAccessView* const ppUAViews[D3D11_PS_CS_UAV_REGISTER_COUNT] = {};

			_ASSERTE(numViews <= D3D11_PS_CS_UAV_REGISTER_COUNT);

			// ID3DX11EffectUnorderedAccessViewVariable::SetUnorderedAccessView() にほぼ相当する。
			// 追加／消費バッファ（AppendStructuredBuffer / ConsumeStructuredBuffer）でないならば、
			// pUAVInitialCounts パラメータは NULL でよいらしい。
			pDeviceContext->CSSetUnorderedAccessViews(0, numViews, ppUAViews, nullptr);
		}

		void ClearRtvSrvUavSlots(ID3D11DeviceContext* pDeviceContext)
		{
			// Direct3D 11 ランタイムからの警告メッセージ「D3D11: WARNING: ID3D11DeviceContext::OMSetRenderTargets……」が出力されるときの対処法。
			// Direct3D 10 の場合のメッセージは「D3D10: WARNING: ID3D10Device::OMSetRenderTargets……」となる。
			// これはあるテクスチャを出力レンダーターゲットに設定したまま、さらに入力リソースに設定した状態でレンダリングを実行するなど、
			// 自己レンダリングが発生しうる状況に対する警告の D3D エラーメッセージなので、
			// 回避するにはいったん強制的にレンダーターゲットとシェーダーリソースのスロット設定をリセットする。
			// また、SRV はシェーダーステージごとに設定／リセットする必要がある。
			// SRV と UAV のリソース競合に関しても同様。
			// なお、エフェクト フレームワークのリフレクションに頼りすぎると、直接 SRV/UAV スロットを意識しなくて済むため、
			// 逆にこういう基本的な誤りに気付きにくくなる弊害がある。
			// エフェクト フレームワーク内部では結局のところ、API を使ってシェーダーステージごとに SRV/UAV を設定していることを忘れないこと。
			// http://wlog.flatlib.jp/item/1077
			// 
			// ちなみに Direct3D では、頂点レイアウト、およびシェーダーステージごとの SRV/UAV や定数バッファの設定は
			// シェーダープログラムのインスタンスを変えても同一ステージであれば共有される。
			// したがって、デバイス／デバイス コンテキストへのシェーダープログラムの設定と、SRV などの設定とは順不同。
			// また、頂点バッファの設定と、頂点レイアウトの設定とは順不同。
			// さらに、シェーダーステージの組み合わせ（パイプライン構築）も、C++ 側で動的に制御できる。
			// エフェクト フレームワークの technique と pass は、この API 特性をうまく生かした仕組みになっている。
			// OpenGL では、頂点属性や Uniform 設定はシェーダープログラムのインスタンスごとに個別設定する必要がある。
			// したがって、シェーダープログラムの適用後に Uniform を設定し、頂点バッファのバインド後に頂点属性を設定する必要がある。
			// ちなみにシェーダーステージの組み合わせは、GLSL コンパイル＆リンク時に決まってしまう。
			// ただし新しいバージョンの OpenGL で導入された Uniform Buffer Object や Program Pipeline Object を使えば、
			// Direct3D に比較的近い設計にすることも可能。

			{
				// とりあえず RT0～RT2 をクリア。
				ClearRtvSlots(pDeviceContext, 3);
			}

			{
				// とりあえず t0～t5 レジスタをクリア。
				ClearSrvSlots(pDeviceContext, 6);
			}

			{
				// とりあえず u0 レジスタをクリア。
				ClearUavSlots(pDeviceContext, 1);
			}
		}

		// VSM では深度のほかに深度の二乗も記録する必要があるため、深度バッファへの書込の他にカラーバッファへの書込を必要とするが、
		// 深度バッファのみを書き込むだけでよい場合は、空の RTV と有効な DSV をセットしてレンダリングを実行すればよい。
		// Forward+ 用の深度バッファ描画などの場面で使えるはず。
		// 詳しくは旧 DirectX SDK 付属の CascadedShadowMaps11 などを参照のこと。
		void SetDsvOnly(ID3D11DeviceContext* pDeviceContext, ID3D11DepthStencilView* pDSV)
		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				nullptr,
			};
			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, pDSV);
		}

	} // end of namespace


	static const UINT ShadowTexResSlotIndex = 3; // t3 レジスタ。

	void MyD3DManager::ApplyPsBlurToShadowTex()
	{
		auto pDeviceContext = this->GetMainThreadDeviceContext();

		pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

		this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		this->SetOneQuadIndexBuffer();

		// 水平方向のブラー。
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_pShadowColorBufferSRV.Get(),
			};
			pDeviceContext->PSSetShaderResources(ShadowTexResSlotIndex, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_pShadowBlurWorkRTV.Get(),
			};

			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}

		this->DrawOneQuad(m_effectTechs.TechApplyHorizontalMovingAverageFilterToShadowTex);

		// 垂直方向のブラー。
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_pShadowBlurWorkSRV.Get(),
			};
			pDeviceContext->PSSetShaderResources(ShadowTexResSlotIndex, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_pShadowColorBufferRTV.Get(),
			};

			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}

		this->DrawOneQuad(m_effectTechs.TechApplyVerticalMovingAverageFilterToShadowTex);

		//pDeviceContext->GSSetShader(nullptr, nullptr, 0);
	}

	void MyD3DManager::ApplyCsBlurToShadowTex()
	{
		auto pDeviceContext = this->GetMainThreadDeviceContext();

		// NOTE: シェーダー側と合わせること。
		// HACK: ヘッダーファイル経由でシンボルを共有するべきかも。
		const uint32_t ShadowBlurComputingWorkSize = 1024;
		//const uint32_t ShadowBlurComputingWorkSize = 256;

		// 水平方向のブラー。
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_pShadowColorBufferSRV.Get(),
			};
			pDeviceContext->CSSetShaderResources(ShadowTexResSlotIndex, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_pShadowBlurWorkUAV.Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}

		this->Dispatch(m_effectTechs.CSTechApplyBlurToShadowTex,
			//m_shadowCascadeConfig.m_iBufferSize, m_shadowCascadeConfig.m_nCascadeLevels, 1);
			m_shadowCascadeConfig.m_iBufferSize / ShadowBlurComputingWorkSize,
			m_shadowCascadeConfig.m_iBufferSize,
			m_shadowCascadeConfig.m_nCascadeLevels);

		// 垂直方向のブラー。
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_pShadowBlurWorkSRV.Get(),
			};
			pDeviceContext->CSSetShaderResources(ShadowTexResSlotIndex, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_pShadowColorBufferUAV.Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}

		this->Dispatch(m_effectTechs.CSTechApplyBlurToShadowTex,
			//m_shadowCascadeConfig.m_iBufferSize, m_shadowCascadeConfig.m_nCascadeLevels, 1);
			m_shadowCascadeConfig.m_iBufferSize / ShadowBlurComputingWorkSize,
			m_shadowCascadeConfig.m_iBufferSize,
			m_shadowCascadeConfig.m_nCascadeLevels);
	}

	void MyD3DManager::ApplyPsBlurToDownSampledTex()
	{
		auto pDeviceContext = this->GetMainThreadDeviceContext();

		// ワーク テクスチャ 0 に対する水平方向ぼかしの結果をワーク テクスチャ 1 に書き込む。

		// Output
		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_ppDownSampledTempWorkRTV[1].Get(),
			};

			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}
		// Input
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_ppDownSampledTempWorkSRV[0].Get(),
			};
			//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		// HACK: ブレンドも深度ステンシルもないので、クリアは不要かも。
		//pDeviceContext->ClearRenderTargetView(m_ppDownSampledTempWorkRTV[1].Get(), &MyMath::ZERO_VECTOR4F.x);

		if (m_myEffectSettings.EnablesBloomEffect)
		{
			this->DrawOneQuad(m_effectTechs.TechApplyHorizontalGaussianBlurToDownSampledTex);
		}

		ClearRtvSrvUavSlots(pDeviceContext);

		// ワーク テクスチャ 1 に対する垂直方向ぼかしの結果をワーク テクスチャ 0 に書き込む。

		// Output
		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_ppDownSampledTempWorkRTV[0].Get(),
			};

			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}
		// Input
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_ppDownSampledTempWorkSRV[1].Get(),
			};
			//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		// HACK: ブレンドも深度ステンシルもないので、クリアは不要かも。
		//pDeviceContext->ClearRenderTargetView(m_ppDownSampledTempWorkRTV[0].Get(), &MyMath::ZERO_VECTOR4F.x);

		if (m_myEffectSettings.EnablesBloomEffect)
		{
			this->DrawOneQuad(m_effectTechs.TechApplyVerticalGaussianBlurToDownSampledTex);
		}
	}

	void MyD3DManager::ApplyCsBlurToDownSampledTex()
	{
		// コンピュート シェーダーはピクセル シェーダーと違って任意位置に出力できるので、
		// ダウンサンプルされたテクスチャの解像度の縦横がまったく同じ場合、転置を利用することで水平・垂直ぼかしにまったく同じコードが使える。

		auto pDeviceContext = this->GetMainThreadDeviceContext();

		// ワーク テクスチャ 0 に対する水平方向ぼかしの結果をワーク テクスチャ 1 に書き込む。

		// Input
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_ppDownSampledTempWorkSRV[0].Get(),
			};
			//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		// Output
		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_ppDownSampledTempWorkUAV[1].Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}
		if (m_myEffectSettings.EnablesBloomEffect)
		{
			this->Dispatch(m_effectTechs.CSTechApplyGaussianBlurToDownSampledTex, DOWN_SAMPLED_TEX_SIZE, 1, 1);
		}

		ClearRtvSrvUavSlots(pDeviceContext);

		// ワーク テクスチャ 1 に対する垂直方向ぼかしの結果をワーク テクスチャ 0 に書き込む。

		// Input
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_ppDownSampledTempWorkSRV[1].Get(),
			};
			//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		// Output
		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_ppDownSampledTempWorkUAV[0].Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}
		if (m_myEffectSettings.EnablesBloomEffect)
		{
			this->Dispatch(m_effectTechs.CSTechApplyGaussianBlurToDownSampledTex, DOWN_SAMPLED_TEX_SIZE, 1, 1);
		}
	}

	void MyD3DManager::DoPsParellelReductionTest(ID3D11ShaderResourceView* pReductionSrcImageView, uint32_t reductionSrcImageSize)
	{
		auto pDeviceContext = this->GetMainThreadDeviceContext();

		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				pReductionSrcImageView,
			};
			pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_ppPSReductionWorkRTV[0].Get(),
			};
			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}

		pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());
		this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		this->SetOneQuadIndexBuffer();

		// できるかぎり描画コール回数が減るように、複数段のカーネルを組みあわせてリダクションを実行する。

		uint32_t step = 8;
		//const uint32_t step = 4;
		//const uint32_t step = 2;
		auto techReduction = m_effectTechs.TechReduction8x8;
		//auto techReduction = m_effectTechs.TechReduction4x4;
		//auto techReduction = m_effectTechs.TechReduction2x2;

		// ビューポートは入力テクスチャではなく出力テクスチャの出力領域に合わせる。
		uint32_t reductionDstImageSize = reductionSrcImageSize;
		reductionDstImageSize = uint32_t(std::ceil(float(reductionDstImageSize) / float(step)));
		SetupSingleViewport(pDeviceContext, reductionDstImageSize, reductionDstImageSize);

		this->DrawOneQuad(techReduction);

		int inputIndex = 0;
		while (reductionDstImageSize > 1)
		{
			if (reductionDstImageSize <= 2)
			{
				step = 2;
				techReduction = m_effectTechs.TechReduction2x2;
			}
			else if (reductionDstImageSize <= 4)
			{
				step = 4;
				techReduction = m_effectTechs.TechReduction4x4;
			}
			reductionDstImageSize = uint32_t(std::ceil(float(reductionDstImageSize) / float(step)));

			const int outputIndex = !inputIndex;

			ClearRtvSrvUavSlots(pDeviceContext);
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					m_ppPSReductionWorkSRV[inputIndex].Get(),
				};
				pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}
			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_ppPSReductionWorkRTV[outputIndex].Get(),
				};
				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
			}

			SetupSingleViewport(pDeviceContext, reductionDstImageSize, reductionDstImageSize);

			this->DrawOneQuad(techReduction);
			inputIndex = outputIndex;
		}
	}

	void MyD3DManager::DoCsParellelReductionTest(ID3D11ShaderResourceView* pReductionSrcImageView, uint32_t reductionSrcImageSize)
	{
		auto pDeviceContext = this->GetMainThreadDeviceContext();

		// 2D → 1D。
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				pReductionSrcImageView,
			};
			pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_ppCSReductionWorkUAV[0].Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}

		// HACK: 割り切れない場合は切り上げが必要。
		// HACK: 可変サイズの場合は定数バッファでシェーダー側にサイズ情報を送る。
		const uint32_t gridSizeX = reductionSrcImageSize / MY_CS_REDUCTION_TILE_SIZE_X;
		const uint32_t gridSizeY = reductionSrcImageSize / MY_CS_REDUCTION_TILE_SIZE_Y;
		this->Dispatch(m_effectTechs.CSTechReductionTexture2DTo1D,
			gridSizeX, gridSizeY, 1);

		// 1D → 1pix。
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_ppCSReductionWorkSRV[0].Get(),
			};
			pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_ppCSReductionWorkUAV[1].Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}

		// HACK: 1D テクスチャのサイズがローカル グループ サイズを超える場合、複数回の CS 発行が必要。
		const uint32_t gridSize = 1;
		this->Dispatch(m_effectTechs.CSTechReductionTexture1DTo1D,
			gridSize, 1, 1);
	}

	bool MyD3DManager::Render(bool advancesFrame, bool renders3d)
	{
		if (!m_isInitialized)
		{
			// 初期化が済んでいないのに描画することはできない。
			return false;
		}

		HRESULT hr = E_FAIL;

		auto pDeviceContext = this->GetMainThreadDeviceContext();

		_ASSERTE(pDeviceContext);

		MyUtil::HRStopwatch stopwatch;
		stopwatch.Start();

		if (renders3d)
		{
			this->Render3D(advancesFrame);
		}

#ifdef USE_D2D
		// Direct2D コンテンツの描画。
		const size_t projectileCount = m_myUIAnimCenter.GetProjectileAnimCount();
		for (size_t i = 0; i < projectileCount; ++i)
		{
			// おそらく ID2D1DeviceContext::SetTarget() メソッド呼び出しによって参照カウントが増加するらしい。
			// DXGI サーフェイスから作成したリソース共有ビットマップの場合、使用が終わったらリセットしておかないとスワップ チェーンのリサイズが失敗する。
			m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

			//auto textOriginPoint = D2D1::Point2F(0.0f, 0.0f);
			auto textOriginPoint = m_myUIAnimCenter.GetProjectilePosition(i);
			//textOriginPoint.y -= m_myCameraSettings.GetScreenHeightF();
			m_d2dContext->BeginDraw();
			if (!renders3d)
			{
				const auto strMessage = L"Now Loading...";
				const auto strLength = wcslen(strMessage);
				m_d2dContext->DrawText(strMessage, static_cast<uint32_t>(strLength),
					m_textFormat.Get(), D2D1::RectF(0,0,0,0),
					m_d2dBlackBrush.Get());
			}
			m_d2dContext->DrawTextLayout(
				textOriginPoint,
				m_textLayout.Get(),
				m_d2dBlackBrush.Get(),
				D2D1_DRAW_TEXT_OPTIONS_NO_SNAP | D2D1_DRAW_TEXT_OPTIONS_CLIP
				);
			hr = m_d2dContext->EndDraw();
			// TODO: D2DERR_RECREATE_TARGET を無視する。

			m_d2dContext->SetTarget(nullptr);
		}
		if (advancesFrame)
		{
			m_myUIAnimCenter.UpdateAnimTimer();
		}
		m_myUIAnimCenter.RemoveDeadItemsInQueue();
#endif

		// バック バッファからフロント バッファへ転送。
		// Windows ストアアプリでは必ず垂直同期待ちになるらしい。
		// http://shikihuiku.wordpress.com/2012/11/13/storeapp%E3%81%AE%E5%9E%82%E7%9B%B4%E5%90%8C%E6%9C%9F/
		// アプリの種類と、画面更新およびフレームレートの設計に関するガイドラインが公開されている。
		// アクション ゲームであれば常時更新・フレームレートの維持は必須になるが、ボード ゲームなどはその限りではない。
		// http://msdn.microsoft.com/ja-jp/library/windows/apps/dn602564.aspx
#if 0
		hr = m_pSwapChain->Present(0, 0); // 垂直同期待ちをしない。
#else
		hr = m_pSwapChain->Present(1, 0); // 垂直同期待ちをする。
#endif
		// 描画が終わったら I/O スロットをクリアしておく。
		ClearRtvSrvUavSlots(pDeviceContext);

		stopwatch.Stop();
		if (advancesFrame)
		{
			m_fpsCounter.AdvanceFrameCounter();
			m_fpsCounter.AddElapsedTimeSec(stopwatch.GetElapsedTimeInSeconds());
			const uint32_t FpsCounterLimit = 20; // N フレーム分の経過時間を使って FPS を計算する。
			if (m_fpsCounter.GetFrameCounter() >= FpsCounterLimit)
			{
				//ATLTRACE("Frame=%u, Time=%.12f, FPS=%.1f\n", m_fpsCounter.GetFrameCounter(), m_fpsCounter.GetElapsedTimeSec(), m_fpsCounter.GetFpsValue());
				m_fpsCounter.UpdateFpsValue();
				m_fpsCounter.Reset();
			}
		}
		return SUCCEEDED(hr);
	}

	void MyD3DManager::Render3D(bool advancesFrame)
	{
		auto pDeviceContext = this->GetMainThreadDeviceContext();

		_ASSERTE(pDeviceContext);

		// エフェクト テクニックを切り替え、パスを適用すると、内部で ID3D11DeviceContext::VSSetShader() などが呼ばれ、
		// シェーダープログラムが切り替わるが、
		// 同一のスロットを使い続けるかぎりは、シェーダーごとに定数バッファをいちいち設定し直す必要は無い。
		// Direct3D 9 世代までのグラフィックス デバイスには定数バッファ（定数レジスタ）のスロット数に厳しい制約があるため、
		// 場合によってはシェーダーごとに定数レジスタ セットを切り替える必要があるが、
		// Direct3D 10 以降はかなり余裕がある。
		{
			// 各シェーダーへ定数バッファ（へのハンドル）を設定する。
			// 配列インデックスがレジスタのスロット番号（b0, b1, ...）に対応している。
			// 実際の定数データ自体の更新（GPU への転送）は、ID3D11DeviceContext::UpdateSubresource() もしくは
			// ID3D11DeviceContext::Map() / Unmap() で行なう。
			// エフェクト フレームワークのエフェクト変数は、この一連の操作をすべて隠蔽しているにすぎない。
			// 
			// なお、要素が1つだけの場合は別に一時配列を用意する必要は無い。ポインタ型のスカラー変数へのアドレスを渡すだけでよい。
			// ただし _com_ptr_t, Microsoft::WRL::ComPtr などのスマートポインタを使う場合は、
			// 必ず一時配列もしくは生ポインタ型のスカラー変数へいったん格納する必要がある。
			// また、ATL::CComPtr::operator&() の実装と _com_ptr_t::operator&(), Microsoft::WRL::ComPtr::operator&() の実装の違いに注意。
			// 前者は Release() を行なわないが、後者は Release() を行なうようになっている。
			// 安全や可読性のためには、operator&() は使わずに、Microsoft::WRL::ComPtr::ReleaseAndGetAddressOf() を使ったほうがよい。
			ID3D11Buffer* const ppCBuffers[] =
			{
				m_pCBufferBoneMatrixPalette.Get(),
				m_pCBufferViewParams.Get(),
				m_pCBufferMeshPartAttribute.Get(),
				m_pCBufferLightParams.Get(),
				m_pCBufferClassInstanceSelectorTable.Get(),
				m_pCBufferShadowRenderingInfo.Get(),
				m_pCBufferShadowSamplingInfo.Get(),
			};
			// とりあえず使用しているステージに対してすべてバインドしておく。
			pDeviceContext->VSSetConstantBuffers(0, ARRAYSIZE(ppCBuffers), ppCBuffers);
			pDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(ppCBuffers), ppCBuffers);
			pDeviceContext->HSSetConstantBuffers(0, ARRAYSIZE(ppCBuffers), ppCBuffers);
			pDeviceContext->DSSetConstantBuffers(0, ARRAYSIZE(ppCBuffers), ppCBuffers);
			pDeviceContext->GSSetConstantBuffers(0, ARRAYSIZE(ppCBuffers), ppCBuffers);
			pDeviceContext->CSSetConstantBuffers(0, ARRAYSIZE(ppCBuffers), ppCBuffers);
		}
		{
			// サンプラーステートはほとんどの場合ピクセル シェーダーのみで必要だが、
			// 場合によっては頂点シェーダーでの VTF やテッセレーション シェーダーでのディスプレースメント マッピング、
			// コンピュート シェーダーで参照されることもある。
			// ただし SRV/RTV や SRV/UAV などと違って I/O が入れ替わることがないので、
			// あまりパターンがなければ、フレームごとに1回だけ設定してすべてのエフェクト テクニックで共有する。
			ID3D11SamplerState* const ppSamplerStates[] =
			{
				m_pSamplerPointWrap.Get(),
				m_pSamplerLinearWrap.Get(),
				m_pSamplerLinearClamp.Get(),
				m_pSamplerShadowLinearClamp.Get(), // シャドウ サンプリング用。Aniso のほうが高品質だが、とりあえず Linear にしておく。
				m_pSamplerPointBorderTransparent.Get(),
			};
			pDeviceContext->PSSetSamplers(0, ARRAYSIZE(ppSamplerStates), ppSamplerStates);
			pDeviceContext->VSSetSamplers(0, ARRAYSIZE(ppSamplerStates), ppSamplerStates);
		}

#pragma region // コンピュート シェーダーで乱数テーブルを更新する。//
		ClearRtvSrvUavSlots(pDeviceContext);
		{
			ID3D11UnorderedAccessView* const ppUAViews[] =
			{
				m_pRandomTableUAV.Get(),
			};
			pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
		}
		this->Dispatch(m_effectTechs.CSTechUpdateRandomTable, ComputeDispatchCountX, 1, 1);
#pragma endregion

		// 3D 用のビュー行列は最初に1回設定すればよい。シャドウマップを作成する場合もライト行列はライト パラメータなどに含めておく。
		this->UpdateEffectMatrices();

#pragma region // シャドウマップを描画する。//
		if (true)
		{
			ClearRtvSrvUavSlots(pDeviceContext);

			// デバイスのビューポートを設定。
			// ビューポートをシャドウ テクスチャのサイズに合わせる。
			SetupSingleViewport(pDeviceContext, m_shadowCascadeConfig.m_iBufferSize, m_shadowCascadeConfig.m_iBufferSize);

			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_pShadowColorBufferRTV.Get(),
				};

				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, m_pShadowDSV.Get());
			}

			// レンダーターゲットをゼロ クリアする。
			pDeviceContext->ClearRenderTargetView(m_pShadowColorBufferRTV.Get(), &MyMath::ZERO_VECTOR4F.x);

			pDeviceContext->ClearDepthStencilView(
				m_pShadowDSV.Get(), // 深度ステンシル ビュー。
				D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // クリアするバッファ。ステンシルを含まなくてもビットを立てていて OK らしい。
				1.0f, // 深度のクリア値。
				0 // ステンシルのクリア値。
				);

			// シャドウを描画する際にメッシュのテクスチャのアルファ チャンネルを見てアルファ テストを行なうこともありえるが、
			// SRV のセットはメッシュループで行なう。

			if (!m_mainMeshArray.empty())
			{
				// デバイスに頂点レイアウトをセット。
				pDeviceContext->IASetInputLayout(m_pInputLayoutPNTIW.Get());

				pDeviceContext->RSSetState(m_pRasterStateShadow.Get());

				// TODO: シャドウ レンダリングを行なうか否か（シャドウ キャスターか否か）はパーツごとに設定できるようにする。
				this->DrawMeshArray(m_effectTechs.TechShadowRender, nullptr, nullptr);

				// シャドウマップのぼかし。
				// ぼかし量は「カスケード合わせ」のアルゴリズムにおいてレンダリング条件に影響を与えるので、
				// ぼかし量を動的に変更するときは注意。
#if 0
				pDeviceContext->RSSetState(m_pRasterStateSolidMsaa.Get());
				// [A] ピクセル シェーダー版。
				this->ApplyPsBlurToShadowTex();
#else
				// [B] コンピュート シェーダー版。
				this->ApplyCsBlurToShadowTex();
#endif
			}
		}
#pragma endregion

#pragma region // まずオフスクリーンの FP16 MSAA カラーテクスチャにメッシュなどを描画する。//
		// 3D シーンは MSAA ターシャリ バッファに描画しておき、それを非 MSAA のバックバッファに転送する。
		// 3D シーン専用のターシャリ バッファを用意すると、カメラ切替時のクロスフェードなどが実装しやすい。
		// MSAA の不要な 2D HUD は後からバックバッファに直接描画する。

		pDeviceContext->RSSetState(m_pRasterStateSolidMsaa.Get());

		ClearRtvSrvUavSlots(pDeviceContext);

		// ビューポートをスクリーンのサイズに合わせる。
		SetupSingleViewport(pDeviceContext, m_myCameraSettings.GetScreenWidthF(), m_myCameraSettings.GetScreenHeightF());

		// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
		{
			// MRT を使って、ファーマップと法線・深度マップを同時に生成する。
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_pSubColorBufferRTV.Get(),
				m_pMSAAFurGeoMapRTV.Get(),
				m_pMSAANormalDepthMapRTV.Get(),
			};

			//pDeviceContext->OMSetRenderTargets(1, ppRTViews, m_pSubDSV);
			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, m_pSubDSV.Get());
		}
		{
			// エフェクト側（シェーダー側）のテクスチャ変数と、C++ 側のテクスチャのシェーダーリソース ビューとを関連付ける。
			// Direct3D 10/11 では、テクスチャ オブジェクトのインターフェイス（ID3D10Texture2D/ID3D11Texture2D など）そのものとは
			// 直接関連付けないような方針になっている。
			// ID3D10EffectShaderResourceVariable::SetResource() や ID3DX11EffectShaderResourceVariable::SetResource() にほぼ相当するが、
			// エフェクト API のインターフェイスを経由する場合はレジスタ番号をほとんど意識しなくてよくなる。
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				nullptr, // マテリアルのディフューズ マップ テクスチャ用に空けておく。
				m_pToonShadingRefSRV.Get(),
				m_envCubeMap.GetSRV(),
				m_pShadowColorBufferSRV.Get(),
			};
			//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		// レンダーターゲット（カラーテクスチャ）を単色で塗りつぶす。
		// ClearRenderTargetView() は RGBA32F を受け取る。D3DXCOLOR, D3DXVECTOR4 を使うこともできる。
		// XMFLOAT4 などの float4 構造体へのアドレスを使うこともできる。
		pDeviceContext->ClearRenderTargetView(m_pSubColorBufferRTV.Get(), &m_myCommonSettings.BackColor.x);
		//pDeviceContext->ClearRenderTargetView(m_pSubColorBufferRTV, D3DXCOLOR(0.0f, 0.5f, 0.5f, 1.0f));
		//pDeviceContext->ClearRenderTargetView(m_pSubColorBufferRTV, D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f));

		// レンダーターゲット（ファーマップ）をゼロ クリアする。
		pDeviceContext->ClearRenderTargetView(m_pMSAAFurGeoMapRTV.Get(), &MyMath::ZERO_VECTOR4F.x);
		// レンダーターゲット（法線・深度マップ）をゼロ クリアする。
		pDeviceContext->ClearRenderTargetView(m_pMSAANormalDepthMapRTV.Get(), &MyMath::ZERO_VECTOR4F.x);

		pDeviceContext->ClearDepthStencilView(
			m_pSubDSV.Get(), // 深度ステンシル ビュー。
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // クリアするバッファ。ステンシルを含まなくてもビットを立てていて OK らしい。
			1.0f, // 深度のクリア値。
			0 // ステンシルのクリア値。
			);

		if (m_myCommonSettings.DisplaysCoordAxes)
		{
			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPC.Get());

			// 座標軸を描画する。
			this->SetMyVertexBuffer(m_pCoordAxisLineVertexBufferPC.Get(), sizeof(MyVertexTypes::MyVertexPC), D3D_PRIMITIVE_TOPOLOGY_LINELIST);

			this->DrawPrimitive(m_effectTechs.TechRenderCoordAxisLines, 6U);
		}

		if (true)
		{
			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutP.Get());

			// ライト方向を描画する。
			// ジオメトリ シェーダーで頂点を増幅してライン化するので、ポイント プリミティブをキックするだけでよい。
			this->SetMyVertexBuffer(m_pSinglePointVertexBufferP.Get(), sizeof(MyVertexTypes::MyVertexP), D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

			this->DrawPrimitive(m_effectTechs.TechRenderLightDirLine, 1U);
		}

		{
			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCNT.Get());

			// 四角形をテスト描画する。
			this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCNT.Get(), sizeof(MyVertexTypes::MyVertexPCNT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			this->SetOneQuadIndexBuffer();

			this->DrawOneQuad(m_effectTechs.TechLambertPCNT);

			// 三角形をテスト描画する。
			this->SetMyVertexBuffer(m_pOneTriangleVertexBufferPCNT.Get(), sizeof(MyVertexTypes::MyVertexPCNT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			this->DrawOneTriangle(m_effectTechs.TechLambertPCNT);
		}

		if (!m_mainMeshArray.empty())
		{
			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPNTIW.Get());

			// メッシュの描画。
			{
				// 予備のダミーパスを使う方法。冗長だし面倒。
				//D3DX11_PASS_SHADER_DESC psDesc = {};
				//pTechnique->GetPassByIndex(0)->GetPixelShaderDesc(&psDesc);
				//Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader;
				//psDesc.pShaderVariable->GetPixelShader(psDesc.ShaderIndex, &pPixelShader);

				// エフェクト テクニックの外で定義したピクセル シェーダーのインスタンス（関数ポインタのようなもの）を取得する。
				// リフレクションを切っていてもこの方法は使える。
				// 現状ピクセル シェーダーのみで動的シェーダーリンクを使っているので、
				// とりあえずピクセル シェーダーだけ実行時にに差し替える方式にしている。
				// シャドウ サンプリングを行なうか否か（シャドウ レシーバーか否か）、トゥーン シェーディングを行なうか否かは
				// パーツごとに設定できるようにする。
				// HACK: シェーダーインターフェイス実装クラスのインスタンス配列を渡すタイミングで、
				// 順序を勝手に推量した配列を渡すとエラーになる。
				// D3D11 ERROR: ID3D11DeviceContext::PSSetShader: ppClassInstances[1]=UniInstanceSpecularDummy[0] has a type that does not implement the required interface. [ STATE_SETTING ERROR #2097308: DEVICE_SETSHADER_INVALID_INSTANCE_TYPE]
				// D3DReflect(), ID3D11ShaderReflection::GetVariableByName(), ID3D11ShaderReflectionVariable::GetInterfaceSlot() を使って
				// やはりセオリー通り、インターフェイスごとのスロット番号を正しく取得しておかないとダメらしい。
				// しかしそのためには動的シェーダーリンクを使っているシェーダーを個別にコンパイルして、
				// そのバイトコードを使ってリフレクションしないとダメな模様。

#if 0
				Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader;
				auto pEffectShaderVariable = m_pMainEffect->GetVariableByName("g_psPhongAndCreateFurMap")->AsShader();
				pEffectShaderVariable->GetPixelShader(0, &pPixelShader);
#endif
#if 0
				// リフレクションを切っているとシェーダーの情報は取得できない。NULL 参照でアクセス違反が発生する。
				// ちなみに Effects11 のソースコード中で使われている D3DXASSERT は単にエラーメッセージを OutputDebugStringA で出力するだけで、
				// デバッグ セッションは中断しない。CRT の _ASSERTE をそのまま使えばいいのに……
				D3DX11_EFFECT_SHADER_DESC shaderDesc = {};
				pEffectShaderVariable->GetShaderDesc(0, &shaderDesc);
#endif

				this->DrawMeshArray(m_effectTechs.TechSkinning, m_pClassLinkagePSMainLighting.Get(), m_pPSMainLighting.Get());
			}

			// メッシュを描画した後は、頂点シェーダー＆ピクセル シェーダー以外の付加的なシェーダーステージをいったんリセットしておく。
			// エフェクト ファイル側でシェーダーステージの設定を記述すると、XXSetShader() が内部でコールされるが、
			// 描画が終わってもテクニック適用前の状態に自動的に復元されないので注意（昔の D3DX9, Effects9 のような Begin-End でなく Apply になっている）。
			// また、エフェクト ファイル側で明示的に NULL 設定しない場合、前回の設定値が使われる。
			// ジオメトリ シェーダーステージやテッセレーション関連ステージは
			// 頂点シェーダーやピクセル シェーダーに比べて使用頻度が低く、リセットし忘れることがあるので注意。
			// コンピュート シェーダーは描画パイプラインとは独立しているので、リセットし忘れても影響はない。
			//pDeviceContext->HSSetShader(nullptr, nullptr, 0);
			//pDeviceContext->DSSetShader(nullptr, nullptr, 0);
			//pDeviceContext->GSSetShader(nullptr, nullptr, 0);
		}

#if 0
		// デバイスに頂点レイアウトをセット。
		pDeviceContext->IASetInputLayout(m_pInputLayoutPCT);

		// フォント テクスチャをテスト描画する。
		// Z バッファを書き込まない。（Z バッファを参照する必要すらないかも）
		this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT, sizeof(MyMath::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		this->SetOneQuadIndexBuffer();

		this->DrawOneQuad(m_effectTechs.TechFont);
#endif

#pragma endregion

		// HACK: 透明なメッシュを描画することを考えると、背景は先に描画したほうがよい。

#pragma region // FP16 MSAA カラーテクスチャに、コンピュート シェーダーによるシミュレーション結果を描画する。//

		if (m_myEffectSettings.DisplaysWaveFront)
		{
			ClearRtvSrvUavSlots(pDeviceContext);

			// フリップ フラグの初期値は false なので、最初は #0 が入力、#1 が出力。
			// HACK: 波動方程式の数値計算にはバッファが3つ必要なはず。
			// 最初は #0, #1 が入力で、#2 が出力。
			auto* pWaveSimSRV0 = m_ppWaveSimWorkSRV[0].Get();
			auto* pWaveSimSRV1 = m_ppWaveSimWorkSRV[1].Get();
			auto* pWaveSimUAV = m_ppWaveSimWorkUAV[2].Get();
			// 剰余を使ってもいいが、普通に分岐のほうが速そう。
			switch (m_waveSimInOutFlipCounter)
			{
			case 1:
				pWaveSimSRV0 = m_ppWaveSimWorkSRV[1].Get();
				pWaveSimSRV1 = m_ppWaveSimWorkSRV[2].Get();
				pWaveSimUAV = m_ppWaveSimWorkUAV[0].Get();
				break;
			case 2:
				pWaveSimSRV0 = m_ppWaveSimWorkSRV[2].Get();
				pWaveSimSRV1 = m_ppWaveSimWorkSRV[0].Get();
				pWaveSimUAV = m_ppWaveSimWorkUAV[1].Get();
				break;
			default:
				break;
			}
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					pWaveSimSRV0,
					pWaveSimSRV1,
					m_pWaveSimMaskSRV.Get(),
					m_pWaveFrontPlaneGridSRV.Get(),
					m_pRandomTableSRV.Get(), // 乱数テーブルの可視化テスト用。
				};
				pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
				pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
				pDeviceContext->CSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}
			{
				ID3D11UnorderedAccessView* const ppUAViews[] =
				{
					pWaveSimUAV,
				};
				pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
			}
			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_pSubColorBufferRTV.Get(),
				};
				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, m_pSubDSV.Get());
			}

			// コンピュート シェーダーによる波面シミュレーション テスト。
			// 対象領域（ドメイン）を矩形や直方体で覆う。
			// 実際の入出力データ構造には 2D/3D テクスチャを使うため、有効グリッドも無効グリッドも一律すべて処理する。
			this->Dispatch(m_effectTechs.CSTechSimpleComputingTest, ComputeDispatchCountX, 1, 1);

			// ピクセル シェーダー 4.0 (ps_4_0) は UAV 経由で直接バッファ アクセスできない。SRV 経由のみ。
			// また、RW バッファは SRV 経由でアクセスできない（RW テクスチャは SRV / UAV / RTV が使えるが、シェーダー 5.0 必須）。
			// SRV 経由でアクセスするため、またフレームを進めるためにビューのフリップを行なう。
			// つまり、コンピュート シェーダーの入力（SRV）と出力（UAV）を入れ替える。
			// なお、ps_5_0 であれば、UAV 経由で直接アクセス可能となるが、
			// UAV 経由での読み出しにはフォーマットの強い制約がある（R32_FLOAT のみ可能）。
			// なお、UAV / RTV を使って出力先としてバインドしていたリソースを、SRV を使って入力元としてバインドする場合、
			// あるいはその逆の操作をする場合、いったんビューをともにアンバインドする必要がある。
			// コンピュート シェーダーによって UAV 経由で出力したデータを別の同一フォーマット テクスチャに
			// CopyResource() でコピーして SRV 経由で入力として参照する方法もあるが、
			// フレームごとに In/Out のビューをフリップする方法のほうが効率的。
			// なお、コンピュート シェーダーの出力結果をシステム メモリ側にマップして CPU で読み取る必要がある場合は、
			// CopyResource() が必須となる（UAV や RTV をバインド可能にしたテクスチャ、つまり GPU で書き込み可能にしたリソースは直接 CPU アクセスできないようになっている）。

			{
				CBufferLightParamsPack lightParam;
				ApplyTestLightColor(lightParam);
				lightParam.LightPos = MyMath::Vector3F(100, 300, 100);
				this->UpdateCBuffer(&lightParam);

				CBufferMeshPartAttributePack shaderMeshParam;
				// ダイナミックレンジで照明計算するので、0～1 の範囲外でも OK。
				shaderMeshParam.MaterialColorDiffuse = MyMath::Vector4F(0.6f, 0.8f, 1.0f, 1.0f);
				shaderMeshParam.MaterialColorAmbient = MyMath::Vector4F(0.8f, 0.8f, 0.8f, 1.0f);
				shaderMeshParam.MaterialColorSpecular = MyMath::Vector4F(1.6f, 1.8f, 2.0f, 1.0f);
				shaderMeshParam.MaterialOpacityAlpha = 0.7f;
				shaderMeshParam.MaterialSpecularPower = 4.0f;
				this->UpdateCBuffer(&shaderMeshParam);
			}

			// シミュレーション結果の可視化。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCNT.Get());
			this->SetMyVertexBuffer(m_pWaveFrontPlaneVertexBufferPCNT.Get(), sizeof(MyVertexTypes::MyVertexPCNT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			this->SetOneQuadIndexBuffer();

			//this->DrawOneQuad(m_effectTechs.TechDisplaySimpleComputingTest);
			// 水面グリッドの全頂点バッファを用意せず、インスタンス描画で対応する。
			// 有効グリッドのインデックスが書き込まれた専用バッファを SRV 経由で参照しながら、有効グリッドのみ描画する。
			this->DrawQuadInstances(m_effectTechs.TechDisplaySimpleComputingTest, m_waveFrontPlaneGridCount);

			// 並列リダクションのテスト。
			// シェーダーモデル 5.0 での最適化を行なえば、どうやらピクセル シェーダーよりもコンピュート シェーダーのほうがパフォーマンスが高くなる模様？　あまり変わらない？
			// TODO: トーン マッピングへの応用。

			const uint32_t reductionSrcImageSize = COMPUTING_TEMP_WORK_SIZE;
			auto* pReductionSrcImageView = m_ppWaveSimWorkSRV[0].Get();
#if 0
			// [A] ピクセル シェーダー版。
			this->DoPsParellelReductionTest(pReductionSrcImageView, reductionSrcImageSize);
#elif 1
			// [B] コンピュート シェーダー版。
			this->DoCsParellelReductionTest(pReductionSrcImageView, reductionSrcImageSize);
#endif

			if (advancesFrame)
			{
				m_waveSimInOutFlipCounter = (m_waveSimInOutFlipCounter + 1) % WaveSimWorkBufferCount;
			}
		}

#pragma endregion

		// ビューポートをスクリーンのサイズに合わせる。
		SetupSingleViewport(pDeviceContext, m_myCameraSettings.GetScreenWidthF(), m_myCameraSettings.GetScreenHeightF());

#pragma region // MSAA 法線・深度マップを張り付けた矩形ポリゴンを非 MSAA 法線・深度マップに描画（転送）する。//
		if (m_myEffectSettings.EnablesToonInk)
		{
			ClearRtvSrvUavSlots(pDeviceContext);

			// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_pNormalDepthMapRTV.Get(),
				};

				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
			}
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					m_pMSAANormalDepthMapSRV.Get(), // MSAA 転送ソース用のテクスチャ スロットに割り当てる。
				};
				//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
				pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}

			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

			// 矩形の頂点バッファとインデックス バッファをセット。
			this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			this->SetOneQuadIndexBuffer();

			// レンダーターゲットを単色で塗りつぶす。
			//pDeviceContext->ClearRenderTargetView(m_pFurGeoMapRTV, D3DXCOLOR(0.3f, 0.3f, 0.8f, 1.0f));
			//pDeviceContext->ClearRenderTargetView(m_pFurGeoMapRTV, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));

			this->TransportByCurrentMSAA();
		}
#pragma endregion

#pragma region // MSAA ファーマップを張り付けた矩形ポリゴンを非 MSAA ファーマップに描画（転送）する。//
		if (m_myEffectSettings.EnablesImageBasedFur)
		{
			ClearRtvSrvUavSlots(pDeviceContext);

			// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_pFurGeoMapRTV.Get(),
				};
				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
			}
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					m_pMSAAFurGeoMapSRV.Get(), // MSAA 転送ソース用のテクスチャ スロットに割り当てる。
				};
				pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}

			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

			// 矩形の頂点バッファとインデックス バッファをセット。
			this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			this->SetOneQuadIndexBuffer();

			// レンダーターゲットを単色で塗りつぶす。
			//pDeviceContext->ClearRenderTargetView(m_pFurGeoMapRTV, D3DXCOLOR(0.3f, 0.3f, 0.8f, 1.0f));
			//pDeviceContext->ClearRenderTargetView(m_pFurGeoMapRTV, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));

			this->TransportByCurrentMSAA();
		}
#pragma endregion

#pragma region // 法線・深度マップを参照して、スクリーンのピクセルごとに輪郭線抽出を実行する。//
		if (m_myEffectSettings.EnablesToonInk)
		{
			ClearRtvSrvUavSlots(pDeviceContext);

			// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_pSubColorBufferRTV.Get(),
				};

				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, m_pSubDSV.Get());
			}
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					m_pNormalDepthMapSRV.Get(),
				};
				//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
				pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}

			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

			// 矩形の頂点バッファとインデックス バッファをセット。
			this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			this->SetOneQuadIndexBuffer();

			this->DrawOneQuad(m_effectTechs.TechEdgeDetectColorSketch);
		}
#pragma endregion

#pragma region // ファーマップを参照して、スクリーンのピクセルごとにファー（ライン プリミティブ）を生成する。//
		if (m_myEffectSettings.EnablesImageBasedFur)
		{
			ClearRtvSrvUavSlots(pDeviceContext);

			// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
			{
				ID3D11RenderTargetView* const ppRTViews[] =
				{
					m_pSubColorBufferRTV.Get(),
				};

				pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, m_pSubDSV.Get());
			}
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					m_pFurGeoMapSRV.Get(),
				};
				pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews); // VTF に必要。
				pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}

			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

			// スクリーンの各ピクセルに対応したポイント プリミティブ集合の頂点バッファをセット。
			this->SetMyVertexBuffer(m_pImageBasedFurVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

#if 1
			// イメージ ベースのファーを合成する。
			this->DrawPrimitive(m_effectTechs.TechImageBasedFur, m_imageBasedFurVertexCount);
#else
			m_effectTechs.TechImageBasedFur->GetPassByIndex(0)->Apply(0, pDeviceContext);
			pDeviceContext->DrawInstanced(1, m_myCameraSettings.m_screenWidth * m_myCameraSettings.m_screenHeight, 0, 0);
#endif
		}
#pragma endregion

#pragma region // FP16 MSAA カラーテクスチャを張り付けた矩形ポリゴンを非 MSAA ダウンサンプル テクスチャに描画（転送）する。//

		ClearRtvSrvUavSlots(pDeviceContext);

		// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_ppDownSampledTempWorkRTV[0].Get(),
			};
			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_pSubColorBufferSRV.Get(), // MSAA 転送ソース用のテクスチャ スロットに割り当てる。
			};
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		// デバイスに頂点レイアウトをセット。
		pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

		// 矩形の頂点バッファとインデックス バッファをセット。
		this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		this->SetOneQuadIndexBuffer();

		// ビューポートをダウンサンプル先テクスチャのサイズに合わせる。
		SetupSingleViewport(pDeviceContext, DOWN_SAMPLED_TEX_SIZE, DOWN_SAMPLED_TEX_SIZE);

		// レンダーターゲット（バック バッファ）を単色で塗りつぶす。
		pDeviceContext->ClearRenderTargetView(m_ppDownSampledTempWorkRTV[0].Get(), &MyMath::ZERO_VECTOR4F.x);

		// 高輝度成分のみをワーク テクスチャ 0 に抽出する。
#if 1
		this->ExtractHighIntensityByCurrentMSAA();
#else
		this->TransportByCurrentMSAA();
#endif

		ClearRtvSrvUavSlots(pDeviceContext);

		// ブルーム用テクスチャ（ダウンサンプル テクスチャのガウスぼかし）を作成する。
#if 0
		// [A] ピクセル シェーダー版。
		this->ApplyPsBlurToDownSampledTex();
#else
		// [B] コンピュート シェーダー版。
		this->ApplyCsBlurToDownSampledTex();
#endif

		ClearRtvSrvUavSlots(pDeviceContext);

		// ビューポートをスクリーンのサイズに合わせる。
		SetupSingleViewport(pDeviceContext, m_myCameraSettings.GetScreenWidthF(), m_myCameraSettings.GetScreenHeightF());

#pragma endregion

		// 以降は非 MSAA のバックバッファに直接レンダリングしていく。

#pragma region // FP16 MSAA カラーテクスチャを張り付けた矩形ポリゴンを非 MSAA バック バッファに描画（転送）する。//

		// デバイスにレンダーターゲットと深度ステンシルをセット。（深度ステンシルのセットは任意）
		{
			ID3D11RenderTargetView* const ppRTViews[] =
			{
				m_pBackBufferRTV.Get(),
			};
			pDeviceContext->OMSetRenderTargets(ARRAYSIZE(ppRTViews), ppRTViews, nullptr);
		}
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_pSubColorBufferSRV.Get(), // MSAA 転送ソース用のテクスチャ スロットに割り当てる。
			};
			//pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}

		// デバイスに頂点レイアウトをセット。
		pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

		// 矩形の頂点バッファとインデックス バッファをセット。
		this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		this->SetOneQuadIndexBuffer();

		// レンダーターゲット（バック バッファ）を単色で塗りつぶす。
		//pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, D3DXCOLOR(0.3f, 0.3f, 0.8f, 1.0f));
		//pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, D3DXCOLOR(0.0f, 0.5f, 0.5f, 1.0f));
		static const MyMath::Vector4F clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), &clearColor.x);

		// 転送。
#if 1
		this->TransportByCurrentMSAA();
#endif

#if 1
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				m_ppDownSampledTempWorkSRV[0].Get(),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
			};
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		// ぼかされたダウンサンプル テクスチャを加算合成。
		// ダウンサンプル時に高輝度成分のみ抽出していたら、ブルーム エフェクトをかけることになる。
		if (m_myEffectSettings.EnablesBloomEffect)
		{
			this->DrawOneQuad(m_effectTechs.TechAddDownSampledTex);
		}
#endif
#if 0
		// 波紋を合成。
		this->DrawOneQuad(m_effectTechs.TechDisplaySimpleComputingTest);
#endif

#pragma endregion

#pragma region // シャドウマップのテスト描画。//
		if (true)
		{
			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					nullptr,
					nullptr,
					nullptr,
					m_pShadowColorBufferSRV.Get(), // 確認テスト用。
					nullptr,
				};
				pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}

			const int scaledShadowSize = 128;

			pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

			this->SetMyVertexBuffer(m_pOneSquareVertexBufferPCT.Get(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			this->SetOneQuadIndexBuffer();

			CBufferViewParamsPack viewParam;
			//pDeviceContext->RSSetState(m_pRasterStateShadow.Get());
			viewParam.UniScreenSize.x = m_myCameraSettings.m_screenWidth;
			viewParam.UniScreenSize.y = m_myCameraSettings.m_screenHeight;
			MyMath::Matrix4x4F mS, mT, mViewProj;
			MyMath::CreateTrMatrixScaling(&mS, &MyMath::Vector3F(
				+0.5f * scaledShadowSize,
				-0.5f * scaledShadowSize,
				1));
			MyMath::CreateTrMatrixTranslation(&mT, &MyMath::Vector3F(
				+1,
				-1,
				0));
			MyMath::CreateTrMatrixOrtho2DRH(&mViewProj,
				0,
				m_myCameraSettings.GetScreenWidthF(),
				m_myCameraSettings.GetScreenHeightF(),
				0);
			MyMath::MultiplyMatrix(&viewParam.UniWorldViewProjMatrix,
				&mViewProj, &mS, &mT);
			this->UpdateCBuffer(&viewParam);
			this->DrawOneQuad(m_effectTechs.TechShadowHudTest);
			//pDeviceContext->GSSetShader(nullptr, nullptr, 0);
			//pDeviceContext->RSSetState(m_pRasterStateSolidMsaa.Get());
		}
#pragma endregion

		// パーティクルの検証。
		// 描画キックに使うのはダミーの頂点バッファ。
		// 実際の各パーティクル データはすべて構造化バッファで管理し、更新はすべてコンピュート シェーダーで行なう。
		if (false)
		{
			{
				ID3D11UnorderedAccessView* const ppUAViews[] =
				{
					m_pFlakeParticleUAV.Get(),
				};
				pDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(ppUAViews), ppUAViews, nullptr);
			}

			if (advancesFrame)
			{
				const UINT dispatchX = UINT(std::ceil(float(m_flakeParticleCount) / float(MY_FLAKE_CS_LOCAL_GROUP_SIZE * MY_FLAKE_CS_PARTICLES_NUM_PER_THREAD)));

				this->Dispatch(m_effectTechs.CSTechUpdateFlakeParticle, dispatchX, 1, 1);
			}

			ClearUavSlots(pDeviceContext, 1);

			{
				ID3D11ShaderResourceView* const ppSRViews[] =
				{
					m_pFlakeParticleSRV.Get(),
				};
				pDeviceContext->VSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
			}

			pDeviceContext->IASetInputLayout(m_pInputLayoutP.Get());

			this->SetMyVertexBuffer(m_pSinglePointVertexBufferP.Get(), sizeof(MyVertexTypes::MyVertexP), D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

			this->DrawInstancedPrimitive(m_effectTechs.TechFlakeParticle, 1U, m_flakeParticleCount);
		}

		if (false)
		{
			pDeviceContext->IASetInputLayout(m_pInputLayoutP.Get());

			// NVIDIA GeForce GTX 760 で試した結果だが、インスタンス数が数万のオーダーになると突然失速を始めるという現象が発生する。
			// テッセレータ／ジオメトリ シェーダーを問わない。
			// 32bit/64bit ともに変わらない。
			// パーティクル情報の保持と管理・操作は ByteAddressBuffer よりも StructuredBuffer のほうが楽だが、
			// この問題があるかぎり StructuredBuffer＋インスタンス描画よりも ByteAddressBuffer を使うしかない？
			// GeForce ドライバーのバグかもしれないが、CAPCOM が BAB を使っているくらいなので、DirectX 11 の限界なのかもしれない。
			// DirectX 12 では改善されるのだろうか……
			// → 同数の頂点バッファを用意してみたが、むしろインスタンシングのほうが高速だった。
			// 数万で失速しているのは、描画面積が大きいせいかもしれない。
			// → 描画面積を小さくすると改善。200x200平方ピクセル×25k個程度の描画だとピクセル フィルレートを食いつぶしてしまうらしい。
#if 0
			// テッセレータで増幅して Quad にする。
			this->SetMyVertexBuffer(m_pSinglePointVertexBufferP.Get(), sizeof(MyVertexTypes::MyVertexP), D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
			//this->DrawPrimitive(m_effectTechs.TechTessBillboard, 1U);
			this->DrawInstancedPrimitive(m_effectTechs.TechTessBillboard, 1U, 25000);
#else
			// ジオメトリ シェーダーで増幅して Quad にする。
			this->SetMyVertexBuffer(m_pSinglePointVertexBufferP.Get(), sizeof(MyVertexTypes::MyVertexP), D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			//this->DrawPrimitive(m_effectTechs.TechGeomBillboard, 1U);
			this->DrawInstancedPrimitive(m_effectTechs.TechGeomBillboard, 1U, 25000);
#endif
		}

		//static auto textOriginPoint = D2D1::Point2F(0.0f, 0.0f);

#pragma region // HUD 文字列の描画。最前面になるように、3D シーンがバック バッファにレンダリングされた後で、最後に実施する。//
		{
			ID3D11ShaderResourceView* const ppSRViews[] =
			{
				nullptr,
				nullptr,
				m_pHudFontAlphaTexSRV.Get(),
				nullptr,
				nullptr,
			};
			pDeviceContext->PSSetShaderResources(0, ARRAYSIZE(ppSRViews), ppSRViews);
		}
		// バック バッファは MSAA ではないことに注意。
		// もしポリゴンのエッジを描画する場合、MSAA ターシャリ バッファに描画するようにしたほうがよい。
		// なお、MSAA はアルファトゥカバレッジを使わないと、アルファ チャンネル・アルファ マップで透過したピクセルに AA がかからない。
		{
			CBufferViewParamsPack viewParam;
			MyMath::CreateTrMatrixOrtho2DRH(&viewParam.UniWorldViewProjMatrix,
				0,
				m_myCameraSettings.GetScreenWidthF(),
				m_myCameraSettings.GetScreenHeightF(),
				0);
			this->UpdateCBuffer(&viewParam);

			static wchar_t messageString[1024];
			if (advancesFrame)
			{
				swprintf_s(messageString, L"D3D: PeriodicMode: %7.1f FPS", m_fpsCounter.GetFpsValue());
				//swprintf_s(messageString, L"%f,%f", textOriginPoint.x, textOriginPoint.y);
			}
			else
			{
				wcscpy_s(messageString, L"D3D: EventDrivenMode");
			}

			// 頂点バッファを書き換えるのと、定数バッファを書き換えるのとではどちらがよい？
			// 一度にレンダリングできるのが最大1024文字程度であれば、定数バッファ1スロットで4096ベクトルまかなえるので
			// 位置 float2、大きさ float、テクスチャ座標 float2×2、色 float4 は十分収まりきる。
			// OpenGL Uniform Block 仕様との兼ね合いも考える必要はあるが……

			//const LPCWSTR pSrcString = L"Test";
			const MyMath::Vector2F msgStartPos(4, 4);
			const MyMath::Vector4F upperColor(1.0f, 0.7f, 0.7f, 1);
			const MyMath::Vector4F lowerColor(0.7f, 0.7f, 1.0f, 1);
			//const MyMath::Vector4F upperColor(1.0f, 1.0f, 0.0f, 1);
			//const MyMath::Vector4F lowerColor(1.0f, 1.0f, 0.0f, 1);
			ATLVERIFY(m_fontRects.UpdateVertexBufferByString(pDeviceContext,
				messageString,
				msgStartPos, upperColor, lowerColor,
				m_pHudFontTexData->FontHeight,
				m_pHudFontTexData->UsesMonospaceFont,
				m_pHudFontTexData->TextureData.TextureWidth,
				m_pHudFontTexData->TextureData.TextureHeight,
				m_pHudFontTexData->CodeUVMap));

			// デバイスに頂点レイアウトをセット。
			pDeviceContext->IASetInputLayout(m_pInputLayoutPCT.Get());

			// フォント テクスチャを参照して、フォントを描画する。
			// Z バッファを書き込まない。（Z バッファを参照する必要すらないかも）
			this->SetMyVertexBuffer(m_fontRects.GetVertexBuffer(), sizeof(MyVertexTypes::MyVertexPCT), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			pDeviceContext->IASetIndexBuffer(m_fontRects.GetIndexBuffer(), MyDynamicManyRectsBase::IndexFormat, 0);

			const int stringLength = m_fontRects.GetStringLength();
			if (stringLength > 0)
			{
				this->DrawIndexedPrimitive(m_effectTechs.TechFont, stringLength * 6U);
			}
			//this->DrawOneQuad(m_effectTechs.TechFont);
		}
#pragma endregion
	}


	bool MyD3DManager::ResizeScreen(UINT width, UINT height)
	{
		// スクリーン サイズに変更があった場合、スワップ チェーンのリサイズや深度バッファの再作成などが必要。
		if (width == m_myCameraSettings.m_screenWidth && height == m_myCameraSettings.m_screenHeight)
		{
			return false;
		}

		// レンダーターゲット カラーテクスチャおよびそのビューを作成する。
		if (!this->CreateRenderTargetColorTexAndView(renderTargetTexFormat, width, height, m_msaaSampleCount, m_msaaQuality))
		{
			return false;
		}

		// 深度ステンシル テクスチャおよびそのビューを作成する。
		if (!this->CreateDepthStencilAndView(width, height, m_msaaSampleCount, m_msaaQuality))
		{
			return false;
		}

		// バック バッファのレンダーターゲット ビューを作成する。
		if (!this->CreateBackBufferRenderTargetView(width, height))
		{
			return false;
		}

		if (!this->CreateImageBasedFurVertexBuffer(width, height))
		{
			return false;
		}

		m_myCameraSettings.m_screenWidth = width;
		m_myCameraSettings.m_screenHeight = height;

		return true;
	}

	bool MyD3DManager::ResizeShadowBuffer(uint32_t width, uint32_t height, uint32_t cascadeCount)
	{
		_ASSERTE(m_pD3DDevice != nullptr);
		// UNDONE: エラー発生時に途中 return すると、RTV や SRV が不正なまま（作成した Texture と食い違う状態）になってしまう。
		// ロールバックできるようにするか、すべての RTV や SRV を先にいったん解放しておく必要がある。

		HRESULT hr = E_FAIL;

		// TODO: シャドウの解像度設定情報を保持しておく。同じ場合は変更しない。

		const auto shadowTexFormat = DXGI_FORMAT_R32G32_FLOAT; // 深度と深度の2乗を格納する。
		const auto shadowDSFormat = DXGI_FORMAT_D32_FLOAT;

		// API の仕様に関する制約。
		// TODO: 実際のシステム リソースの制約も加わる。
		_ASSERTE(1 <= width && width <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
		_ASSERTE(1 <= height && height <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
		_ASSERTE(1 <= cascadeCount && cascadeCount <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

		D3D11_TEXTURE2D_DESC description = {};
		description.ArraySize = cascadeCount;
		description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		description.Format = shadowTexFormat;
		description.Width = width;
		description.Height = height;
		description.MipLevels = 1;
		description.SampleDesc.Count = 1;
		description.SampleDesc.Quality = 0;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pShadowColorBufferTex.ReleaseAndGetAddressOf());

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pShadowBlurWorkTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

		// テクスチャ配列をレンダーターゲットとする場合、同時に使用する深度ステンシルは同様にテクスチャ配列にする必要がある。
		description.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		description.Format = shadowDSFormat;

		hr = m_pD3DDevice->CreateTexture2D(
			&description,
			nullptr, // no initial data
			m_pShadowDepthStencilTex.ReleaseAndGetAddressOf());

		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pShadowColorBufferTex.Get(), m_pShadowColorBufferSRV))
		{
			return false;
		}

		if (!this->CreateMyShaderResourceView(m_pShadowBlurWorkTex.Get(), m_pShadowBlurWorkSRV))
		{
			return false;
		}

#if 0
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = cascadeCount;
		rtvDesc.Texture2DArray.MipSlice = 0;
#endif

		if (!this->CreateMyRenderTargetView(m_pShadowColorBufferTex.Get(), m_pShadowColorBufferRTV))
		{
			return false;
		}

		if (!this->CreateMyUnorderedAccessView(m_pShadowColorBufferTex.Get(), m_pShadowColorBufferUAV))
		{
			return false;
		}

		if (!this->CreateMyRenderTargetView(m_pShadowBlurWorkTex.Get(), m_pShadowBlurWorkRTV))
		{
			return false;
		}

		if (!this->CreateMyUnorderedAccessView(m_pShadowBlurWorkTex.Get(), m_pShadowBlurWorkUAV))
		{
			return false;
		}

#if 0
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = shadowDSFormat;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = cascadeCount;
		dsvDesc.Texture2DArray.MipSlice = 0;
#endif

		if (!this->CreateMyDepthStencilView(m_pShadowDepthStencilTex.Get(), m_pShadowDSV))
		{
			return false;
		}

		return true;
	}

} // end of namespace
