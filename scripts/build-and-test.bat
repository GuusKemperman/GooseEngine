@echo off
REM Builds every configuration and runs the tests. Bails on the first error.
setlocal
cd /d "%~dp0.." || exit /b 1

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvars64.bat`) do set "VCVARS=%%i"
if not defined VCVARS (
	echo error: no Visual Studio install with the C++ toolset was found.>&2
	echo        Install the "Desktop development with C++" workload.>&2
	exit /b 1
)
call "%VCVARS%" >nul || (
	echo error: failed to initialise the Visual Studio build environment.>&2
	exit /b 1
)

REM Only configure when there is no cache yet. "cmake --build" re-runs the configure itself
REM via ninja's RERUN_CMAKE edge when a CMakeLists.txt changes, and configuring
REM unconditionally would rewrite every CXXDependInfo.json and force all 31 module dyndep
REM steps to re-run on every commit.
REM
REM Deliberately not a for loop -- cmd.exe silently discards "exit /b 1" from a
REM multi-statement loop body, which would let a failing build pass the pre-commit hook.
call :build x64-Debug || exit /b 1
call :build x64-Release || exit /b 1
call :test x64-Debug || exit /b 1
call :test x64-Release || exit /b 1
exit /b 0

:build
if not exist "out\build\%~1\CMakeCache.txt" (
	cmake --preset %~1 || (
		echo error: cmake configure failed for %~1.>&2
		exit /b 1
	)
)
cmake --build --preset %~1 || (
	echo error: build failed for %~1.>&2
	exit /b 1
)
exit /b 0

:test
pushd "out\build\%~1\bin" || (
	echo error: %~1 was not built -- out\build\%~1\bin is missing.>&2
	exit /b 1
)
.\test_runner.exe || (
	echo error: tests failed for %~1.>&2
	exit /b 1
)
popd
exit /b 0
