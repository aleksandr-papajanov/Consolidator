@echo off
setlocal

set "ROOT=%~dp0.."
set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "SOURCE_DIR=%ROOT%\Native\Consolidator"
set "BUILD_DIR=%SOURCE_DIR%\out\build-vscode"

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

"%CMAKE%" -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX=OFF
if errorlevel 1 exit /b %errorlevel%

"%CMAKE%" --build "%BUILD_DIR%" --config Debug
exit /b %errorlevel%
