@echo off
rem build.cmd <folder> -- builds the C CLI app in <folder> using that
rem folder's own Makefile. With no argument, builds every folder in
rem this repo that has a Makefile.
rem
rem   build.cmd tribute_to_tony
rem   build.cmd
setlocal

if "%~1"=="" goto build_all

if not exist "%~1\" (
    echo "%~1" is not a folder
    exit /b 1
)
if not exist "%~1\Makefile" (
    echo No Makefile in "%~1"
    exit /b 1
)
pushd "%~1"
mingw32-make %2 %3 %4 %5
set RC=%ERRORLEVEL%
popd
exit /b %RC%

:build_all
for /d %%D in (*) do (
    if exist "%%D\Makefile" (
        echo === %%D ===
        pushd "%%D"
        mingw32-make
        popd
    )
)
exit /b 0
