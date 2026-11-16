rem Human-reviewed

@echo off
setlocal

if exist .env (
    for /f "usebackq tokens=1,* delims==" %%A in (".env") do set %%A=%%B
)

rem Invalid doc XML (e.g. a bare < in a description) silently breaks every editor tooltip
where xmllint >nul 2>&1
if errorlevel 1 (
    echo WARNING: xmllint not found - skipping doc_classes XML validation
) else (
    xmllint --noout doc_classes\*.xml
    if errorlevel 1 exit /b 1
)

scons target=editor
if errorlevel 1 exit /b 1

scons target=template_debug
if errorlevel 1 exit /b 1

scons target=template_release
if errorlevel 1 exit /b 1

set ADDON_DIR=addons\vaudio-godot-native-openal-3d
set BIN_DIR=%ADDON_DIR%\bin\windows

rem Copies bin\windows (the plugin, vaudionative, OpenAL, and the debug window + GLFW when vendored) into each addon folder
setlocal enabledelayedexpansion
for %%V in (VAUDIO_RELEASE_DIR VAUDIO_DEMO_ADDON_DIR) do (
    set DEST=!%%V!
    if "!DEST!"=="" (
        echo %%V not set - skipping copy
    ) else (
        echo Copying build output to !DEST!
        if not exist "!DEST!\bin\windows" mkdir "!DEST!\bin\windows"
        for %%F in ("%BIN_DIR%\*.dll" "%BIN_DIR%\*.exe") do copy /y "%%~F" "!DEST!\bin\windows\" >nul
        for %%F in (plugin.cfg plugin.gd vaudio_godot_native_openal_3d.gdextension) do copy /y "%ADDON_DIR%\%%F" "!DEST!\" >nul
    )
)

endlocal
endlocal
