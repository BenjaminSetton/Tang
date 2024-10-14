@echo off

:: Generate a Visual Studio solution
:: Visual Studio 2019: "vs2019"
:: Visual Studio 2022: "vs2022"
set GENERATOR="vs2022"
set BUILD_DIR="build"

:: Store the TANG project directory in an environment variable
setx TANG_PROJ_DIR %~dp0

echo Building project for %GENERATOR%...

call "TANG/vendor/premake/premake5.exe" %GENERATOR%

echo Finished building project in %BUILD_DIR% folder!

pause