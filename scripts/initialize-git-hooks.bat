@echo off
REM Creates or replaces the git pre-commit hook.
setlocal
cd /d "%~dp0.." || exit /b 1

for /f "delims=" %%h in ('git rev-parse --git-path hooks') do set "HOOK=%%h/pre-commit"
if not defined HOOK (
	echo error: not inside a git repository>&2
	exit /b 1
)

> "%HOOK%" echo #!/bin/sh
>>"%HOOK%" echo # clang-format the staged C++ files, then build every config and run the tests
>>"%HOOK%" echo set -e
>>"%HOOK%" echo.
>>"%HOOK%" echo if ! command -v clang-format ^>/dev/null 2^>^&1; then
>>"%HOOK%" echo 	echo "pre-commit: clang-format was not found on your PATH." ^>^&2
>>"%HOOK%" echo 	echo "            Visual Studio ships one under VC\Tools\Llvm\bin in its" ^>^&2
>>"%HOOK%" echo 	echo "            install directory -- add that to PATH, or install LLVM." ^>^&2
>>"%HOOK%" echo 	exit 1
>>"%HOOK%" echo fi
>>"%HOOK%" echo.
>>"%HOOK%" echo if ! git diff --cached -z --name-only --diff-filter=ACM -- '*.cpp' '*.h' '*.ixx' ^| xargs -0 -r clang-format --dry-run -Werror; then
>>"%HOOK%" echo 	echo "" ^>^&2
>>"%HOOK%" echo 	echo "pre-commit: the file(s) above are not formatted to _clang-format." ^>^&2
>>"%HOOK%" echo 	echo "            Fix them with: clang-format -i PATH..." ^>^&2
>>"%HOOK%" echo 	exit 1
>>"%HOOK%" echo fi
>>"%HOOK%" echo.
>>"%HOOK%" echo exec "$(git rev-parse --show-toplevel)/scripts/build-and-test.bat"

echo Installed %HOOK%
