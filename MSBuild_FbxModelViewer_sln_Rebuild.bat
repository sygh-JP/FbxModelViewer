@rem This batch file executes automatic build command by MSBuild.
@rem Fire by "Developer Command Prompt for VS2015"
@msbuild FbxModelViewer.sln /p:Platform="x64" /p:Configuration=Debug /t:Rebuild
@msbuild FbxModelViewer.sln /p:Platform="x64" /p:Configuration=Release /t:Rebuild
@msbuild FbxModelViewer.sln /p:Platform="Win32" /p:Configuration=Debug /t:Rebuild
@msbuild FbxModelViewer.sln /p:Platform="Win32" /p:Configuration=Release /t:Rebuild
@pause