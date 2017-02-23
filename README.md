# FbxModelViewer
FBX model viewer by sygh.

This project includes my FBX viewer and some tools.
The viewer supports some of the FBX features to a limited extent such as skinning mesh and multiple animation.
Also it supports some types of shader effects such as shadow mapping, environment mapping, toon rendering, bloom, etc.

このプロジェクトには自前のFBXビューアーといくつかのツールが含まれます。
ビューアーはスキンメッシュやマルチアニメーションのようなFBXの機能のいくつかを限定的にサポートします。
またシャドウマッピング、環境マッピング、トゥーンレンダリング、ブルームのような数種類のシェーダー効果もサポートします。

## Development Environment (開発環境)
* Microsoft Visual Studio 2015 Community Update 3
* MFC 14.0
* .NET Framework 4.5.2 (WPF)
* DirectX 11.1 (Direct3D 11.1, Direct2D 1.1)
* Autodesk FBX SDK 2017.1
* Boost C++ 1.59.0
* DirectXTK 2016-04-26
* DirectXTex 2016-04-26

## Target Environment (ターゲット環境)
* Windows 7 SP1 Platform Update/Windows 8.1/Windows 10 (Desktop)
* Graphics hardware compatible with Direct3D 11.0 (Shader Model 5.0, Feature Level 11_0) or higher
* Graphics driver compatible with Direct3D 11.1 or higher

## How to Build (ビルド方法)
1. Install FBX SDK and append the global include and library directory paths to it
1. Install Boost C++ (not required to build) and append the global include and library directory paths to it
1. Build DirectXTK (GitHub/CodePlex) and append the global include and library directory paths to it
1. Build DirectXTex (GitHub/CodePlex) and append the global include and library directory paths to it
1. Build **Compact Effects11** (derived from the sample in DirectX SDK June 2010) and append the global include and library directory paths to it
1. Copy "DDSTextureLoader.h" and "DDSTextureLoader.cpp" in DXTK/DXTex to the directory "SharedUtility/DDSTextureLoader"
1. Build "FbxModelViewer.sln"
1. Execute the F# script "FSharpUtilScripts/copy_dlls_for_exes.fsx"

2017-02-24, sygh.
