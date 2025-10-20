
# brew install eigen libyaml fftw ffmpeg libsamplerate libtag chromaprint
# brew install eigen libsamplerate
cmake --preset macOS-preset
cmake --build --preset build-macOS --config release
