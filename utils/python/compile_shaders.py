
# %SHADER_COMPILER% "%SRC_SHADER_PATH%/%%f.frag" -o "%OUT_SHADER_PATH%/%%f/frag.spv"

import os
import sys
from pathlib import Path

def main():

    print("Running from python script!")

    vulkanSDK = os.environ["VULKAN_SDK"]
    sourceDir = "src/shaders"
    outputDir = "out/shaders"
    workingDir = Path(sys.argv[0]).parent.absolute().parent.parent # This is ugly, but for some reason it won't let me do .parent 3 times...
    
    # Ensure the Vulkan SDK is installed
    if vulkanSDK is None:
        print("Please install the Vulkan SDK to compile shaders")
        return
    
    shaderCompiler = os.path.normpath(f"{vulkanSDK}/Bin/glslc.exe")
    print(f"Compiling shaders using '{shaderCompiler}'")
    
    # Check that the output directory exists
    os.makedirs(f"{workingDir}/{outputDir}", exist_ok=True)
    
    # Get list of source shaders to compile
    shaderList = []
    for path in Path(f"{workingDir}/{sourceDir}").rglob("*.vert"):
        shaderList.append(path)
        
    for path in Path(f"{workingDir}/{sourceDir}").rglob("*.frag"):
        shaderList.append(path)

    print("Building shaders start...")
    
    for shaderPath in shaderList:
        shaderName = (shaderPath.suffix)[1:]
        parentDirName = shaderPath.parent.name
        fullOutputPath = os.path.normpath(f"{workingDir}/{outputDir}/{parentDirName}")
        
        os.makedirs(fullOutputPath, exist_ok=True)
        #print(f'{shaderCompiler} "{shaderPath}" -o "{fullOutputPath}/{shaderName}.spv"')
        os.system(f'{shaderCompiler} "{shaderPath}" -o "{fullOutputPath}/{shaderName}.spv"')
        print(f'Compiled "{shaderPath}" into "{fullOutputPath}/{shaderName}.spv"')
    
    print("Finished building shaders!")

if __name__ == "__main__":
    main()