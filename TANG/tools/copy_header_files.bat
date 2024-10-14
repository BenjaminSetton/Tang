@echo off

set BAT_DIR=%~dp0

pushd %BAT_DIR%

cd %BAT_DIR%

:: Copies all header files from the TANG source directory to an "include/TANG" directory in the output folder. The TANG folder is
:: included so that whenever a project includes this library it will be able to scope it's includes like so: #include "TANG/HEADER.H"
:: Template - copy_header_files.py <src_dir> <out_dir>
:: E.g. - copy_header_files.py "C:\my_src_dir" "C:\my_out_dir"
call py ./python/copy_header_files.py "TANG/src" "TANG/out/include/TANG"

popd

pause