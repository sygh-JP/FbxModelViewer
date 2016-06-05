#pragma once

// DirectXTex/DirectXTK 付属の DDSTextureLoader に実装されている、非公開だが有用なメソッドの間接呼び出しラッパーを公開する。


namespace MyDirectXTex
{

	extern void GetSurfaceInfo(
		_In_ size_t width,
		_In_ size_t height,
		_In_ DXGI_FORMAT fmt,
		_Out_opt_ size_t* outNumBytes,
		_Out_opt_ size_t* outRowBytes,
		_Out_opt_ size_t* outNumRows);

}
