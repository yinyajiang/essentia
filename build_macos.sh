
export PKG_CONFIG_PATH=/Volumes/extern-usb/MMProject/vocalremover/sondkits/3rd/ffmpeg-mac/lib/pkgconfig
# brew install eigen libyaml fftw ffmpeg libsamplerate libtag chromaprint
# brew install eigen libsamplerate
cmake -B build -D FFTW3f_DIR="$(pwd)/packaging/macos/fftw3f/lib/cmake/fftw3f" -D Eigen3_DIR="$(pwd)/packaging/macos/eigen3/share/eigen3/cmake/Eigen3" -D SampleRate_DIR="$(pwd)/packaging/macos/libsamplerate/lib/cmake/SampleRate"
cmake --build build --config release 
cmake --install build --config release --prefix "$(pwd)/build/installed"