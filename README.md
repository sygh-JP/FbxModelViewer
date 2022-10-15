﻿# FbxModelViewer
FBX model viewer by sygh.

This project includes my FBX viewer and some tools.
The viewer supports some of the FBX features to a limited extent such as skinning mesh and multiple animation.
Also it supports some types of shader effects such as shadow mapping, environment mapping, toon rendering, bloom, etc.

このプロジェクトには自前のFBXビューアーといくつかのツールが含まれます。
ビューアーはスキンメッシュやマルチアニメーションのようなFBXの機能のいくつかを限定的にサポートします。
またシャドウマッピング、環境マッピング、トゥーンレンダリング、ブルームのような数種類のシェーダー効果もサポートします。

## Development Environment
* Microsoft Visual Studio 2015 Update 3
* ATL/MFC 14.0
* .NET Framework 4.5.2 (WPF)
* DirectX 11.1 (Direct3D 11.1, Direct2D 1.1)
* [Autodesk FBX](https://www.autodesk.com/products/fbx/overview) SDK 2020.3.2
* [DirectXTK](https://github.com/microsoft/DirectXTK) 2016-04-26
* [DirectXTex](https://github.com/microsoft/DirectXTex) 2016-04-26
* [Compact Effects11](https://github.com/sygh-JP/CompactEffects11) (derived from the sample in DirectX SDK June 2010)

## Target Environment
* Windows 7 SP1 Platform Update/Windows 8.1/Windows 10 (Desktop)
* Graphics device compatible with Direct3D 11.0 (Shader Model 5.0, Feature Level 11_0) or higher
* Graphics driver compatible with Direct3D 11.1 or higher

## How to Build
1. Install **FBX SDK** and append the global include and library directory paths to it
1. Build **DirectXTK** (GitHub/CodePlex) and append the global include and library directory paths to it
1. Build **DirectXTex** (GitHub/CodePlex) and append the global include and library directory paths to it
1. Build **Compact Effects11** and append the global include and library directory paths to it
1. Copy "DDSTextureLoader.h" and "DDSTextureLoader.cpp" in DXTK/DXTex to the directory "SharedUtility/DDSTextureLoader"
1. Build "FbxModelViewer.sln"
1. Execute the F# script "FSharpUtilScripts/copy_dlls_for_exes.fsx"

You can append the global include and library directory paths above by editing "Microsoft.Cpp.Win32.user.props" or "Microsoft.Cpp.x64.user.props" in "%LocalAppData%\Microsoft\MSBuild\v4.0".

2022-10-16, sygh.
