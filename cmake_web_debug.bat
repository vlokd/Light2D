@echo off
mkdir build_web_debug
pushd build_web_debug
call cmake ^
-DCMAKE_TOOLCHAIN_FILE=%EMSDK_PATH%/cmake/Modules/Platform/Emscripten.cmake ^
-DCMAKE_BUILD_TYPE=Debug -G "Ninja" ../
popd