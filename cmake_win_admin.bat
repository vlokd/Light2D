@echo off
mkdir build_win
pushd build_win
call cmake -G "Visual Studio 16 2019" ../
popd
call init_symlinks_admin.bat