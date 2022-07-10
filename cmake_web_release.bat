@echo off
mkdir build_web_release
pushd build_web_release
call cmake ^
-DCMAKE_TOOLCHAIN_FILE=%EMSDK_PATH%/cmake/Modules/Platform/Emscripten.cmake ^
-DCMAKE_BUILD_TYPE=Release -G "Ninja" ../
popd