@echo off

echo · Preparing directories...
rem # set a variable named `CWD` with the value of `%~dp0`
rem # that expands to the current drive and path at device 0.
set CWD=%~dp0
set SOURCE=%CWD%..
set DEST=%CWD%..\build
mkdir "%DEST%"

echo Source directory: %SOURCE%
echo DEST directory: %DEST%

exit 0

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" set "vswhere=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" (
	echo ERROR: Failed to find vswhere.exe
	exit 1
)
echo Found %vswhere%

echo Looking for VC...
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set vc_dir=%%i
)
if not exist "%vc_dir%\Common7\Tools\vsdevcmd.bat" (
	echo ERROR: Failed to find VC tools x86/x64
	exit 1
)
echo Found %vc_dir%

call "%vc_dir%\Common7\Tools\vsdevcmd.bat" -arch=x86 -host_arch=x64

echo Building webview.dll (x86)
mkdir "%SOURCE%\dll\x86"
cl /D "WEBVIEW_API=__declspec(dllexport)" ^
	/I "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\include" ^
	"%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x86\WebView2Loader.dll.lib" ^
	/std:c++17 /EHsc "/Fo%DEST%"\ ^
	"%SOURCE%\webview.cc" /link /DLL "/OUT:%DEST%\webview.dll" || exit \b
copy "%DEST%\webview.dll" "%SOURCE%\dll\x86"
copy "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x86\WebView2Loader.dll" "%SOURCE%\dll\x86"

call "%vc_dir%\Common7\Tools\vsdevcmd.bat" -arch=x64 -host_arch=x64
echo Building webview.dll (x64)
mkdir "%SOURCE%\dll\x64"
cl /D "WEBVIEW_API=__declspec(dllexport)" ^
	/I "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\include" ^
	"%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x64\WebView2Loader.dll.lib" ^
	/std:c++17 /EHsc "/Fo%DEST%"\ ^
	"%SOURCE%\webview.cc" /link /DLL "/OUT:%DEST%\webview.dll" || exit \b
copy "%DEST%\webview.dll" "%SOURCE%\dll\x64"
copy "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x64\WebView2Loader.dll" "%DEST%"
copy "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x64\WebView2Loader.dll" "%SOURCE%\dll\x64"

echo Building webview.exe (x64)
cl /I "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\include" ^
	"%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x64\WebView2Loader.dll.lib" ^
	/std:c++17 /EHsc "/Fo%DEST%"\ ^
	"%SOURCE%\main.cc" /link "/OUT:%DEST%\webview.exe" || exit \b

echo Building webview_test.exe (x64)
cl /I "%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\include" ^
	"%SOURCE%\script\microsoft.web.webview2.1.0.664.37\build\native\x64\WebView2Loader.dll.lib" ^
	/std:c++17 /EHsc "/Fo%DEST%"\ ^
	"%SOURCE%\webview_test.cc" /link "/OUT:%DEST%\webview_test.exe" || exit \b
