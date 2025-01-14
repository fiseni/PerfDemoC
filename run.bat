@echo off
setlocal enabledelayedexpansion

set "type=%1"
set "impl=%2"

if "%type%"=="" (
    set "type=test"
)

if "%impl%"=="" (
    set "impl=4"
)

:: Compilation flags
set "FLAGS=/permissive- /GS /GL /Gy /Gm- /W3 /WX- /O2 /Oi /sdl /Gd /MD /arch:AVX2 /EHsc /Zc:inline /fp:precise /Zc:forScope /nologo /D ""_CRT_SECURE_NO_WARNINGS"" /D ""NDEBUG"" /D ""_CONSOLE"""
set "FILES=main.c utils.c cross_platform_time.c hash_table_string.c hash_table_sizelist.c source_data.c test.c processor%impl%.c"

if "%type%"=="echo" (
    echo cl %FLAGS% %FILES% /Fe:demo.exe
) else (
    cl %FLAGS% %FILES% /Fe:demo.exe
    if errorlevel 1 (
        echo Build failed.
        exit /b 1
    )
    demo.exe %type%
    del *.obj
)

endlocal
