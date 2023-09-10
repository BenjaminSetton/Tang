@echo off

:: Generate a Visual Studio 2019 solution
set IDE="Visual Studio 17 2022"
set BUILD_DIR="build"

echo Building project for %IDE%, are you sure you want to proceed?

pause

pushd ..
cmake -G %IDE% -S . -B %BUILD_DIR% .
popd

echo Finished building project in "build" folder!

pause