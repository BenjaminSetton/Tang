
import os
import sys
from pathlib import Path

def main():
    vulkanSDK = os.environ["VULKAN_SDK"]
    sourceDir = "src/shaders"
    outputDir = "out/shaders"
    projectDir = str(Path(sys.argv[0]).parent.absolute().parent.parent) # This is ugly, but for some reason it won't let me do .parent 3 times...
    
    # Ensure the Vulkan SDK is installed
    if vulkanSDK is None:
        print("Please install the Vulkan SDK to compile shaders")
        return
        
    print("Building shaders start...")
    
    shaderCompiler = os.path.normpath(f"{vulkanSDK}/Bin/glslc.exe")
    print(f"Compiling shaders using '{shaderCompiler}'")
    
    # Check that the output directory exists
    os.makedirs(f"{projectDir}/{outputDir}", exist_ok=True)
    
    # Get list of source shaders to compile
    shaderList = []
    for path in Path(f"{projectDir}/{sourceDir}").rglob("*.vert"):
        shaderList.append(path)
        
    for path in Path(f"{projectDir}/{sourceDir}").rglob("*.frag"):
        shaderList.append(path)
    
    # TODO - Checksum shader sources and only compile those which are different from the ones
    #        we already compiled
    
    # Compile the shader sources
    for shaderPath in shaderList:
        shaderName = (shaderPath.suffix)[1:]
        parentDirName = shaderPath.parent.name
        fullOutputPath = os.path.normpath(f"{projectDir}/{outputDir}/{parentDirName}")
        
        os.makedirs(fullOutputPath, exist_ok=True)
        os.system(f'{shaderCompiler} "{shaderPath}" -o "{fullOutputPath}/{shaderName}.spv"')
        
        # Log the source and output paths to the console
        fullShaderDstPath = str(f'{fullOutputPath}/{shaderName}')
        shortShaderSrcPath = f"./TANG{str(shaderPath).replace(projectDir, '')}"
        shortShaderDstPath = f"./TANG{fullShaderDstPath.replace(projectDir, '')}"
        print(f'[SHADER] Compiled "{os.path.normpath(shortShaderSrcPath)}" into "{os.path.normpath(shortShaderDstPath)}.spv"')
    
    print("Finished building shaders!")

if __name__ == "__main__":
    main()