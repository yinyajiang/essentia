#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include "essentiautils.h"
#include <chrono>

#define ENDTIME 60.0
#define SAMPLE_RATE 16000 //44100

struct KeyResult {
  std::string key;
  std::string scale;
};


#if defined(_WIN32) || defined(WIN32)

#define TSTRING std::wstring
std::string t2u8(const TSTRING &ws) {
  int buffer_size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0,
                                        nullptr, nullptr);
  if (buffer_size == 0) {
    return std::string();
  }
  std::vector<char> buffer(buffer_size);
  WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, buffer.data(), buffer_size,
                      nullptr, nullptr);
  return std::string(buffer.data());
}

int wmain(int argc, wchar_t* argv[]){

#else

#define TSTRING std::string
std::string t2u8(const TSTRING &s) {
  return s;
}
int main(int argc, char* argv[]) {
  
#endif

  std::string filename = "/Volumes/extern-usb/Downloads/周杰倫 Jay Chou【青花瓷 Blue and White Porcelain】-Official Music Video.mp3";
  auto start = std::chrono::high_resolution_clock::now();


  EssentiaUtils essentiaUtils;
  size_t sizeBytes = 16000 * 60 * sizeof(float);
  std::vector<uint8_t> f32data;
  f32data.resize(sizeBytes);
  essentiaUtils.loadFile(filename.c_str(), f32data.data(), &sizeBytes, 16000, 60);

  char keybuff[10] = {0};
  char scalebuff[10] = {0};
  essentiaUtils.findKey(f32data.data(), sizeBytes, 16000, keybuff, scalebuff);
  std::cout << "*key: " << keybuff << ";" << scalebuff << std::endl;

  float bpm = essentiaUtils.findbpm(f32data.data(), sizeBytes, 16000);
  std::cout << "*bpm: " << bpm << std::endl;

  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "compute duration: " << duration.count() << "s" << std::endl;
  return 0;
}
