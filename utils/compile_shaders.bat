@echo off

set BAT_DIR=%~dp0

pushd %BAT_DIR%

cd %BAT_DIR%

:: The parameters to this python file are the shader source directory, followed by the shader output directory, and finally (and optionally) the shader include directory. 
:: Note that all provided paths are relative to the root of the project directory. 
:: The shader include directory should only include any glsl files that are '#include'd in other shader files. If unspecified, the compiler will error out in any shader using that preprocessor directive
:: Template - compile_shaders.py <src_dir> <out_dir> <include_dir>
:: E.g. - compile_shaders.py "C:\my_src_dir" "C:\my_out_dir" "C:\my_include_dir"
call py ./python/compile_shaders.py "src/shaders" "out/shaders" "src/shaders/common"

popd

pause