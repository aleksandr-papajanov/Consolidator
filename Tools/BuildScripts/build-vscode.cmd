@echo off
setlocal
set "ROOT=%~dp0..\.."
set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "SOURCE_DIR=%ROOT%\Consolidator.Native"
set "BUILD_DIR=%SOURCE_DIR%\build"
set "MIN_API_DIR=D:\Projects\Ableton\Consolidator\Native\Consolidator\Source\min-api"

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

echo [CMAKE] Configuring...
"%CMAKE%" -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 18 2026" -A x64 -DC74_MIN_API_DIR="%MIN_API_DIR%"
if errorlevel 1 exit /b %errorlevel%

echo [BUILD] Building Core + Max...
"%CMAKE%" --build "%BUILD_DIR%" --config RelWithDebInfo
if errorlevel 1 exit /b %errorlevel%

echo [TEST] Running Core tests...
"%BUILD_DIR%\RelWithDebInfo\ConsolidatorCoreTests.exe"
if errorlevel 1 exit /b %errorlevel%

echo ============================================================
echo   Build + tests passed
echo ============================================================
exit /b 0