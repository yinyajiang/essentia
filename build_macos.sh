
# brew install eigen libyaml fftw ffmpeg libsamplerate libtag chromaprint
export MACOSX_DEPLOYMENT_TARGET=11.0
brew install eigen
if [ "$(uname -m)" == "arm64" ]; then
    echo "Building for macOS arm64"
    cmake --preset macOS-arm-preset
    cmake --build --preset build-macOS-arm --config release
else
    echo "Building for macOS x86_64"
    cmake --preset macOS-preset
    cmake --build --preset build-macOS --config release
fi
