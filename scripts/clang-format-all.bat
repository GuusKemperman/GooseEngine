@echo off
REM Runs clang-format in place on every .ixx, .cpp and .h file in the repository, using the
REM _clang-format at the repo root. The pre-commit hook only checks staged files; this fixes them.
REM
REM Files are enumerated with git, so build output (out\), .vs\ and anything else in
REM .gitignore is skipped. Untracked files that are not ignored are still formatted.
setlocal
cd /d "%~dp0.." || exit /b 1

where clang-format >nul 2>&1 || (
	echo error: clang-format was not found on your PATH.>&2
	echo        Visual Studio ships one under VC\Tools\Llvm\bin in its>&2
	echo        install directory -- add that to PATH, or install LLVM.>&2
	exit /b 1
)

REM A single clang-format process for every file. Passing the list via --files also sidesteps
REM cmd.exe's command line length limit.
set "FILES=%TEMP%\goose-clang-format-%RANDOM%.txt"
git ls-files --cached --others --exclude-standard -- "*.ixx" "*.cpp" "*.h" > "%FILES%" || (
	del "%FILES%" 2>nul
	exit /b 1
)
clang-format -i --verbose --files="%FILES%"
set "RESULT=%ERRORLEVEL%"
del "%FILES%"
exit /b %RESULT%
