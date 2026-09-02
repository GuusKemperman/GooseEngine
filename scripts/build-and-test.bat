@echo off
REM Builds every configuration and runs the tests. Bails on the first error.
setlocal
cd /d "%~dp0.." || exit /b 1

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvars64.bat`) do set "VCVARS=%%i"
if not defined VCVARS (
	echo error: no Visual Studio install with the C++ toolset was found>&2
	exit /b 1
)
call "%VCVARS%" || exit /b 1

REM Only configure when there is no cache yet. "cmake --build" re-runs the configure itself
REM via ninja's RERUN_CMAKE edge when a CMakeLists.txt changes, and configuring
REM unconditionally would rewrite every CXXDependInfo.json and force all 31 module dyndep
REM steps to re-run on every commit.
REM
REM Deliberately not a for loop -- cmd.exe silently discards "exit /b 1" from a
REM multi-statement loop body, which would let a failing build pass the pre-commit hook.
if not exist "out\build\x64-Debug\CMakeCache.txt" (cmake --preset x64-Debug || exit /b 1)
cmake --build --preset x64-Debug || exit /b 1
if not exist "out\build\x64-Release\CMakeCache.txt" (cmake --preset x64-Release || exit /b 1)
cmake --build --preset x64-Release || exit /b 1

pushd out\build\x64-Debug\bin && .\test_runner.exe || exit /b 1
popd
pushd out\build\x64-Release\bin && .\test_runner.exe || exit /b 1
popd
