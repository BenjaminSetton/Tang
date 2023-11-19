@echo off

set BAT_DIR=%~dp0

pushd %BAT_DIR%

cd %BAT_DIR%

call py ./python/compile_shaders.py

popd

pause