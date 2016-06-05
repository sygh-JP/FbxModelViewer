#pragma once

// HACK: OpenGL は Shared Context を使わないと、サブスレッドでリソース生成・破棄できない。
// Direct3D/OpenGL 両対応するのは並大抵のことではなく、そういった作業はすでにミドルウェアの範疇になる。
// OpenGL がまともにマルチスレッド対応するか、後継の Vulkan がまともに使えるようになるまで、個人でのマルチ対応はあきらめたほうがいい。

#include "FbxNodeAnalyzerBase.h"
#include "MyD3DMesh.h"
#ifdef MY_FBX_ENABLES_OPENGL_SUPPORT
#include "MyOGLMesh.h"
#endif

namespace MyFbxViewer
{
	extern HRESULT CreateMySkinMeshFromFbx(
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
		);

} // end of namespace
