@echo off
setlocal enabledelayedexpansion

rem Links the repo's addon folder into devproject/addons, so build.bat output is picked up live

for %%I in ("%~dp0..\addons\vaudio-godot-native-openal-3d") do set SRC_DIR=%%~fI
set ADDON_DIR=%~dp0addons\vaudio-godot-native-openal-3d

if exist "%ADDON_DIR%" (
    for %%A in ("%ADDON_DIR%") do set ADDON_ATTR=%%~aA
    if "!ADDON_ATTR:~8,1!"=="l" (
        echo %ADDON_DIR% is already linked
        exit /b 0
    )
    echo %ADDON_DIR% exists and is not a link - delete it and re-run this script
    exit /b 1
)

if not exist "%~dp0addons" mkdir "%~dp0addons"
mklink /J "%ADDON_DIR%" "%SRC_DIR%" >nul || exit /b 1

echo Linked %SRC_DIR% into %ADDON_DIR%

endlocal
