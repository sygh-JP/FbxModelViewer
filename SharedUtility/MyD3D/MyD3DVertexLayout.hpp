#pragma once

// ID3D11InputLayout 生成のための頂点レイアウトを定義する。
// AlignedByteOffset には D3D11_APPEND_ALIGNED_ELEMENT を指定するとよい。
// 対応する C/C++ 頂点構造体のメンバー定義順に合わせて記述すれば、
// Format メンバーをもとに自動計算してくれる。
// Direct3D 9 での IDirect3DVertexDeclaration9 生成のための D3DVERTEXELEMENT9 構造体配列に近いが、
// データ末尾に D3DDECL_END による番兵（sentinel）を付ける方式ではなくなっている。

namespace MyD3D
{
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayT[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};


	//! @brief  ポイント プリミティブの描画などに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayP[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};


	//! @brief  ライン プリミティブの描画などに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayPC[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};


	//! @brief  ダウンサンプル転送などに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayPCT[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

#if 0
	//! @brief  フォントの描画などに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC c_layoutDescArrayP4CT[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                 D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, sizeof(float) * 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const UINT c_layoutElemCountP4CT = ARRAYSIZE(c_layoutDescArrayP4CT);
#endif

	//! @brief  三角形プリミティブの描画などに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayPCNT[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};


	//! @brief  スキニングに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayPNTIW[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,         0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// 符号なし整数フォーマット DXGI_FORMAT_R8G8B8A8_UINT を使って、BYTE[4] をバインドする。
		// HLSL には byte4 がないので、uint4 にバインドされることになるが、マッピングは Direct3D が自動でやってくれる。
		// 符号なし正規化整数フォーマットとして DXGI_FORMAT_R8G8B8A8_UNORM があるが、
		// Direct3D/HLSL では頂点レイアウトに直接整数型を含めることができるので、今回は正規化をする必要がない。
		// OpenGL/GLSL も、3.3 以降は頂点属性に直接整数型を含めることができる。
		{ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#if 1
		{ "BLENDINDICES", 1, DXGI_FORMAT_R8G8B8A8_UINT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#endif
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#if 1
		{ "BLENDWEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#endif
	};

#if 0
	//! @brief  フレーク パーティクルに使用される頂点レイアウト宣言。<br>
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutElemDescArrayFlake[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
#endif

} // end of namespace
