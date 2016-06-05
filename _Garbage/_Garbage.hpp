#pragma once

// もはや使わなくなったが、参考までに残しておくソース群。

//#define USE_D3D9
#define USE_D3D10_1
//#define USE_D3D11

#ifdef USE_D3D9

#ifdef _DEBUG
#define D3D_DEBUG_INFO // おそらく Direct3D 9 専用。D3D 10 以降はデバイス作成時のフラグでデバッグ レイヤーを有効にする。
// http://msdn.microsoft.com/ja-jp/library/ee418554.aspx
#endif

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#ifdef _DEBUG
#pragma comment(lib, "d3dx9d.lib")
#else
#pragma comment(lib, "d3dx9.lib")
#endif

#elif defined(USE_D3D10_1)

//#include <d3d10.h> // Direct3D 10.0
#include <d3d10_1.h> // Direct3D 10.1
#include <d3dx10.h>
//#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3d10_1.lib")
#ifdef _DEBUG
#pragma comment(lib, "d3dx10d.lib")
#else
#pragma comment(lib, "d3dx10.lib")
#endif

#elif defined(USE_D3D11)

#include <d3d11.h>
#include <d3dx11.h>
#pragma comment(lib, "d3d11.lib")
#ifdef _DEBUG
#pragma comment(lib, "d3dx11d.lib")
#else
#pragma comment(lib, "d3dx11.lib")
#endif

#endif

// NOTE: もし Direct2D 1.0 との連携が必要で、D3D 10.1 で開発をせざるをえないが、
// 将来的に Direct2D 1.1 + D3D 11.1 への移行を検討している場合は、
// ID3D10～ などの型名は直接記述せず、IMyD3D～ などに typedef しておくとよいかもしれない。
// ただし VC++ 2008 の場合、D3D/D3DX の型に typedef を使うと、なぜかインテリセンスが機能しなくなることがある模様。
// なお、DirectX SDK June 2010 では、デフォルトで D3Dcommon.h にて ID3DBlob が typedef してあるなど、
// D3D 10 と D3D 11 で共通して使える型名がすでにいくつか存在することに注意。
// D3D 10 ではオブジェクト生成メソッドも描画メソッドも ID3D10Device/ID3D10Device1 が担当していたが、
// D3D 11 では描画メソッドが ID3D11Device から ID3D11DeviceContext へ委譲されている。
// また、D3D 10 では、エフェクト ユーティリティ Effects10 はシステム標準コア ライブラリでの提供だったが、
// D3D 11 では、エフェクト ユーティリティ Effects11 はシステム非標準の D3DX11 での提供となり、
// さらに COM DLL のバイナリではなく、スタティック ライブラリのソースコードの提供になっている。
// ソースコードは DX SDK サンプル Effects11 の中にある。
// Effects10 と Effects11 は型名の違いが大半で、機能的にはほぼ後方互換だが、互換性のない改変もいくつか含まれる。
// 詳しくは、「Effects 10 と Efects 11 の相違点」を参照。
// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476141.aspx
// http://msdn.microsoft.com/ja-jp/library/ee416152.aspx
// ちなみになぜか Effects10 では、
// ID3D10EffectVectorVariable::SetFloatVector(), SetIntVector(), SetBoolVector(), ...,
// ID3D10EffectMatrixVariable::SetMatrix(), ..., は、
// Setter なのに引数が const T* ではない。多分ただの定義ミス。
// Effects11 の ID3DX11EffectVectorVariable, ID3DX11EffectMatrixVariable では改善されている。

#ifdef USE_D3D10_1
//typedef ID3D10Device  IMyD3DDevice;
//typedef ID3D10Device  IMyD3DDeviceContext;
typedef ID3D10Device1  IMyD3DDevice;
typedef ID3D10Device1  IMyD3DDeviceContext;
#elif defined(USE_D3D11)
typedef ID3D11Device  IMyD3DDevice;
typedef ID3D11DeviceContext  IMyD3DDeviceContext;
#endif


#pragma region // エフェクト変数（行列、ベクトル、スカラーなど）のセットアップ。//
// エフェクト変数≒シェーダー定数であり、GLSL でいう uniform 変数に相当。
// 
// OpenGL/GLSL は最初から名前での個別アクセスが基本だったが、
// Direct3D/HLSL は最初からレジスタ番号でのアクセスが基本で、
// 名前でのアクセスは D3DX エフェクト インターフェイス のリフレクション経由（おまけ扱い）だった。
// なお、OpenGL 3.1 / ES 3.0 以降の Uniform Block & Uniform Buffer Object や、Direct3D 10 以降の Constant Buffer (cbuffer) は、
// シェーダー定数をレジスタ単位ではなくブロック単位で書き換えることが可能になり、API コールのオーバーヘッドなどを減らせるようになっている。
// 
// SRV/UAV エフェクト変数の取得・設定は、すべてのシェーダーリソース ビュー／アンオーダード アクセス ビューが
// C++ 側で作成された後で実行する必要がある。

//! @brief  ベクトル型エフェクト変数は4次元のみを扱う。<br>
inline void SetEffectVariableVector(_Inout_ ID3DEffectVectorVariable* pTarget, _In_ const MyMath::Vector4F* pVector)
{
	_ASSERTE(pTarget != nullptr);
#ifdef USE_D3D10_1
	pTarget->SetFloatVector(const_cast<float*>(&pVector->x));
#else
	pTarget->SetFloatVector(&pVector->x);
#endif
}

inline void SetEffectVariableMatrix(_Inout_ ID3DEffectMatrixVariable* pTarget, _In_ const MyMath::MatrixF* pMatrix)
{
	_ASSERTE(pTarget != nullptr);
#ifdef USE_D3D10_1
	pTarget->SetMatrix(const_cast<float*>(&pMatrix->_11));
#else
	pTarget->SetMatrix(&pMatrix->_11);
#endif
}

inline ID3DEffectMatrixVariable* GetEffectVariableAsMatrix(ID3DEffect* pEffect, LPCSTR pVariableName)
{
	_ASSERTE(pEffect != nullptr);

	auto* pVarTemp = pEffect->GetVariableByName(pVariableName);
	_ASSERTE(pVarTemp != nullptr);
	auto* pRetVal = pVarTemp->AsMatrix();
	_ASSERTE(pRetVal != nullptr);

	return pRetVal;
}

inline ID3DEffectVectorVariable* GetEffectVariableAsVector(ID3DEffect* pEffect, LPCSTR pVariableName)
{
	_ASSERTE(pEffect != nullptr);

	auto* pVarTemp = pEffect->GetVariableByName(pVariableName);
	_ASSERTE(pVarTemp != nullptr);
	auto* pRetVal = pVarTemp->AsVector();
	_ASSERTE(pRetVal != nullptr);

	return pRetVal;
}

inline ID3DEffectScalarVariable* GetEffectVariableAsScalar(ID3DEffect* pEffect, LPCSTR pVariableName)
{
	_ASSERTE(pEffect != nullptr);

	auto* pVarTemp = pEffect->GetVariableByName(pVariableName);
	_ASSERTE(pVarTemp != nullptr);
	auto* pRetVal = pVarTemp->AsScalar();
	_ASSERTE(pRetVal != nullptr);

	return pRetVal;
}

inline ID3DEffectShaderResourceVariable* GetEffectVariableAsShaderResource(ID3DEffect* pEffect, LPCSTR pVariableName)
{
	_ASSERTE(pEffect != nullptr);

	auto* pVarTemp = pEffect->GetVariableByName(pVariableName);
	_ASSERTE(pVarTemp != nullptr);
	auto* pRetVal = pVarTemp->AsShaderResource();
	_ASSERTE(pRetVal != nullptr);

	return pRetVal;
}

inline ID3DX11EffectUnorderedAccessViewVariable* GetEffectVariableAsUnorderedAccessView(ID3DX11Effect* pEffect, LPCSTR pVariableName)
{
	_ASSERTE(pEffect != nullptr);

	auto* pVarTemp = pEffect->GetVariableByName(pVariableName);
	_ASSERTE(pVarTemp != nullptr);
	auto* pRetVal = pVarTemp->AsUnorderedAccessView();
	_ASSERTE(pRetVal != nullptr);

	return pRetVal;
}

#pragma endregion


void DrawMeshSubset(ID3DX10Mesh* pMesh, ID3D10EffectTechnique* pTechnique, UINT attribId)
{
	D3D10_TECHNIQUE_DESC techDesc = {};
	pTechnique->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		pTechnique->GetPassByIndex(p)->Apply(0);
		// ID3D10EffectVectorVariable 経由の値設定はいったん保留され、
		// Apply() を呼んだタイミングで対応する定数バッファなどが実際に更新されるらしい。
		pMesh->DrawSubset(attribId);
	}
}

void DoD3DX10DrawMeshSample(ID3DX10Mesh* pMesh, ID3D10EffectTechnique* pTechnique)
{
	UINT attrSize = 0;
	pMesh->GetAttributeTable(nullptr, &attrSize);
	if (attrSize == 0)
	{
		// TODO: エフェクト変数や定数バッファ経由でデフォルトのマテリアルを設定する。
		DrawMeshSubset(pMesh, pTechnique, 0);
	}
	else
	{
		// 属性（マテリアル）の数だけループを回す。
		for (UINT atr = 0; atr < attrSize; ++atr)
		{
			// TODO: 属性テーブルからマテリアル配列のインデックスを取得し、
			// エフェクト変数や定数バッファ経由でマテリアルを設定する。
			DrawMeshSubset(pMesh, pTechnique, atr);
		}
	}
}


// D3DX10CompileFromFile(), D3DX11CompileFromFile(), D3DCompileFromFile() などのパラメータで指定する
// ターゲット プロファイルの ASCII 文字列は、エフェクトの場合、シェーダーモデルのバージョンではないことに注意。
// エフェクト フレームワークのプロファイル名となる。
// Direct3D 10 の Effects10 では "fx_4_0", "fx_4_1" が有効な名前。
// 一方、シェーダー 4.0, 4.0 level 9_x や 4.1 を使う場合でも、D3DX11 の Effects11 では "fx_5_0" のプロファイルを使用する。


#pragma region // ユーザー定義頂点データから ID3DX10Mesh を作成する例。参考までに。//

template<UINT Options, typename TIndex> bool CreateMyD3DX10SkinMeshImpl(CComPtr<ID3DX10Mesh>& pMesh, ID3D10Device* pDevice, const MyMath::TMySkinVertexArray& tempVB, const std::vector<TIndex>& tempIB, const MyMath::TMyAttributeRangeArray& tempAttrRangeArray)
{
	// Options 引数で 32bit インデックスを指定しない限り、16bit インデックスが使用される。
	HRESULT hr = D3DX10CreateMesh(pDevice,
		VertexInputLayoutElemDescArrayPNTIW,
		ARRAYSIZE(VertexInputLayoutElemDescArrayPNTIW),
		VertexInputLayoutElemDescArrayPNTIW[0].SemanticName,
		tempVB.size(),
		tempIB.size() / 3, // 全て三角形面に変換済みであることが前提。
		Options, &pMesh);
	if (FAILED(hr))
	{
		DXTRACE_ERR(_T("D3DX10CreateMesh() Failed!!"), hr);
		return false;
	}
	else
	{
		hr = pMesh->SetVertexData(0, &tempVB[0]);
		_ASSERTE(SUCCEEDED(hr));
		hr = pMesh->SetIndexData(&tempIB[0], tempIB.size());
		_ASSERTE(SUCCEEDED(hr));
		// ID3DX10Mesh::SetAttributeData() の使い方がいまいち不明。
		// ID3DX10Mesh の場合、デフォルトでサイズ1の属性テーブルが必ず生成されているみたい？
		if (tempAttrRangeArray.size() >= 2)
		{
			std::vector<D3DX10_ATTRIBUTE_RANGE> tempAttrTable(tempAttrRangeArray.size());
			for (size_t i = 0; i < tempAttrRangeArray.size(); ++i)
			{
				tempAttrTable[i].AttribId = tempAttrRangeArray[i].AttribId;
				tempAttrTable[i].FaceStart = tempAttrRangeArray[i].FaceStart;
				tempAttrTable[i].FaceCount = tempAttrRangeArray[i].FaceCount;
				tempAttrTable[i].VertexStart = tempAttrRangeArray[i].VertexStart;
				tempAttrTable[i].VertexCount = tempAttrRangeArray[i].VertexCount;
			}

			hr = pMesh->SetAttributeTable(&tempAttrTable[0], tempAttrTable.size());
			_ASSERTE(SUCCEEDED(hr));
		}
		hr = pMesh->CommitToDevice();
		_ASSERTE(SUCCEEDED(hr));
	}
	return true;
}

bool CreateMyD3DX10SkinMesh(CComPtr<ID3DX10Mesh>& pMesh, ID3D10Device* pDevice, const MyMath::TMySkinVertexArray& tempVB, const std::vector<UINT16>& tempIB, const MyMath::TMyAttributeRangeArray& tempAttrRangeArray)
{
	return CreateMyD3DX10SkinMeshImpl<0>(pMesh, pDevice, tempVB, tempIB, tempAttrRangeArray);
}

bool CreateMyD3DX10SkinMesh(CComPtr<ID3DX10Mesh>& pMesh, ID3D10Device* pDevice, const MyMath::TMySkinVertexArray& tempVB, const std::vector<UINT32>& tempIB, const MyMath::TMyAttributeRangeArray& tempAttrRangeArray)
{
	return CreateMyD3DX10SkinMeshImpl<D3DX10_MESH_32_BIT>(pMesh, pDevice, tempVB, tempIB, tempAttrRangeArray);
}

#pragma endregion


#if defined(USE_D3D10_1) && defined(D3DX10_DLL)
HRESULT SaveTextureAsFile(ID3D10Texture2D* pTexture, LPCWSTR pOutputImageFilePath)
{
	return D3DX10SaveTextureToFileW(pTexture, D3DX10_IFF_DDS, pOutputImageFilePath);
}
#elif defined(USE_D3D11) && defined(D3DX11_DLL)
HRESULT SaveTextureAsFile(ID3D11DeviceContext* pD3DImmediateContext, ID3D11Texture2D* pTexture, LPCWSTR pOutputImageFilePath)
{
	return D3DX11SaveTextureToFileW(pD3DImmediateContext, pTexture, D3DX11_IFF_DDS, pOutputImageFilePath);
}
#endif
