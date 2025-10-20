call %~dp0\packaging\build-dependencies-msvc.bat
cd /d %~dp0
@REM cmake -B build -DCMAKE_PREFIX_PATH=%~dp0\packaging\msvc
@REM set PATH=%PATH%;%~dp0\packaging\msvc\bin
@REM cmake --build build --config release
@REM cmake --install build --config release --prefix "%~dp0\build\installed"

cmake --preset windows-preset
cmake --build --preset build-windows --config Release
pause