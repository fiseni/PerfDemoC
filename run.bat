@echo off
setlocal enabledelayedexpansion

set "arg1=data/masterParts.txt"
set "arg2=data/parts.txt"

if "%1"=="short" (
    set "arg1=data/masterPartsShort.txt"
    set "arg2=data/partsShort.txt"
)

set "impl=%2"
if "%impl%"=="" (
    set "impl=4"
)

:: Compilation flags
set "FLAGS=/permissive- /GS /GL /Gy /Gm- /W3 /WX- /O2 /Oi /sdl /Gd /MD /arch:AVX2 /EHsc /Zc:inline /fp:precise /Zc:forScope /nologo /D ""_CRT_SECURE_NO_WARNINGS"" /D ""_CONSOLE"""
set "FILES=main.c utils.c cross_platform_time.c hash_table_string.c hash_table_sizelist.c source_data.c test.c processor%impl%.c"

if "%1"=="test" (
    cl %FLAGS% %FILES% /Fe:demo.exe
    if errorlevel 1 (
        echo Build failed.
        exit /b 1
    )
    demo.exe test %arg2%
    del *.obj
) else (
    cl %FLAGS% /D "NDEBUG" %FILES% /Fe:demo.exe
    if errorlevel 1 (
        echo Build failed.
        exit /b 1
    )
    demo.exe %arg1% %arg2%
    del *.obj
)

endlocal
