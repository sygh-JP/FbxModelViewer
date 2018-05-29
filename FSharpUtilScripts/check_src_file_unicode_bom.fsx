open System
open System.IO
open System.Text.RegularExpressions
open System.Linq

// ASCII のみで記述されたソースファイルは UTF-8/UTF-16 である必要はないが、
// 通例 UTF-8/UTF-16 を採用するべき。少なくとも Shift_JIS (CP932) などの ANSI MBCS は完全撲滅対象。

try
    let isRecursive = true
    let dirPath = ".."
    let pattern = ""

    // Visual Studio がビルド時に自動生成する一時中間ファイルは対象外。
    let regex = new Regex(@"^(?!.*TemporaryGeneratedFile_).+\.(cs|cpp|hpp|c|h|rc|rc2|fsx)$");
    let files = System.IO.Directory.EnumerateFiles(dirPath, "*.*", if isRecursive then SearchOption.AllDirectories else SearchOption.TopDirectoryOnly);
    //printfn "# Count of target files = %d" (files.Count())
    let filteredFiles = files.Where(fun fname -> regex.IsMatch(fname)).Select(fun f -> f)

    for file in filteredFiles do
        use fs = new FileStream(file, FileMode.Open, FileAccess.Read)
        use br = new BinaryReader(fs)
        let mark = br.ReadBytes(3) // とりあえず3バイト分読もうとしてみる。
        let size = mark.Length
        if ((size <> 3) || mark.[0] <> 0xEFuy || mark.[1] <> 0xBBuy || mark.[2] <> 0xBFuy) then
            // BOM 付き UTF-8 テキストではない。
            if ((size < 2) || mark.[0] <> 0xFFuy || mark.[1] <> 0xFEuy) then
                // UTF-16 テキストでもない。
                printfn "'%s' is neither UTF-8 nor UTF-16." file

with
    | ex -> printfn "Class=%s, Message=\"%s\"" (ex.GetType().ToString()) ex.Message

printf "Press any key to exit..."
Console.ReadKey(true) |> ignore
