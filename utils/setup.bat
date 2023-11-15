@echo off

:: Generate a Visual Studio solution
:: Visual Studio 2019: "Visual Studio 16 2019"
:: Visual Studio 2022: "Visual Studio 17 2022"
set IDE="Visual Studio 17 2022"
set BUILD_DIR="build"

echo Building project for %IDE%...

pushd ..
cmake -G %IDE% -S . -B %BUILD_DIR% .
popd

echo Finished building project in "build" folder!

pause