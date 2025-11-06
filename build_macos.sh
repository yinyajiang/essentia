
# brew install eigen libyaml fftw ffmpeg libsamplerate libtag chromaprint
# brew install eigen libsamplerate
export MACOSX_DEPLOYMENT_TARGET=11.0
if [ "$(uname -m)" == "arm64" ]; then
    cmake --preset macOS-arm-preset
    cmake --build --preset build-macOS-arm --config release
else
    cmake --preset macOS-preset
    cmake --build --preset build-macOS --config release
fi
