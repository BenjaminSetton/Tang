@echo off

IF "%VULKAN_SDK%"=="" (
	echo Vulkan SDK not found. Please make sure the Vulkan SDK is installed. If it is, make sure you have the VULKAN_SDK environment variable set correctly
	pause
	exit /b 0
)

set SHADER_COMPILER=%VULKAN_SDK%/Bin/glslc.exe
set SRC_SHADER_PATH=src/shaders
set OUT_SHADER_PATH=out/shaders/compiled

echo Building shaders start...
echo Compiling shaders using %SHADER_COMPILER%

pushd ..

:: Make the out directory if it doesn't exist
IF NOT EXIST out (
	mkdir out
)

cd out

:: Make the out/shaders directory if it doesn't exist
IF NOT EXIST shaders (
	mkdir shaders
)

cd ..

%SHADER_COMPILER% %SRC_SHADER_PATH%/shader.vert -o %OUT_SHADER_PATH%/vert.spv
%SHADER_COMPILER% %SRC_SHADER_PATH%/shader.frag -o %OUT_SHADER_PATH%/frag.spv
popd

echo Finished building shaders!

pause