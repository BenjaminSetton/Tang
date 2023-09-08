@echo off

echo Compiling shaders...

C:\VulkanSDK\1.3.261.0\Bin\glslc.exe ../src/shaders/shader.vert -o ../src/shaders/vert.spv
C:\VulkanSDK\1.3.261.0\Bin\glslc.exe ../src/shaders/shader.frag -o ../src/shaders/frag.spv

echo Finished compiling shaders!

pause