#pragma once

// Visual Studio 2012 での C++ DirectX ベースの Windows ストア アプリのプロジェクト テンプレートに付属するヘルパーコード。


// Win32 API が例外を処理できるようにするヘルパー ユーティリティ。
namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Win32 API エラーをキャッチするためのブレークポイントをこの行に設定します。
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			throw _com_error(hr);
#else
			throw Platform::Exception::CreateException(hr);
#endif
		}
	}
}
