@rem This batch file executes automatic build command by MSBuild.
@rem Fire by "Developer Command Prompt for VS2015"
@setlocal
@set sln_path="..\FbxModelViewer.sln"
@msbuild %sln_path% /p:Platform="x64" /p:Configuration="Debug" /t:Rebuild
@set build_status=%errorlevel%
@if not %build_status%==0 goto label_exit
@msbuild %sln_path% /p:Platform="x64" /p:Configuration="Release" /t:Rebuild
@set build_status=%errorlevel%
@if not %build_status%==0 goto label_exit
@msbuild %sln_path% /p:Platform="Win32" /p:Configuration="Debug" /t:Rebuild
@set build_status=%errorlevel%
@if not %build_status%==0 goto label_exit
@msbuild %sln_path% /p:Platform="Win32" /p:Configuration="Release" /t:Rebuild
@set build_status=%errorlevel%
@if not %build_status%==0 goto label_exit
@echo All builds succeeded.
:label_exit
@endlocal
@pause