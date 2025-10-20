#pragma once
#include <cstddef>

#ifdef _WIN32
    #ifdef EXPORTS_ESSENTIAUTILS
        #define EXPORTS_ESSENTIAUTILS_API __declspec(dllexport)
    #else
        #define EXPORTS_ESSENTIAUTILS_API __declspec(dllimport)
    #endif
#else
    #define EXPORTS_ESSENTIAUTILS_API
#endif


class EXPORTS_ESSENTIAUTILS_API EssentiaUtils {
public:
    EssentiaUtils();
    ~EssentiaUtils();
    bool findKey(const char* u8filename, char* keybuff, char* scalebuff);
    bool findKey(const uint8_t *f32data, size_t sizeBytes, size_t sampleRate,
                 char *keybuff, char *scalebuff);
    float findbpm(const uint8_t *f32data, size_t sizeBytes, size_t sampleRate);

  private:
    float _findbpm(const uint8_t *f32data, size_t sizeBytes, size_t sampleRate, int offset);
};


  
  


