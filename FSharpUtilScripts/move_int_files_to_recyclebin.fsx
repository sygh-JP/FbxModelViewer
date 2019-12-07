// http://msdn.microsoft.com/ja-jp/library/vstudio/dd233175.aspx
// https://docs.microsoft.com/en-us/dotnet/fsharp/tutorials/fsharp-interactive/index
#if INTERACTIVE
#r @"Microsoft.VisualBasic.dll"
#endif

//#light "off"

open System
open Microsoft.VisualBasic.FileIO

//let taskDescMsg = "バックアップに不要なファイルおよびビルド中間生成物をごみ箱に移動します。"
let taskDescMsg = "Now moving files and internal built products to recycle bin, not required to be backed-up."
printfn "%s" taskDescMsg

let moveFileFolder fileFolderName moveFunc existsFunc =
    try
        //let canceledMsg = "移動がキャンセルされました。"
        let canceledMsg = "Moving canceled."
        try
            // HACK: ファイルなのかフォルダーなのか判断して、呼び出す関数を分けるような分岐を実装してもよい。
            printfn "Now moving : <%s>" fileFolderName
            if existsFunc(fileFolderName) then
                moveFunc(fileFolderName, UIOption.AllDialogs, RecycleOption.SendToRecycleBin)
        with
            | :? OperationCanceledException as ex -> printfn "%s; Message=\"%s\"" canceledMsg ex.Message
            | ex -> printfn "Message=\"%s\"" ex.Message
    finally
        ignore()

let moveFile fileName = moveFileFolder fileName FileSystem.DeleteFile System.IO.File.Exists
let moveFolder fileName = moveFileFolder fileName FileSystem.DeleteDirectory System.IO.Directory.Exists

let moveBinObjFolders baseDirName =
    moveFolder (baseDirName + @"\bin")
    moveFolder (baseDirName + @"\obj")

//moveFile ".\\test.txt"

// VC++ 2015 Update 2 以降は .sdf (SQL Server Compact Edition Database File) によるインテリセンス データベース ファイルが廃止されて .VC.db となった模様。
// 拡張子を変えただけ？　フォーマットも違う？
moveFile @"..\FbxModelViewer.sdf"
moveFile @"..\FbxModelViewer.VC.db"
//moveFile @"..\FbxModelViewer.v11.suo"
//moveFile @"..\FbxModelViewer.v12.suo"
moveFolder @"..\.vs"
//moveFolder @"..\.svn"
moveFolder @"..\ipch"
moveFolder @"..\bin"
moveFolder @"..\Win32"
moveFolder @"..\x64"
//moveFolder @"..\DirectXTexMisc\Win32"
//moveFolder @"..\DirectXTexMisc\x64"
//moveFolder @"..\FbxLoader\Win32"
//moveFolder @"..\FbxLoader\x64"
moveFolder @"..\FbxModelMonitor\obj"
moveFolder @"..\FbxModelMonitor\Win32"
moveFolder @"..\FbxModelMonitor\x64"
moveFolder @"..\MyWpfGraphLibMfc\obj"
moveFolder @"..\MyWpfGraphLibMfc\Win32"
moveFolder @"..\MyWpfGraphLibMfc\x64"
moveFolder @"..\SharedUtility\obj"
moveFolder @"..\SharedUtility\Win32"
moveFolder @"..\SharedUtility\x64"

moveFile @"..\FbxModelMonitor\FbxModelMonitor.aps"
moveFile @"..\MyWpfGraphLibMfc\MyWpfGraphLibMfc.aps"

moveBinObjFolders @"..\MyWpfGraphLibrary"
moveBinObjFolders @"..\WpfFontBmpWriter"

printf "Press any key to exit..."
Console.ReadKey(true) |> ignore
