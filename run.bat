@echo off
setlocal enabledelayedexpansion

set "type=%1"
set "impl=%2"

if "%type%"=="" (
    set "type=test"
)

if "%impl%"=="" (
    set "impl=1"
)

:: Compilation flags
set "FLAGS=/O2 /arch:AVX /nologo"
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
)

endlocal
