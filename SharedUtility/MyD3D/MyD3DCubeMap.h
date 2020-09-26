#pragma once


#include "MyNoncopyable.hpp"
#include "MyBasicVertexTypes.hpp"


// DirectX SDK サンプルの CubeMapGS を参考に作成。
// DirectX 10 以降では、ジオメトリ シェーダーと MRT を活用することで
// キューブマップの6面をリアルタイム レンダリングする際に1パスで描画できる。

// なお、D3D11_RESOURCE_MISC_TEXTURECUBE に対して D3D11_USAGE_DYNAMIC は使えないらしい。
// つまり D3D11_CPU_ACCESS_WRITE が使えない。
// CPU で書き換える場合、ID3D11DeviceContext::Map()/Unmap() は使えず、リソースの再作成から行なう必要がある。
// Direct3D テクスチャ ソースデータのバイナリ仕様は DDS 同様、
// ( ( ( ( ( width * height ) * volume_depth ) * mip ) * cube_face ) * array )
// となっているが、
// CPU 側でキューブマップ画像の6面すべてを用意して初期化するのは大変。
// DirectXTex を使えば D3DX のように DDS ファイルから直接キューブマップをロードできる。ミップマップ生成もツール側で制御できる。
// キューブマップの DDS 作成は旧 DirectX SDK 付属の DirectX Texture Tool もしくは NVIDIA の Photoshop プラグインを使うとよさげ。
// ・NVIDIA Texture Tools for Adobe Photoshop
// https://developer.nvidia.com/nvidia-texture-tools-adobe-photoshop
// 自然景観の環境マッピングを行なう場合、キューブマップ用の景観画像6枚の生成には Terragen を使うと手っ取り早い。
// Terragen 3 であれば標準で TIFF や Windows BMP に対応している。

// スカイドーム・スカイボックス用途のスフィアマップやキューブマップは、事前レンダリングされた静的なテクスチャ（DDS ファイル）を使う。
// ミラーボックスなど、周囲の状況をリアルタイムに反映して疑似レイトレースのような表現を行なうには、動的なレンダーテクスチャを使う。


namespace MyD3D
{
	typedef MyVertexTypes::MyVertexPNT CubeMapVertex;


	// キューブマップ用テクスチャ画像データをもとに作成する。
	class MyCubeMap final : MyUtils::MyNoncopyable<MyCubeMap>
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D>           m_pEnvMap;           // Environment map
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_pEnvMapSRV;        // Shader resource view for the cubic env map
	public:
		ID3D11ShaderResourceView* GetSRV() const { return m_pEnvMapSRV.Get(); }
	public:
		MyCubeMap()
		{
		}

		bool Create(ID3D11Device* pDevice, DXGI_FORMAT texFormat, uint32_t imageWidth, uint32_t imageHeight, uint32_t mipLevels, const uint8_t* pImageData, size_t imageDataSizeInBytes);

		// HACK: 上記はほぼ DDS 専用。6面の DIB 画像データを個別に供給するメソッドもあるとよい。PNG などに簡易対応できる。
		// もしくは呼び出し側で結合データを作成するためのヘルパーを用意する。

		void Release()
		{
			// 再作成に備えて破棄もできるようにしておく。
			// なお、ココでスマートポインタをリセットして COM インターフェイスを解放したとしても、
			// D3D デバイスにセットしたまま参照カウントが残っていると、
			// リソースが破棄されないので注意。
			m_pEnvMap.Reset();
			m_pEnvMapSRV.Reset();
		}
	};


	// GPU で各面をレンダリング可能なキューブマップ。CPU で書き換えることはできない。
	class MyRenderTargetCubeMap final : MyUtils::MyNoncopyable<MyRenderTargetCubeMap>
	{
		MyMath::MatrixF m_amCubeMapViewAdjust[6]; // Adjustment for view matrices when rendering the cube map
		Microsoft::WRL::ComPtr<ID3D11Texture2D>           m_pEnvMap;           // Environment map
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>    m_pEnvMapRTV;        // Render target view for the alpha map
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>    m_apEnvMapOneRTV[6]; // 6 render target view, each view is used for 1 face of the env map
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_pEnvMapSRV;        // Shader resource view for the cubic env map
		//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_apEnvMapOneSRV[6]; // Single-face shader resource view
		// --> DX SDK のサンプルでは SRV 配列も定義されていたが使われていなかった。
		// 2D テクスチャ配列 Texture2DArray 中の特定のスライスだけをターゲットにする SRV/RTV/UAV は、
		// D3D11_SHADER_RESOURCE_VIEW_DESC::Texture2DArray.FirstArraySlice や
		// D3D11_RENDER_TARGET_VIEW_DESC::Texture2DArray.FirstArraySlice や
		// D3D11_UNORDERED_ACCESS_VIEW_DESC::Texture2DArray.FirstArraySlice にオフセットを指定することで作成できるらしい（ArraySize は 1 にする）。
		// ただし Texture2DArray のリソースに対して Texture2D 単体の SRV/RTV/UAV を作成することは不可能。リソースのタイプを合わせなければならない。
		// キューブマップの場合はどうすればいいのか不明。
		Microsoft::WRL::ComPtr<ID3D11Texture2D>           m_pEnvMapDepth;      // Depth stencil for the environment map
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>    m_pEnvMapDSV;        // Depth stencil view for environment map for all 6 faces
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>    m_pEnvMapOneDSV;     // Depth stencil view for environment map for all 1 face

		Microsoft::WRL::ComPtr<ID3D11Buffer>              m_pVBVisual;         // Vertex buffer for quad used for visualization
	public:
		MyRenderTargetCubeMap()
		{
			std::fill_n(m_amCubeMapViewAdjust, ARRAYSIZE(m_amCubeMapViewAdjust), MyMath::ZERO_MATRIXF);
		}

		bool Create(ID3D11Device* pDevice, uint32_t mipLevels = 9, uint32_t envMapSizePixel = 256);
	};

}
