mkdir build_win\Debug
mkdir build_win\Release
mklink /j build_win\Debug\res .\src
mklink /j build_win\Release\res .\src