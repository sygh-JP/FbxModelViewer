﻿open System

// ローカルの規定の場所に D3DCompiler や FBX SDK が展開されているものとして、
// アプリケーションの EXE バイナリが生成される場所に DLL およびデバッグ データベース ファイルをコピーする。
// *.dll, *.pdb ファイルは Git リポジトリには追加しない。

// [例]
// "C:\Program Files (x86)\Windows Kits\10\bin\x86\d3dcompiler_47.dll"
// "C:\Program Files (x86)\Windows Kits\10\bin\x64\d3dcompiler_47.dll"
// "C:\Program Files\Autodesk\FBX\FBX SDK\2017.0.1\lib\vs2015\x86\debug\libfbxsdk.dll"
// "C:\Program Files\Autodesk\FBX\FBX SDK\2017.0.1\lib\vs2015\x86\debug\libfbxsdk.pdb"
// "C:\Program Files\Autodesk\FBX\FBX SDK\2017.0.1\lib\vs2015\x86\release\libfbxsdk.dll"
// "C:\Program Files\Autodesk\FBX\FBX SDK\2017.0.1\lib\vs2015\x64\debug\libfbxsdk.dll"
// "C:\Program Files\Autodesk\FBX\FBX SDK\2017.0.1\lib\vs2015\x64\debug\libfbxsdk.pdb"
// "C:\Program Files\Autodesk\FBX\FBX SDK\2017.0.1\lib\vs2015\x64\release\libfbxsdk.dll"

// FBX SDK 2020.0.1 まではデバッグビルドに pdb ファイルが同梱されていたが、2020.1 には含まれなくなっている模様。
// 2020.2 以降は FBX PDB として単独で提供されている。

// String.Format() において、パラメータが1つだけの場合、[|var|] として Object 配列に明示的に格納しないと型推論が失敗する。
// 関数型言語の利点が裏目に出る例。いっそ sprintf を使ったほうがいいかも。

// System.Environment.GetFolderPath(System.Environment.SpecialFolder.ProgramFiles) は、
// OS ではなくメソッドを呼び出したプロセスが 32bit か 64bit かで変化するので、今回は使えない。
// .NET 4 で追加された Environment.Is64BitOperatingSystem プロパティをチェックして、自前でパスを構築する必要がある。

let copyFile fileName srcDirPath dstDirPath =
    let srcFilePath = System.IO.Path.Combine(srcDirPath, fileName)
    let dstFilePath = System.IO.Path.Combine(dstDirPath, fileName)
    printfn "Now copying from \"%s\" to \"%s\"" srcFilePath dstFilePath
    if not (System.IO.File.Exists(srcFilePath)) then
        printfn "Src file does not exist!!"
    else if not (System.IO.Directory.Exists(dstDirPath)) then
        printfn "Dst dir does not exist!!"
    else if (System.IO.File.Exists(dstFilePath)) then
        printfn "Dst file already exists!!"
        // HACK: 上書きするかどうかを確認して、OK であれば上書きする？　それとも問答無用で強制的に上書きする？
    else
        System.IO.File.Copy(srcFilePath, dstFilePath)

let copyD3DCompilerDlls srcArch dstArch isDebug =
    let fileName = "d3dcompiler_47.dll"
    let configuration = (if isDebug then "Debug" else "Release")
    let programFilesDirPath = @"C:\Program Files" + (if Environment.Is64BitOperatingSystem then " (x86)" else "")
    let srcDirPath = sprintf @"%s\Windows Kits\10\bin\%s" programFilesDirPath srcArch
    let dstDirPath = sprintf @"..\bin\%s\%s" dstArch configuration
    copyFile fileName srcDirPath dstDirPath

let copyFbxDlls srcArch dstArch isDebug =
    let extensions = (if isDebug then ["dll"; "pdb"] else ["dll"])
    for ext in extensions do
        let fileName = sprintf "libfbxsdk.%s" ext
        // FBX SDK のほうは configuration がすべて小文字だが、Windows のファイルシステムでは大文字・小文字の違いは無視される。
        let configuration = (if isDebug then "Debug" else "Release")
        let programFilesDirPath = @"C:\Program Files"
        let fbxSdkVersionName = "2020.3.1"
        let vsVersionName = "vs2015"
        let srcDirPath = sprintf @"%s\Autodesk\FBX\FBX SDK\%s\lib\%s\%s\%s" programFilesDirPath fbxSdkVersionName vsVersionName srcArch configuration
        let dstDirPath = sprintf @"..\bin\%s\%s" dstArch configuration
        copyFile fileName srcDirPath dstDirPath

try
    try
        let copyBy copyFunc =
            copyFunc "x86" "Win32" true
            copyFunc "x86" "Win32" false
            copyFunc "x64" "x64" true
            copyFunc "x64" "x64" false

        copyBy copyD3DCompilerDlls
        copyBy copyFbxDlls
    with
        | ex -> printfn "Message=\"%s\"" ex.Message
finally
    ignore()

printf "Press any key to exit..."
Console.ReadKey(true) |> ignore
