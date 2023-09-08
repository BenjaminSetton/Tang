@echo off

:: Generate a Visual Studio 2019 solution
set IDE="Visual Studio 16 2019"
set BUILD_DIR="build"

echo Building project...

pushd ..
cmake -G %IDE% -S . -B %BUILD_DIR% .
popd

echo Finished building project in "build" folder!

pause