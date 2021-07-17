#include "stdafx.h"
#include "MyFbxVMeshBuilder.hpp"
#include "MyBoneMatrixPalettePack.h"
#include "MyTextureHelper.hpp"
#include "MyStopwatch.hpp"

#include "DebugNew.h"


namespace
{
	MyCommon::MyModelMeshDetailInfo::TSharedPtr CreateModelMeshInfo(
		const MyFbx::MyFbxMeshAnalyzer* pMeshAnalyzer,
		const MyFbx::MyFbxSkeletonAnalyzer* pSkeletonAnalyzer,
		const MyMath::TMyMaterialPtrsArray& materialsArray,
		const TIntArray& materialIndicesForAttrTable)
	{
		auto pModelMeshInfo = std::make_shared<MyCommon::MyModelMeshDetailInfo>();

		// HACK: 1つの FBX ファイルには1つ以上のメッシュ（パーツ、チャンク）が含まれるが、メッシュごとにマテリアル テーブルが
		// 作成される（そのメッシュで使われているマテリアルのみが含まれる配列が作成される）ので、場合によってはダブることになる。
		// テクスチャ テーブルの統合は大データの多重ロード・多重アロケートを防止するためにも絶対必須であり、すでに実装済みだが、
		// モデル単位でマテリアル テーブルも統合して、全メッシュ作成後にマテリアル インデックスを再構築したほうがよい。
		// UI で統一的にマテリアルを編集する場合に、ダブったものがあると扱いが厄介。
		// もしくは同じマテリアルをスマートポインタ経由のシャローコピーにしてしまう。
		// なお、ゲームを作る場合は、テクスチャ テーブルはレンダリング エンジンで1つ持てばよいが、
		// マテリアル テーブルはメッシュの集合（XNA の Model 相当）ごとに持たせたほうがよいかも。
		// トゥーン ディフューズ参照テクスチャもメッシュ集合に対して1つ持たせる。
		// もちろん、マテリアルをシーン全体で共有してもいいが、名前がダブったりすると管理しにくい。
		// ツリーでなくハッシュ テーブルであれば std::string/std::wstring などをキーにした名前検索でも比較的高速に処理できるはず。

		pModelMeshInfo->SetMaterialsArray(materialsArray);
		pModelMeshInfo->SetMaterialIndicesArrayForAttribTable(materialIndicesForAttrTable);
		pModelMeshInfo->SetMeshName(pMeshAnalyzer->GetMeshNameW().c_str());
		{
			const size_t skinCount = pMeshAnalyzer->GetSkinAnalyzerCount();
			if (skinCount > 0)
			{
				// スキン メッシュの場合、スケルトン階層構造とアニメーション情報を構築する。
				{
					// とりあえず 0 番スキン情報のみを使う。しかしマルチスキンレイヤーとかどういう使い道があるのか……
					// マルチ初期姿勢とか？
					auto pSkinAnalyzer = pMeshAnalyzer->GetSkinAnalyzer(0);
					// 先にスケルトン情報配列をセット。
					pModelMeshInfo->SetBoneSkeletonInfoArray(pSkinAnalyzer->GetBoneSkeletonInfoArray());
					pModelMeshInfo->SetBoneInitialPoseInfoArray(pSkinAnalyzer->GetBoneInitialPoseInfoArray());
					pModelMeshInfo->SetBoneAnimInfoArrayArray(pSkinAnalyzer->GetBoneAnimInfoArrayArray());
					pModelMeshInfo->SetRootBoneIndex(pSkinAnalyzer->GetRootBoneIndex());
				}

				// ボーン スケルトン階層構造情報とアニメーションのローカル ボーン行列の作成。
				// マルチトラック アニメーションおよびスケルトンのみでアニメーションが含まれない場合にも対処する。
				// FBX では通例ボーンごとのグローバル変換行列を扱うので、
				// 単純にすべてのパーツに対して単一のモーションを再生するだけならば階層構造情報の取得は不要だが、
				// パーツ別モーション ブレンドをする場合は分解・再構築が必要となる。
				// ちなみに DirectX RM の X フォーマットはデフォルトでローカル変換行列を扱い、再帰計算が必須となっていた。
				const auto& boneSkeletonInfoArray = pModelMeshInfo->GetBoneSkeletonInfoArray();
				auto boneInitialPoseInfoArray = pModelMeshInfo->GetBoneInitialPoseInfoArray();
				const auto& boneAnimInfoArrayArray = pModelMeshInfo->GetBoneAnimInfoArrayArray();
				for (size_t b = 0; b < boneSkeletonInfoArray.size(); ++b)
				{
					const size_t animCount = boneAnimInfoArrayArray.size();

					// クラスターとスケルトンを使ってアニメーション用のボーン階層構造を完成させる。
					auto boneSkeletonInfo = boneSkeletonInfoArray[b];
					auto& boneInitialPoseInfo = boneInitialPoseInfoArray[b];
					const auto& boneName = boneSkeletonInfo->GetBoneNameW();
					auto skeleton = pSkeletonAnalyzer->FindSkeleton(boneName);
					_ASSERTE(skeleton != nullptr);
					auto parent = skeleton->GetParent();
					const int parentBoneIndex = parent ? pModelMeshInfo->FindBoneIndex(parent->GetSkeletonNameW()) : MyMath::BoneSkeletonInfo::InvalidBoneIndex;
					if (parentBoneIndex >= 0)
					{
						// スケルトンには存在するがクラスターには存在しないボーンを含む FBX もある。
						// FBX Converter 付属の LocalMotionBlend.fbx がその例。
						boneSkeletonInfo->SetParentBoneIndex(parentBoneIndex);

						{
							const auto& globalInitMatrix = boneInitialPoseInfo.GetGlobalInitAttitudeMatrix();
							const auto& parentGlobalInitMatrix = boneInitialPoseInfoArray[parentBoneIndex].GetGlobalInitAttitudeMatrix();
							MyMath::MatrixF parentGlobalInitMatrixInv, localInitMatrix;
							MyMath::InverseMatrix(&parentGlobalInitMatrixInv, &parentGlobalInitMatrix);
							MyMath::MultiplyMatrix(&localInitMatrix, &globalInitMatrix, &parentGlobalInitMatrixInv);
							boneInitialPoseInfo.SetLocalInitAttitudeMatrix(localInitMatrix);
						}

						for (size_t a = 0; a < animCount; ++a)
						{
							auto pBoneAnimInfoArray = boneAnimInfoArrayArray[a];
							auto boneAnimInfo = pBoneAnimInfoArray->at(b);
							auto parentBoneAnimInfo = pBoneAnimInfoArray->at(parentBoneIndex);
							const size_t frameCount = boneAnimInfo->GetGlobalFrameAttitudeMatrices().size();

							for (size_t f = 0; f < frameCount; ++f)
							{
								const auto& globalFrameMatrix = boneAnimInfo->GetGlobalFrameAttitudeMatrices()[f];
								const auto& globalFrameQuat = boneAnimInfo->GetGlobalFrameAttitudeQuats()[f];
								const auto& parentGlobalFrameMatrix = parentBoneAnimInfo->GetGlobalFrameAttitudeMatrices()[f];
								const auto& parentGlobalFrameQuat = parentBoneAnimInfo->GetGlobalFrameAttitudeQuats()[f];

								// ・ココで使うべきは親のフレームごとのグローバル姿勢行列の逆行列 Gn^-1 か？
								// それともグローバル初期姿勢行列の逆行列 Gini^-1 か？
								// --> Gn^-1 らしい。Gini^-1 は最初に Gn を求めるとき以外に使い道がない？
								// ・ローカル フレーム行列は、ローカル初期姿勢行列 Lini を考慮しないと崩れるものがある？
								// 子のグローバル行列を求める際の行列の乗算順序がおかしいだけ？
								// --> 行列乗算順序がおかしかっただけらしい。
								MyMath::MatrixF parentGlobalFrameMatrixInv, localFrameMatrix;
								MyMath::InverseMatrix(&parentGlobalFrameMatrixInv, &parentGlobalFrameMatrix);
								MyMath::MultiplyMatrix(&localFrameMatrix, &globalFrameMatrix, &parentGlobalFrameMatrixInv);
								boneAnimInfo->AddLocalFrameAttitudeMatrix(localFrameMatrix);
							}
						}
					}
					else
					{
						// ルートだけはグローバルがローカルになる。
						boneInitialPoseInfo.SetLocalInitAttitudeMatrix(boneInitialPoseInfo.GetGlobalInitAttitudeMatrix());

						for (size_t a = 0; a < animCount; ++a)
						{
							auto pBoneAnimInfoArray = boneAnimInfoArrayArray[a];
							auto boneAnimInfo = pBoneAnimInfoArray->at(b);
							const size_t frameCount = boneAnimInfo->GetGlobalFrameAttitudeMatrices().size();

							for (size_t f = 0; f < frameCount; ++f)
							{
								const auto& globalFrameMatrix = boneAnimInfo->GetGlobalFrameAttitudeMatrices()[f];
								boneAnimInfo->AddLocalFrameAttitudeMatrix(globalFrameMatrix);
							}
						}
					}
					const auto& skeletonChildren = skeleton->GetChildren();
					for (auto child : skeletonChildren)
					{
						const int boneIndex = pModelMeshInfo->FindBoneIndex(child->GetSkeletonNameW());
						_ASSERTE(boneIndex >= 0);
						boneSkeletonInfo->AddChildBoneIndex(boneIndex);
					}
				}
				// ローカル初期姿勢を設定したもの（書き換えたもの）を再設定する。
				pModelMeshInfo->SetBoneInitialPoseInfoArray(boneInitialPoseInfoArray);
			}
		}
		return pModelMeshInfo;
	}

	typedef std::unordered_map<std::wstring, bool> TTextureFileNameToAlphaUsageTable;
	typedef std::unordered_map<std::wstring, MyTextureHelper::TTextureDataPackPtr> TTextureFileNameToDibTable;

#if 0
	CStringW UuidToCStringW(GUID guid)
	{
		RPC_WSTR temp = nullptr;
		const auto retCode = UuidToStringW(&guid, &temp);
		if (retCode == RPC_S_OK)
		{
			const CStringW outString = reinterpret_cast<const wchar_t*>(temp);
			::RpcStringFreeW(&temp); // TODO: 解放前に例外が発生したらアウトなので、RAII を書いたほうがよい。
			return outString;
		}
		else
		{
			_ASSERTE(false);
			return CStringW();
		}
	}
#pragma comment(lib, "Rpcrt4.lib")
#else
	CStringW UuidToCStringW(GUID guid)
	{
		// UuidToStringW() の結果と比較したが、大文字/小文字の違いを除いて、一応同じ結果になる。
		// エンディアンの異なる環境でも同じとなるかどうかは不明。

		CStringW str;
		str.Format(
			L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			guid.Data1,
			guid.Data2,
			guid.Data3,
			guid.Data4[0],
			guid.Data4[1],
			guid.Data4[2],
			guid.Data4[3],
			guid.Data4[4],
			guid.Data4[5],
			guid.Data4[6],
			guid.Data4[7]
			);
		return str;
	}
#endif

	HRESULT CreateTextureDibBuffers(LPCWSTR pTextureRootDirPath, const TTextureFileNameToAlphaUsageTable& texFileNameUsageTable, _Out_ TTextureFileNameToDibTable& textureDibTable)
	{
		// WIC を使ってテクスチャ画像をロードする。PNG, JPG, BMP などに対応。
		// 他の OS に移植する場合、libpng や libjpeg などを使う必要あり。
		// TODO: DDS のロードは DirectXTex を使う（ただし圧縮されていたりミップマップを含んでいたりする場合は単一 DIB として扱えない）。
		Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
		HRESULT hr = E_FAIL;
		hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
			reinterpret_cast<void**>(wicFactory.GetAddressOf()));
		if (FAILED(hr))
		{
			DXTRACE_ERR(_T("Failed to create an instance of IWICImagingFactory!!"), hr);
			return hr;
		}

		for (const auto& src : texFileNameUsageTable)
		{
			CPathW texFileFullPath(pTextureRootDirPath);
			texFileFullPath += src.first.c_str();
			ATLTRACE(L"Full path of the texture file = <%s>\n", texFileFullPath.m_strPath.GetString());
			if (!texFileFullPath.FileExists())
			{
				ATLTRACE(L"Specified texture file does not exist!!\n");
				continue;
			}
			Microsoft::WRL::ComPtr<IWICBitmapDecoder> wicDec;
#if 1
			//hr = wicFactory->CreateDecoderFromFilename(texFileFullPath.m_strPath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, wicDec.GetAddressOf());
			hr = wicFactory->CreateDecoderFromFilename(texFileFullPath.m_strPath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, wicDec.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create an instance of IWICBitmapDecoder!!"), hr);
				continue;
			}
#else
			Microsoft::WRL::ComPtr<IStream> stream;
			hr = ::SHCreateStreamOnFileW(texFileFullPath.m_strPath, STGM_READ, stream.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create an instance of IStream!!"), hr);
				continue;
			}

			hr = wicFactory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, wicDec.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create an instance of IWICBitmapDecoder!!"), hr);
				continue;
			}
#endif

			Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> wicFrame;
			// とりあえず最初のフレームのみ取得。
			hr = wicDec->GetFrame(0, wicFrame.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create an instance of IWICBitmapFrameDecode!!"), hr);
				continue;
			}

			UINT width = 0, height = 0;
			hr = wicFrame->GetSize(&width, &height);
			if (width == 0 || height == 0 || width > 4096 || height > 4096)
			{
				// HACK: テクスチャ画像サイズの上限はハードウェアに応じて決めたほうがよい。
				// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro
				ATLTRACE(L"Image size is not available!!\n");
				continue;
			}

			double resX = 0, resY = 0;
			hr = wicFrame->GetResolution(&resX, &resY);

			Microsoft::WRL::ComPtr<IWICBitmapSource> wicBmpSrc;
			hr = wicFrame.As(&wicBmpSrc);
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to get an interface of IWICBitmapSource!!"), hr);
				continue;
			}

			auto fmt = WICPixelFormatGUID();
			bool isGray = false;
			hr = wicBmpSrc->GetPixelFormat(&fmt);
			ATLTRACE(L"Pixel format GUID = <%s>\n", UuidToCStringW(fmt).GetString());

			UINT stride = 0;
			// GUID は構造体なので switch-case は使えないが、C++ では直接の比較演算が可能なようにグローバル演算子オーバーロードが定義されているらしい。
			if (fmt == GUID_WICPixelFormatBlackWhite)
			{
				//stride = (w + 7) ~7;
				stride = MyMath::CalcStrideInBytes(width, 1);
				// OpenGL にはモノクロ 1bit やグレースケール 8bit のテクスチャ フォーマットが存在しない？
				// これらのフォーマットは、アルファ マップ用途には使えるので、32bit カラー化はしない。
				// なお、アルファ マップ（DXGI_FORMAT_A8_UNORM, GL_ALPHA）として使うか、
				// それともグレースケールのカラーマップ（DXGI_FORMAT_R8_UNORM, GL_RED）として使うかはマテリアル特性に依存する。
				// シェーダーコードでどの成分を使用するか、にも影響する。
				// ただ、グレースケールの画像を使ってカラーマップを作ったとしても、確かにメモリー節約にはなるが、
				// それをグレースケールとして扱うためにはどのみち専用シェーダーを書かなければならない。
				isGray = true;
			}
			else if (fmt == GUID_WICPixelFormat1bppIndexed)
			{
				stride = MyMath::CalcStrideInBytes(width, 1);
			}
			else if (fmt == GUID_WICPixelFormat8bppIndexed)
			{
				stride = MyMath::CalcStrideInBytes(width, 8);
			}
			else if (fmt == GUID_WICPixelFormat8bppGray)
			{
				stride = MyMath::CalcStrideInBytes(width, 8);
				isGray = true;
			}
			else if (fmt == GUID_WICPixelFormat8bppAlpha)
			{
				stride = MyMath::CalcStrideInBytes(width, 8);
				isGray = true;
			}
			else if (
				fmt == GUID_WICPixelFormat24bppBGR ||
				fmt == GUID_WICPixelFormat24bppRGB)
			{
				stride = MyMath::CalcStrideInBytes(width, 24);
			}
			else if (
				fmt == GUID_WICPixelFormat32bppBGRA ||
				fmt == GUID_WICPixelFormat32bppRGBA ||
				fmt == GUID_WICPixelFormat32bppPBGRA)
			{
				stride = MyMath::CalcStrideInBytes(width, 32);
				_ASSERTE(stride == width * 4);
			}
			else
			{
				// HDR フォーマットや圧縮フォーマットも非対応。
				ATLTRACE(L"Unsupported pixel format!!\n");
				continue;
			}
			// Direct2D では PBGRA に変換した後 ID2D1RenderTarget::CreateBitmapFromWicBitmap() を使うことで
			// ビットマップ（テクスチャ）を直接生成できる。
			// PC 向けの Direct3D, OpenGL は GDI や Direct2D との直接連携を無視するならば RGBA も BGRA もどちらでも OK だが、
			// OpenGL ES でも使用可能なピクセル フォーマットには制約があるので、
			// WIC コンバーターを使ってここでフォーマット変換しておいたほうがよい。
			// OpenGL ES 2.0 では GL_RGBA と GL_RED/GL_GREEN/GL_BLUE/GL_ALPHA のみで、GL_BGRA が使えない？
			// iPhone 用の OpenGL ES 2.0 では使えるが Android では使えない拡張らしい。
			// GL_BGRA 自体がもともと本家 OpenGL でも最初は拡張機能扱いだったらしく、OpenGL 3.2 で標準化されたらしい。
			// Direct3D では D3DFMT_A8B8G8R8（DXGI_FORMAT_R8G8B8A8_UNORM 相当）と D3DFMT_A8R8G8B8（DXGI_FORMAT_B8G8R8A8_UNORM 相当）が
			// D3D 9 の時代からすでにサポートされていた。
#if 0
			Microsoft::WRL::ComPtr<IWICFormatConverter> wicConverter;
			hr = wicFactory->CreateFormatConverter(wicConverter.GetAddressOf());
			hr = wicConverter->Initialize(wicFrame.Get(), GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut);
#endif

			// 必要に応じてコンバートする。
			// 乗算済みアルファはコンバートしてしまう。
			if (fmt != GUID_WICPixelFormat32bppRGBA &&
				//fmt != GUID_WICPixelFormat32bppBGRA &&
				fmt != GUID_WICPixelFormat8bppGray &&
				fmt != GUID_WICPixelFormat8bppAlpha)
			{
				ATLTRACE("Now converting pixel format...\n");
				Microsoft::WRL::ComPtr<IWICFormatConverter> wicConverter;
				hr = wicFactory->CreateFormatConverter(wicConverter.GetAddressOf());
				if (FAILED(hr))
				{
					DXTRACE_ERR(_T("Failed to create an interface of IWICFormatConverter!!"), hr);
					continue;
				}
				// 8bit グレー or 32bit カラーの一択。
				hr = wicConverter->Initialize(wicFrame.Get(),
					isGray ? GUID_WICPixelFormat8bppGray : GUID_WICPixelFormat32bppRGBA,
					WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut);
				if (FAILED(hr))
				{
					DXTRACE_ERR(_T("Failed to convert pixel format!!"), hr);
					continue;
				}
				// 新しいもので置き換える。IWICFormatConverter は IWICBitmapSource を継承しているらしい。
				wicBmpSrc = wicConverter;
				stride = MyMath::CalcStrideInBytes(width, isGray ? 8 : 32);
			}

			const UINT bufSizeInBytes = stride * height;
			std::vector<uint8_t> dibBufTemp(bufSizeInBytes);
			const WICRect imgRect = { 0, 0, int(width), int(height) };
			hr = wicBmpSrc->CopyPixels(&imgRect, stride, bufSizeInBytes, &dibBufTemp[0]);
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to get pixels!!"), hr);
				continue;
			}
			MyTextureHelper::TTextureDataPackPtr texData(new MyTextureHelper::TextureDataPack());
			texData->TextureDib.swap(dibBufTemp);
			texData->TextureWidth = width;
			texData->TextureHeight = height;
			texData->TextureColorDepthInBits = isGray ? 8 : 32;
			texData->IsForAlpha = src.second;
			textureDibTable[src.first] = texData;
		}

		return hr;
	}

	void CreateTextureTable(ID3D11Device* pD3DDevice,
		const TTextureFileNameToDibTable& textureDibTable, _Out_ MyD3D::TMyFileNameToTexture2DTable& outTable)
	{
		_ASSERTE(pD3DDevice != nullptr);
		HRESULT hr = E_FAIL;
		for (const auto& src : textureDibTable)
		{
			const auto& texData = src.second;
			_ASSERTE(!texData->TextureDib.empty());

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = texData->TextureWidth;
			texDesc.Height = texData->TextureHeight;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 1;
			// 32bit カラー マップ or 8bit アルファ マップを作る。
			if (texData->TextureColorDepthInBits == 32)
			{
				texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				//texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				// 現行の OpenGL ES の制約に合わせて BGRA でなく RGBA にしておく。
			}
			else
			{
				texDesc.Format = DXGI_FORMAT_A8_UNORM;
			}
			texDesc.Usage = D3D11_USAGE_IMMUTABLE; // 作成時に内容も初期化する。
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.SampleDesc.Count = 1;

			const uint32_t stride = texData->GetStrideInBytes();
			D3D11_SUBRESOURCE_DATA subresData = {};
			subresData.pSysMem = &texData->TextureDib[0];
			subresData.SysMemPitch = stride;

			// テクスチャと一緒にシェーダーリソース ビューも同時に作成しておいたほうがよい。
			MyD3D::MyTexture2DSRVPack pack;
			hr = pD3DDevice->CreateTexture2D(&texDesc, &subresData, pack.Texture2D.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create Texture2D!!"), hr);
				continue;
			}
			hr = pD3DDevice->CreateShaderResourceView(pack.Texture2D.Get(), nullptr, pack.TextureSRV.GetAddressOf());
			if (FAILED(hr))
			{
				DXTRACE_ERR(_T("Failed to create shader resource view!!"), hr);
				continue;
			}

			outTable[src.first] = pack;
		}
	}

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
	void CreateTextureTable(
		const TTextureFileNameToDibTable& textureDibTable, _Out_ MyOGL::TMyFileNameToTexture2DTable& outTable)
	{
		for (const auto& src : textureDibTable)
		{
			const auto& texData = src.second;
			_ASSERTE(!texData->TextureDib.empty());

			// HACK: テクスチャ サイズ上限のチェック。ハードウェア能力にも依存する。Direct3D 9/10/11 と同じ仕様とは限らない。

			auto outTexture = MyOGL::Factory::CreateOneTexturePtr();
			_ASSERTE(outTexture.get() != 0);
			if (outTexture.get() == 0)
			{
				continue;
			}

			GLenum lastError = 0;
			glBindTexture(GL_TEXTURE_2D, outTexture.get());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#if 0
			// デフォルトでバイリニア フィルタをバインドしておく。
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
			// 32bit カラー マップ or 8bit アルファ マップを作る。
			const int inoutPixelFormat = (texData->TextureColorDepthInBits == 32) ? GL_RGBA : GL_ALPHA;
			//const int inoutPixelFormat = (texData->TextureColorDepthInBits == 32) ? GL_BGRA : GL_ALPHA;
			// 現行の OpenGL ES の制約に合わせて BGRA でなく RGBA にしておく。
			glTexImage2D(GL_TEXTURE_2D, 0, inoutPixelFormat,
				texData->TextureWidth,
				texData->TextureHeight,
				0, inoutPixelFormat, GL_UNSIGNED_BYTE, &texData->TextureDib[0]);
			lastError = glGetError();
			_ASSERTE(lastError == GL_NO_ERROR);
			// 一応メモリー不足に陥ったりしてないかチェック。

			glBindTexture(GL_TEXTURE_2D, 0);

			outTable[src.first].swap(outTexture);
		}
	}
#endif
}


namespace MyFbxViewer
{
#if 0
	// エッジの頂点インデックスを管理する。
	template<typename T> class TEdge
	{
	public:
		T IndexA;
		T IndexB;

		TEdge()
			: IndexA(), IndexB()
		{}
		explicit TEdge(const T& iA, const T& iB)
			: IndexA(iA), IndexB(iB)
		{}

		bool HasSharedEdgeWith(const TEdge<T>& other) const
		{
			return
				(this->IndexA == other.IndexA && this->IndexB == other.IndexB) ||
				(this->IndexA == other.IndexB && this->IndexB == other.IndexA);
		}

		bool operator ==(const TEdge<T>& other) const
		{
			return this->HasSharedEdgeWith(other);
		}
		bool operator !=(const TEdge<T>& other) const
		{
			return !this->HasSharedEdgeWith(other);
		}
		size_t GetHashCode() const
		{
			return std::hash<T>()(this->IndexA) ^ std::hash<T>()(this->IndexB);
		}

		struct HashFunctor
		{
			size_t operator ()(const TEdge<T>& v) const { return v.GetHashCode(); }
		};
	};
#endif

	// 三角形のエッジ（辺）のひとつとその対角の頂点インデックスを管理する。
	// インデックス（辺・対角点）は補助情報で、同値性に関与しない。
	template<typename TIndex> class TTriEdge
	{
	public:
		MyMath::Vector3F PositionA;
		MyMath::Vector3F PositionB;
		TIndex IndexA;
		TIndex IndexB;
		TIndex Opposite;

		TTriEdge()
			: PositionA(MyMath::ZERO_VECTOR3F), PositionB(MyMath::ZERO_VECTOR3F), IndexA(), IndexB(), Opposite()
		{}
#if 0
		explicit TTriEdge(const TIndex& iA, const TIndex& iB, const TIndex& op)
			: PositionA(MyMath::ZERO_VECTOR3F), PositionB(MyMath::ZERO_VECTOR3F), IndexA(iA), IndexB(iB), Opposite(op)
		{}
#endif
		explicit TTriEdge(const MyMath::Vector3F& posA, const MyMath::Vector3F& posB, const TIndex& iA, const TIndex& iB, const TIndex& op)
			: PositionA(posA), PositionB(posB), IndexA(iA), IndexB(iB), Opposite(op)
		{}

		bool HasSharedEdgeWith(const TTriEdge<TIndex>& other) const
		{
#if 0
			return
				(this->IndexA == other.IndexA && this->IndexB == other.IndexB) ||
				(this->IndexA == other.IndexB && this->IndexB == other.IndexA);
#elif 0
			return
				(MyMath::IsEqualVector3(this->PositionA, other.PositionA) && MyMath::IsEqualVector3(this->PositionB, other.PositionB)) ||
				(MyMath::IsEqualVector3(this->PositionA, other.PositionB) && MyMath::IsEqualVector3(this->PositionB, other.PositionA));
#else
			// HACK: 両面ポリゴンに対応できていない。
			// 両面ポリゴンを複数接続したときに、余計な輪郭線が引かれる。
			// 背を向けて不可視になっているポリゴンとの接続辺に線が引かれるのはある意味正しい挙動ではあるが、
			// その前面に表向きの接続面があっても線が引かれるのはいただけない。
			// やはり両面ポリゴンには輪郭線描画しないのが吉かもしれない。
			return
				MyMath::IsEqualVector3(this->PositionA, other.PositionB) && MyMath::IsEqualVector3(this->PositionB, other.PositionA);
#endif
		}

		bool operator ==(const TTriEdge<TIndex>& other) const
		{
			return this->HasSharedEdgeWith(other);
		}
		bool operator !=(const TTriEdge<TIndex>& other) const
		{
			return !(*this == other);
		}
		size_t GetHashCode() const
		{
			//return this->IndexA ^ this->IndexB;
			//return std::hash<TIndex>()(this->IndexA) ^ std::hash<TIndex>()(this->IndexB);
			return
				std::hash<float>()(this->PositionA.x) ^
				std::hash<float>()(this->PositionA.y) ^
				std::hash<float>()(this->PositionA.z) ^
				std::hash<float>()(this->PositionB.x) ^
				std::hash<float>()(this->PositionB.y) ^
				std::hash<float>()(this->PositionB.z);
		}

		struct HashFunctor
		{
			size_t operator ()(const TTriEdge<TIndex>& v) const { return v.GetHashCode(); }
		};
	};

	// 三角形の頂点位置・頂点インデックスと隣接インデックスを管理する。
	template<typename TIndex> class TTriFace
	{
	public:
		MyMath::Vector3F Positions[3];
		TIndex Indices[3];
		TIndex AdjacentIndices[3];

		TTriFace()
			: Indices()
			, AdjacentIndices()
		{
			std::fill_n(this->Positions, ARRAYSIZE(this->Positions), MyMath::ZERO_VECTOR3F);
		}

		TTriFace(const MyMath::Vector3F& v0, const MyMath::Vector3F& v1, const MyMath::Vector3F& v2, const TIndex& i0, const TIndex& i1, const TIndex& i2)
			: Indices()
			, AdjacentIndices()
		{
			this->Positions[0] = v0;
			this->Positions[1] = v1;
			this->Positions[2] = v2;
			Indices[0] = i0;
			Indices[1] = i1;
			Indices[2] = i2;
			// デフォルトは反対側（対角）の頂点（辺が共有されていない状態）。
			AdjacentIndices[0] = i2; // 0, 1 の Opposite。
			AdjacentIndices[1] = i0; // 1, 2 の Opposite。
			AdjacentIndices[2] = i1; // 2, 0 の Opposite。
		}

#if 0
		TEdge<TIndex> GetEdge(uint32_t start) const
		{
			return TEdge<TIndex>(this->Indices[start % 3], this->Indices[(start + 1) % 3]);
		}
#endif

		TTriEdge<TIndex> GetTriEdge(uint32_t start) const
		{
			return TTriEdge<TIndex>(
				this->Positions[start % 3], this->Positions[(start + 1) % 3],
				this->Indices[start % 3], this->Indices[(start + 1) % 3], this->Indices[(start + 2) % 3]);
		}

		void SetAdjacentIndex(uint32_t start, const TTriEdge<TIndex>& sharedEdge)
		{
			this->AdjacentIndices[start % 3] = sharedEdge.Opposite;
		}
	};

	// GL_TRIANGLES_ADJACENCY および D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ 用の隣接インデックス データを作成する。
	// 6頂点を1単位とする。
	template<typename TIndex, class TVertexPosProvider> void CreateTriangleListAdjacencyIndices(
		const std::vector<TIndex>& inIndexArray, std::vector<TIndex>& outIndexArray,
		TVertexPosProvider vposProvider)
	{
		ATLTRACE(__FUNCTIONW__ L"(): Now creating adjacency info...\n");
		MyUtils::HRStopwatch stopwatch;
		stopwatch.Start();

		// http://prideout.net/blog/?p=54
		// http://ogldev.atspace.co.uk/www/tutorial39/tutorial39.html
		// http://www.flipcode.com/archives/The_Half-Edge_Data_Structure.shtml

		// 頂点インデックス配列は三角形面のみという前提。
		_ASSERTE(inIndexArray.size() % 3 == 0);
		outIndexArray.clear();
		typedef TTriFace<TIndex> TriFace;
		typedef TTriEdge<TIndex> TriEdge;
		//typedef TEdge<TIndex> Edge;
		std::vector<TriFace> faces;
		// エッジをキーにして面インデックスをグループ化する。
		// ちなみにハッシュ関数や同値判定関数としてラムダ式を直接渡すことはできない。
		// 関数オブジェクトおよび比較演算子オーバーロードを定義する。
#if 0
		// ボツ案。

		class TVertexPosComparer;
		TVertexPosComparer vposComparer;
		auto vposComparer = [&tempVB](uint16_t ia, uint16_t ib) { return MyMath::IsEqualVector3(tempVB[ia].Position, tempVB[ib].Position); };
		class TriEdgeEqualityFunctor
		{
		private:
			TVertexPosComparer Comparer;
		public:
			TriEdgeEqualityFunctor(TVertexPosComparer comparer)
				: Comparer(comparer)
			{}
			bool operator ()(const TriEdge& x, const TriEdge& y) const
			{
				// HACK: これでは NG。比較関数だけ頂点位置の重複を考慮していても、ハッシュ値が異なるので同じグループにはならない。
				return (x == y) ||
					(this->Comparer(x.IndexA, y.IndexA) && this->Comparer(x.IndexB, y.IndexB)) ||
					(this->Comparer(x.IndexA, y.IndexB) && this->Comparer(x.IndexB, y.IndexA));
			}
		};
		// Visual C++ ではデフォルトのバケット数は8らしい。
		const auto defaultBucketCount = std::unordered_multimap<TriEdge, size_t, TriEdge::HashFunctor>().bucket_count();
		typedef std::unordered_multimap<TriEdge, size_t, TriEdge::HashFunctor, TriEdgeEqualityFunctor> TMap;
		TMap edgeToFaceIndexMap(defaultBucketCount,
			TMap::hasher(), TMap::key_equal(vposComparer));
#else
		std::unordered_multimap<TriEdge, size_t, TriEdge::HashFunctor> edgeToFaceIndexMap;
#endif
		for (size_t i = 0; i < inIndexArray.size(); i += 3)
		{
			// このタイミングで三角形のエッジ頂点セット（キー）と面インデックス（値）とのテーブルを作成する。
			// 頂点をどの面と共有しているのかが分かる。
			// 頂点インデックスだけの登録だと、位置を共有しているかどうかは分からない。
			// （前段の処理で、位置は同じでも法線や UV の異なる頂点はインデックスが分離されてしまっている）

			const TIndex vertIndex0 = inIndexArray[i + 0];
			const TIndex vertIndex1 = inIndexArray[i + 1];
			const TIndex vertIndex2 = inIndexArray[i + 2];
			const size_t faceIndex = i / 3;
			faces.push_back(TriFace(vposProvider(vertIndex0), vposProvider(vertIndex1), vposProvider(vertIndex2), vertIndex0, vertIndex1, vertIndex2));
			edgeToFaceIndexMap.insert(std::make_pair(TriEdge(vposProvider(vertIndex0), vposProvider(vertIndex1), vertIndex0, vertIndex1, vertIndex2), faceIndex));
			edgeToFaceIndexMap.insert(std::make_pair(TriEdge(vposProvider(vertIndex1), vposProvider(vertIndex2), vertIndex1, vertIndex2, vertIndex0), faceIndex));
			edgeToFaceIndexMap.insert(std::make_pair(TriEdge(vposProvider(vertIndex2), vposProvider(vertIndex0), vertIndex2, vertIndex0, vertIndex1), faceIndex));
		}
		// 単純に共有辺のチェックをする場合、2つの面の組み合わせを調べるため、m_P_2 ではなく m_C_2 となる。
		// O(n^2) なのでインデックス数が10000くらいになるとすさまじく時間がかかるようになる。
		// map もしくは unordered_map などを使って計算量を低減する。
		const size_t faceCount = faces.size();
		for (size_t i = 0; i < faceCount; ++i)
		{
#if 1
			auto& facei = faces[i];
			for (uint32_t k = 0; k < 3; ++k)
			{
				const auto edgeA = facei.GetTriEdge(k);
				const auto othersRange = edgeToFaceIndexMap.equal_range(edgeA);
				// first が end() であるか否かを調べる必要はない。pair をループの初期化と継続条件にそのまま使えばよい。
				for (auto it = othersRange.first; it != othersRange.second; ++it)
				{
					_ASSERTE(it->second < faceCount);
					const auto edgeB = it->first;
					// 自分自身は除外するため、辺は一致しても対角が一致しないことを調べる。
					if (edgeA.Opposite != edgeB.Opposite)
					{
						// 隣接情報を登録。
						facei.SetAdjacentIndex(k, edgeB);
					}
				}
			}
#else
			// ボツ案。

			// ポインタをキャッシュして高速化する。
			auto* pFaces = &faces[0];

			// 最初の i = 0 ループで 0 番目の面との組み合わせチェックはすべて終わるので、j は 1 からでよい。
			for (size_t j = i + 1; j < faceCount; ++j)
			{
#if 0
				if (i == j)
				{
					continue;
				}
#endif
				// i 面の k 番目の辺を j 面の l 番目の辺と共有しているかどうか調べる。
				for (uint32_t k = 0; k < 3; ++k)
				{
					const auto edgeA = pFaces[i].GetTriEdge(k);
					for (uint32_t l = 0; l < 3; ++l)
					{
						const auto edgeB = pFaces[j].GetTriEdge(l);
						if (edgeA.HasSharedEdgeWith(edgeB))
						{
							// お互いの隣接情報を登録。
							pFaces[i].SetAdjacentIndex(k, edgeB);
							pFaces[j].SetAdjacentIndex(l, edgeA);
							goto GotoLabel_NextFace;
						}
						// NOTE: 頂点自体（インデックス）を共有していない場合でも、まったく同じ位置座標にある辺同士は隣接しているとみなす。
						// 前処理としてレンダリング頂点バッファ用に法線あるいは UV が異なる頂点を分離しているので、
						// インデックス バッファだけ使って検索した共有辺群では意図しない位置に輪郭線が引かれてしまう。
						// 単純な球でも UV をアトラス展開している場合は分離対象になる。
						if (
							(vposComparer(edgeA.IndexA, edgeB.IndexA) && vposComparer(edgeA.IndexB, edgeB.IndexB)) ||
							(vposComparer(edgeA.IndexA, edgeB.IndexB) && vposComparer(edgeA.IndexB, edgeB.IndexA))
							)
						{
							pFaces[i].SetAdjacentIndex(k, edgeB);
							pFaces[j].SetAdjacentIndex(l, edgeA);
							goto GotoLabel_NextFace;
						}
					}
				}
				// HACK: 「i と j は隣接する（もしくは隣接しない）」という結果を保存しておけば、
				// 次に j と i の隣接を調べるときにスキップできる。
				// ただしそのために map や unordered_map を使うと今度はメモリの空間オーバーヘッドが発生するので注意。
				// 最初に面の組み合わせ（面インデックス ペア）の配列を作成して、それを走査する形にするのが最速かつ省メモリかも？
				// 意外に総当たりのほうが速かったりする。
				// この処理自体にリアルタイム性は必要ないが、冗長な方法だとロード時間は伸びてしまう。
				// 特にデバッグ ビルドでの重いループは速度低下が顕著。

GotoLabel_NextFace:
				continue;
			}
#endif
		}
		for (const auto& f : faces)
		{
			// (0 + 6n), (2 + 6n), (4 + 6n) が中央の三角形の頂点。
			// (1 + 6n), (3 + 6n), (5 + 6n) が隣接三角形の頂点になる。
			for (int i = 0; i < 3; ++i)
			{
				outIndexArray.push_back(f.Indices[i]);
				outIndexArray.push_back(f.AdjacentIndices[i]);
			}
		}
		_ASSERTE(outIndexArray.size() % 6 == 0);
		_ASSERTE(inIndexArray.size() * 2 == outIndexArray.size());

		stopwatch.Stop();
		ATLTRACE(__FUNCTIONW__ L"(): Finished to create adjacency info.\n");
		ATLTRACE(__FUNCTIONW__ L"(): Elapsed time = %I64d[ms]\n", stopwatch.GetElapsedTimeInMilliseconds());
	}


	//! @brief  FBX ツリーからスキン メッシュを作成する。<br>
	//! 
	//! Direct3D/OpenGL デバイス依存情報とデバイス非依存情報を分けて生成するが、デバイス ロスト時の再構築は考慮しない。<br>
	//! D3D 10 以降であればデバイス ロストはほとんど発生しないので、バッファやテクスチャなどのデバイス依存リソースの再生成は（ほとんど）不要のはず。<br>
	//! 万が一デバイス ロストが発生したら強制終了、でも良い。<br>
	HRESULT CreateMySkinMeshFromFbx(
		_In_ const MyFbx::MyFbxNodeAnalyzerBase& nodeAnalyzer, LPCWSTR pTextureRootDirPath, ID3D11Device* pD3DDevice,
		_Out_ MyD3D::TMyModelMeshPtrsArray& d3dMeshArray,
		_Out_ MyD3D::TMyFileNameToTexture2DTable& d3dTexTable,
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		_Out_ MyOGL::TMyModelMeshPtrsArray& oglMeshArray,
		_Out_ MyOGL::TMyFileNameToTexture2DTable& oglTexTable,
#endif
		_Out_ MyMath::TMyNameToMaterialTable& materialNameTable,
		_Out_ MyCommon::TMyModelMeshDetailInfoPtrsArray& modelMeshInfoArray,
		_Out_ MyCommon::MyAnimTrackInfoTable& animTrackInfoTable
		)
	{
		if (!d3dMeshArray.empty())
		{
			return E_INVALIDARG;
		}

		if (!d3dTexTable.empty())
		{
			return E_INVALIDARG;
		}

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
		if (!oglMeshArray.empty())
		{
			return E_INVALIDARG;
		}

		if (!oglTexTable.empty())
		{
			return E_INVALIDARG;
		}
#endif

		if (!materialNameTable.empty())
		{
			return E_INVALIDARG;
		}

		if (!modelMeshInfoArray.empty())
		{
			return E_INVALIDARG;
		}

		if (animTrackInfoTable.GetAnimCount() != 0)
		{
			return E_INVALIDARG;
		}

		if (nodeAnalyzer.GetMeshAnalyzerArray().empty())
		{
			// HACK: FBX Converter 付属の Bird_Leg.fbx はココで引っかかる。Mesh ではなく Patch らしい。
			// なお、FBX Converter 付属の LocalMotionBlend.fbx にはジャグリングのボールとして NURBS 球が含まれている。
			ATLTRACE("No mesh detected!!\n");
			return S_FALSE;
		}

		// HACK: メッシュはなくても Patch/NURBS＋アニメーションが存在するということはありえる。
		animTrackInfoTable.SetAnimTrackNamesArray(nodeAnalyzer.GetAnimTimeInfo().GetAnimStackNamesArray());

		TTextureFileNameToAlphaUsageTable texFileNameUsageTable;

		// HACK: メッシュやテクスチャのファイル読込は非常に重くなることが予想される。サブスレッドで読込を行なうべき。
		// なお、Direct3D は 9 の頃からすでにマルチスレッド対応モードが一応存在していて、
		// また D3D 10/11 ではリソース生成に関してはデフォルトでスレッドセーフであるため、
		// サブスレッドでリソース生成することには何の問題もないが、OpenGL のほうは最悪。
		// OpenGL 3.2 以降であれば、サブスレッドでリソースを生成するというのは不可能ではないらしいが、非常に手間がかかる。
		// https://web.archive.org/web/20161019114519/http://sa-zero.blog.eonet.jp/switchcase/2013/10/multi-thread.html
		// Windows では、おそらく wglShareLists() を使えば、OpenGL 1.x でもサブスレッドでリソースを生成することはできるはず。
		// サブスレッドでフレームバッファにオフスクリーン描画できるかどうかは不明。
		// https://www.khronos.org/opengl/wiki/OpenGL_and_multithreading
		// glXCreateContext() に相当する拡張として wglCreateContextAttribsARB() という関数もある。
		// wglCreateContextAttribsARB() は、Windows で OpenGL 3.x 以降の機能を利用する GL コンテキストを作成する際にも使用する。
		// https://stackoverflow.com/questions/55885139/what-is-shareable-between-opengl-contexts-and-how-to-enable-sharing
		// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
		// しかし、これらは標準的かつクロスプラットフォームに利用できるものではない。
		// OpenGL のスレッドセーフ性、マルチスレッド レンダリングは現行規格では期待できない。次世代規格が待たれる。
		// http://jp.techcrunch.com/2014/08/12/20140811khronos-group-starts-working-on-the-next-generation-of-its-opengl-3d-specs/
		// http://msdn.microsoft.com/en-us/library/ff476890.aspx
		// Windows ストア アプリの場合、UI スレッドでファイル I/O を行なうことはそもそもできない（50ミリ秒以上 UI スレッドを停止させるアプリはリジェクトされるらしい？）し、
		// ファイル I/O よりは比較的高速なリソース生成でさえも UI スレッドで行なうのは賢明ではないため、
		// ごっそりサブスレッドに委譲するべき。サブスレッドの起動は PPL を使うとよい。
		// https://www.infoq.com/jp/news/2013/02/ants-profiler-8-beta/
		// https://www.infoq.com/jp/articles/Async-API-Design/

		for (MyFbx::MyFbxMeshAnalyzer::TConstSharedPtr pMeshAnalyzer : nodeAnalyzer.GetMeshAnalyzerArray())
		{
			ATLTRACE(__FUNCTIONW__ L"(), Mesh=\"%s\"\n", pMeshAnalyzer->GetMeshNameW().c_str());

			MyVertexTypes::TMySkinVertexArray tempVB;
			std::vector<uint16_t> tempIB;
			MyMath::TMyAttributeRangeArray tempAttrRangeArray;
			MyMath::TMyMaterialPtrsArray tempMaterials;
			TIntArray tempMaterialIndicesForAttrTable;

			// 面をすべて三角形面で構成するように更新する。
			//pMeshAnalyzer->ConvertAllBufferAsTrianglesOnly();

			if (!pMeshAnalyzer->CreateTriangleOnlySkinMeshSourceData(
				tempVB, tempIB, tempAttrRangeArray, tempMaterials, tempMaterialIndicesForAttrTable))
			{
				continue;
			}

			// TODO: 隣接インデックスを使ってジオメトリ シェーダーで処理した結果、正常にレンダリングできるかどうかテストする。
			// 頂点データ構造を抽象化するため、テンプレートとラムダ式を使って getter を渡す。
			decltype(tempIB) adjIndices;
			CreateTriangleListAdjacencyIndices(tempIB, adjIndices,
				[&tempVB](uint16_t index) { return tempVB[index].Position; });

			for (auto& mat : tempMaterials)
			{
				// ダブっていると思われるマテリアルはインスタンスを一本化する。
				// 判定はマテリアルの名前の比較で行なう。
				// マテリアル名の大文字・小文字は区別する。
				// NOTE: Metasequoia はオブジェクトやマテリアルの大文字・小文字を区別しない。LightWave は区別する。
				const auto tableIt = materialNameTable.find(mat->GetMaterialName());
				if (tableIt != materialNameTable.end())
				{
					// すでに存在する。テーブルに登録済みのインスタンスで置き換える。
					mat = tableIt->second;
				}
				else
				{
					// 新規登録。
					materialNameTable[mat->GetMaterialName()] = mat;
				}
			}

			// 継承ではなくポインタのコンポジションにして、
			// Direct3D と OpenGL とでデバイス非依存の情報を共有できるようにする。
			auto pModelMeshInfo = CreateModelMeshInfo(
				pMeshAnalyzer.get(), nodeAnalyzer.GetSkeletonAnalyzer().get(),
				tempMaterials, tempMaterialIndicesForAttrTable);
			{
				const size_t meshBoneCount = pModelMeshInfo->GetBoneCount();
				if (meshBoneCount > MyCpuGpuCommon::MyBoneMatrixPalettePack::MAX_ANIM_BONE_NUM)
				{
					// DirectX 10 世代以降では CBuffer にかなり余裕があるが、転送効率のこともあるので、とりあえずあまり大きくない上限を設けておく。
					ATLTRACE(__FUNCTION__ "(), BoneCount = %Iu (Over CBuffer Capacity)\n", meshBoneCount);
				}
				// TODO: テクスチャ ファイルを読み込んでテクスチャ オブジェクトを作成し、
				// テクスチャ ファイル名をキーとするテクスチャ テーブルを作成する。
				// ファイル パスの相対パス化は FBX 解析時に行なわれていることが前提。
				// FBX テクスチャ ファイル名が絶対パスの場合、"<Filebody>.<Extension>" のみ取り出し、
				// テクスチャ ファイルはコンテンツ ディレクトリ（通例 .FBX ファイルと同階層）にあるものとみなす。
				// なお、制限なしの相対パスや絶対パス（".." を含む）だと、ダブることがありえるので注意。正規化が必要。
				// テーブルの検索キーとして使う場合、確実さを求めるならば絶対パスのほうがよい（複数の .FBX ファイルで、同じ名前だが内容の異なるテクスチャ ファイルを使える）。
				// 検索速度や可搬性を求めるならば短い相対パスのほうがよいはず。
				// ゲームなどで実際にコンテンツとして使用する独自バイナリ形式に変換する際は、当然相対パスでないとダメ。
				// ちなみに XNA はコンテント パイプラインを通したとき、
				// ソースとなるファイル名の Filebody がバッティングしても .xnb ファイルの名前を自動的に生成して一意になるようにしている。
				const auto matCount = pModelMeshInfo->GetMaterialsArray().size();
				for (size_t m = 0; m < matCount; ++m)
				{
					// 1つのメッシュに複数のマテリアル（属性）が含まれていることもある。
					const auto texfnameDiffuse = pModelMeshInfo->GetMaterialsArray()[m]->GetTexFileNameDiffuseMap();
					ATLTRACE(L"TexFileNameDiffuse[%Iu] = \"%s\"\n", m, texfnameDiffuse.c_str());
					if (!texfnameDiffuse.empty())
					{
						// 一時登録。多重登録はしない。
						texFileNameUsageTable[texfnameDiffuse] = false; // カラーマップ用途。アルファ マップ用途ではない。
					}
					// TODO: 法線マップの読み込み。アルファ マップよりむしろ重要。
				}
				// Direct3D/OpenGL それぞれにテクスチャのオブジェクトが必要だが、
				// テクスチャ ファイルの読み込みとカラーバッファの生成自体は1回で済むよう、
				// また複数のメッシュ パーツによって参照されている同じテクスチャを多重生成してしまわないよう、
				// メッシュ作成ループの終了後にテクスチャ テーブルを作成する。
				// DXT/BC6H/BC7 などの圧縮フォーマット（通例 .DDS ファイル）を使用する場合はミップマップや HDR テクスチャも生成できる。
				// 非 Windows 環境（OpenGL ES）用には、アプリケーションの読み込みコード側で別の拡張子が付けられた（圧縮形式の異なる）同名ファイルを
				// 読み込むようにするといいかも。テクスチャ圧縮の完全共通規格ができればいいのだが……
			}
			modelMeshInfoArray.push_back(pModelMeshInfo);

			// Direct3D 描画で使用するメッシュの作成。
			{
				auto pMesh = std::make_shared<MyD3D::MyModelMesh>();
				if (!pMesh->CreateMesh(pD3DDevice,
					&tempVB[0], tempVB.size(),
					&tempIB[0], tempIB.size(),
					adjIndices.empty() ? nullptr : &adjIndices[0], adjIndices.size()))
				{
					return E_FAIL;
				}
				else
				{
					pMesh->SetAttributeRangeArray(tempAttrRangeArray);
				}

				d3dMeshArray.push_back(pMesh);
			}

#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
			// OpenGL 描画で使用するメッシュの作成。
			{
				auto pMesh = std::make_shared<MyOGL::MyModelMesh>();
				if (!pMesh->CreateMesh(
					&tempVB[0], tempVB.size(),
					&tempIB[0], tempIB.size(),
					adjIndices.empty() ? nullptr : &adjIndices[0], adjIndices.size()))
				{
					return E_FAIL;
				}
				else
				{
					pMesh->SetAttributeRangeArray(tempAttrRangeArray);
				}

				oglMeshArray.push_back(pMesh);
			}
#endif
		}

		// TODO: WIC を使ってテクスチャ ファイルを読み込んでカラーバッファ DIB を生成し、その後 Direct3D/OpenGL テクスチャを作成する。
		TTextureFileNameToDibTable textureDibTable;
		CreateTextureDibBuffers(pTextureRootDirPath, texFileNameUsageTable, textureDibTable);
		CreateTextureTable(pD3DDevice, textureDibTable, d3dTexTable);
#if 0
		CreateTextureTable(textureDibTable, oglTexTable);
#endif

		ATLTRACE("TotalMaterialCount = %Iu\n", materialNameTable.size());

		return S_OK;
	}

}
