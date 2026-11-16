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

rem x86_64 builds into bin\windows, arm64 into bin\windows-arm64 (see SConstruct)
for %%A in (x86_64 arm64) do (
    for %%T in (editor template_debug template_release) do (
        scons target=%%T arch=%%A
        if errorlevel 1 exit /b 1
    )
)

set ADDON_DIR=addons\vaudio-godot-native-openal-3d

rem Copies bin\windows and bin\windows-arm64 (the plugin, vaudionative, OpenAL, and the debug window + GLFW when vendored) into each addon folder
setlocal enabledelayedexpansion
for %%V in (VAUDIO_RELEASE_DIR VAUDIO_DEMO_ADDON_DIR) do (
    set DEST=!%%V!
    if "!DEST!"=="" (
        echo %%V not set - skipping copy
    ) else (
        echo Copying build output to !DEST!
        for %%P in (windows windows-arm64) do (
            if not exist "!DEST!\bin\%%P" mkdir "!DEST!\bin\%%P"
            for %%F in ("%ADDON_DIR%\bin\%%P\*.dll" "%ADDON_DIR%\bin\%%P\*.exe") do copy /y "%%~F" "!DEST!\bin\%%P\" >nul
        )
        for %%F in (plugin.cfg plugin.gd vaudio_godot_native_openal_3d.gdextension) do copy /y "%ADDON_DIR%\%%F" "!DEST!\" >nul
    )
)

endlocal
endlocal
