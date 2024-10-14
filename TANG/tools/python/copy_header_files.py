
import os
import sys
import shutil
from pathlib import Path
from print_utils import *

def CopyHeaders() -> int:

    projDir = os.getenv("TANG_PROJ_DIR")
    if projDir is None:
        LogError("Failed to copy headers! Project directory is not set")
        return 0
        
    projDir = os.path.normpath(projDir)
    
    srcDir = sys.argv[1]
    srcPath = f"{projDir}/{srcDir}"
    if srcDir is None or not os.path.exists(srcPath):
        LogError(f'Failed to copy headers! Please provide a valid source path: "{srcPath}"')
        return 0
        
    srcPath = os.path.normpath(srcPath)
        
    dstDir = sys.argv[2]
    dstPath = f"{projDir}/{dstDir}"
    if dstDir is None:
        LogError(f'Failed to copy headers! Please provide a valid destination path: "{dstPath}"')
        return 0
        
    dstPath = os.path.normpath(dstPath)
        
    # Create the dst directory if it doesn't already exist
    os.makedirs(dstPath, exist_ok=True)
    
    LogInfo(f'Copying headers from "{os.path.normpath(srcPath)}" to "{os.path.normpath(dstPath)}"')
    
    srcPathObj = Path(srcPath)
    headerList = []
    for path in srcPathObj.rglob(f"*.h"):
        relHeaderPath = path.relative_to(srcPath)
        headerList.append(relHeaderPath)
        
    # Clear out the destination path to prepare for copy operations
    # The dst path should be correctly set at this point, but just to
    # be extra sure we'll check if it's a sub-directory of the project dir
    dstPathObj = Path(dstPath)
    projDirObj = Path(projDir)
    if projDirObj not in dstPathObj.parents:
        LogError(f'Failed to copy header! Destination path ("{dstPath}") is not part of the project directory ("{projDir}")')
        return 0
    
    shutil.rmtree(dstPath)
        
    # Copy only header files into the same directory structure in out folder
    for headerPath in headerList:
        srcHeaderPath = srcPath / headerPath
        outHeaderPath = dstPath / headerPath.parent
        os.makedirs(outHeaderPath, exist_ok=True)
        shutil.copy(srcHeaderPath, outHeaderPath)
        LogInfo(f'Copied {headerPath}')
        
    return len(headerList)
        

def main():
    
    LogBuildStep("COPY HEADERS START")
    
    numHeadersCopied = CopyHeaders()
        
    LogInfo(f"Copied {numHeadersCopied} header files!")
    LogBuildStep("COPY HEADERS END")
    
        
if __name__ == "__main__":
    main()