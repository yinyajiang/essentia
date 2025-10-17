call %~dp0\packaging\build-dependencies-msvc.bat
cd /d %~dp0
cmake -B build -DCMAKE_PREFIX_PATH=%~dp0\packaging\msvc
set PATH=%PATH%;%~dp0\packaging\msvc\bin
cmake --build build --config release
cmake --install build --config release --prefix "%~dp0\build\installed"
pause