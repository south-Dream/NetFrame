@echo off
setlocal EnableExtensions

set "PROJECT_DIR=%~dp0"
set "PROTO_ROOT=%PROJECT_DIR%proto"
set "PROTO_DIR=%PROTO_ROOT%\message"
set "OUTPUT_DIR=%PROTO_ROOT%\generated"
set "CSHARP_OUTPUT_DIR=%~1"
set "VCPKG_TRIPLET=%VCPKG_DEFAULT_TRIPLET%"
if not defined VCPKG_TRIPLET set "VCPKG_TRIPLET=x64-windows"
set "PROTOC_EXE="
set "EXIT_CODE=0"

echo ================================
echo        Protobuf Generator
echo ================================
echo.

echo [INFO] Project Dir : "%PROJECT_DIR%"
echo [INFO] Proto Dir   : "%PROTO_DIR%"
echo [INFO] Output Dir  : "%OUTPUT_DIR%"
if defined CSHARP_OUTPUT_DIR echo [INFO] CSharp Dir  : "%CSHARP_OUTPUT_DIR%"
echo.

rem ================================
rem Check proto directory
rem ================================

if not exist "%PROTO_DIR%\" (
    echo [ERROR] Proto directory not found:
    echo "%PROTO_DIR%"
    set "EXIT_CODE=1"
    goto END
)

rem ================================
rem Find protoc.exe from PATH
rem ================================

for /f "delims=" %%I in ('where protoc 2^>nul') do (
    if not defined PROTOC_EXE (
        set "PROTOC_EXE=%%I"
    )
)

rem ================================
rem Find protoc.exe from the project manifest installation
rem ================================

if not defined PROTOC_EXE (
    if exist "%PROJECT_DIR%out\build\x64\vcpkg_installed\%VCPKG_TRIPLET%\tools\protobuf\protoc.exe" (
        set "PROTOC_EXE=%PROJECT_DIR%out\build\x64\vcpkg_installed\%VCPKG_TRIPLET%\tools\protobuf\protoc.exe"
    )
)

rem ================================
rem Find protoc.exe from VCPKG_ROOT
rem ================================

if not defined PROTOC_EXE (
    if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\tools\protobuf\protoc.exe" (
        set "PROTOC_EXE=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\tools\protobuf\protoc.exe"
    )
)

rem ================================
rem protoc.exe not found
rem ================================

if not defined PROTOC_EXE (
    echo [ERROR] protoc.exe was not found.
    echo.
    echo Please:
    echo   1. Add protoc.exe to PATH
    echo   or
    echo   2. Set VCPKG_ROOT
    echo.
    set "EXIT_CODE=1"
    goto END
)

echo [INFO] protoc.exe:
echo "%PROTOC_EXE%"
echo.

rem ================================
rem Create output directory
rem ================================

if not exist "%OUTPUT_DIR%\" (
    echo [INFO] Creating output directory...

    mkdir "%OUTPUT_DIR%"

    if errorlevel 1 (
        echo [ERROR] Could not create output directory:
        echo "%OUTPUT_DIR%"
        set "EXIT_CODE=1"
        goto END
    )
)

if defined CSHARP_OUTPUT_DIR (
    if not exist "%CSHARP_OUTPUT_DIR%\" (
        mkdir "%CSHARP_OUTPUT_DIR%"
        if errorlevel 1 (
            echo [ERROR] Could not create CSharp output directory:
            echo "%CSHARP_OUTPUT_DIR%"
            set "EXIT_CODE=1"
            goto END
        )
    )
)

rem ================================
rem Generate protobuf C++ files
rem ================================

set /a PROTO_COUNT=0

for /r "%PROTO_DIR%" %%F in (*.proto) do (
    set /a PROTO_COUNT+=1

    echo --------------------------------
    echo [PROTO] %%~fF

    if defined CSHARP_OUTPUT_DIR (
        "%PROTOC_EXE%" ^
            --proto_path="%PROTO_DIR%" ^
            --cpp_out="%OUTPUT_DIR%" ^
            --csharp_out="%CSHARP_OUTPUT_DIR%" ^
            "%%~fF"
    ) else (
        "%PROTOC_EXE%" ^
            --proto_path="%PROTO_DIR%" ^
            --cpp_out="%OUTPUT_DIR%" ^
            "%%~fF"
    )

    if errorlevel 1 (
        echo.
        echo [ERROR] Failed to generate C++ files for:
        echo "%%~fF"
        set "EXIT_CODE=1"
        goto END
    )
)

rem ================================
rem No proto files
rem ================================

if %PROTO_COUNT% equ 0 (
    echo [INFO] No .proto files were found under:
    echo "%PROTO_DIR%"
    goto END
)

rem ================================
rem Success
rem ================================

echo.
echo ================================
echo [OK] Generated %PROTO_COUNT% proto file(s)
echo [OK] C++ Output: "%OUTPUT_DIR%"
if defined CSHARP_OUTPUT_DIR echo [OK] CSharp Output: "%CSHARP_OUTPUT_DIR%"
echo ================================

:END

echo.
if "%EXIT_CODE%"=="0" (
    echo [DONE] Script finished successfully.
) else (
    echo [FAILED] Script finished with errors.
)

endlocal & exit /b %EXIT_CODE%
