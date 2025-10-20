#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/vectoroutput.h>
#include <chrono>
#include "essentiautils.h"
#include <cstring>
#include <atomic>
#include <cmath>

using namespace std;
using namespace essentia;
using namespace essentia::streaming;
using namespace essentia::scheduler;


static std::atomic<int> essentia_initialized(0);


EssentiaUtils::EssentiaUtils() {
  if (essentia_initialized.fetch_add(1) == 0) {
    essentia::init();
  }
}

EssentiaUtils::~EssentiaUtils() {
  // if (essentia_initialized.fetch_sub(1) == 1) {
  //   essentia::shutdown();
  // }
}

bool EssentiaUtils::loadFile(const char *filename, uint8_t *f32data,
                             size_t *sizeBytes, size_t sampleRate, int duration) {
    if (!filename || duration <= 0 || sizeBytes == nullptr) {
      return false;
    }

    streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

    streaming::Algorithm* audio = factory.create(
      "EasyLoader",
      "filename", std::string(filename),
      "sampleRate", int(sampleRate),
      "endTime", static_cast<Real>(duration),
      "downmix", "mix"
    );

    std::vector<Real> audioBuffer;
    audio->output("audio") >> audioBuffer;
    Network(audio).run();

    const size_t numSamples  = audioBuffer.size();
    const size_t bytesNeeded = numSamples * sizeof(float);

    if (f32data == nullptr) {
      *sizeBytes = bytesNeeded;
      return true;
    }

    if (*sizeBytes < bytesNeeded) {
      return false;
    }

    float* out = reinterpret_cast<float*>(f32data);
    for (size_t i = 0; i < numSamples; ++i) {
      out[i] = static_cast<float>(audioBuffer[i]);
    }
    *sizeBytes = numSamples * sizeof(float);
    return true;
}



bool EssentiaUtils::findKey(const uint8_t* f32data, size_t sizeBytes, size_t sampleRate, char* keybuff, char* scalebuff) {

  const size_t numSamples = sizeBytes / sizeof(float);
  const float* f32 = reinterpret_cast<const float*>(f32data);
  std::vector<Real> audio(numSamples);
  for (size_t i = 0; i < numSamples; ++i) {
    audio[i] = static_cast<Real>(f32[i]);
  }

  essentia::standard::AlgorithmFactory& factory = essentia::standard::AlgorithmFactory::instance();

  if (sampleRate != 16000) {
    essentia::standard::Algorithm* resample = factory.create("Resample",
      "inputSampleRate", static_cast<Real>(sampleRate),
      "outputSampleRate", static_cast<Real>(16000),
      "quality", 1   // 0=最佳质量, 4=最快。1 通常够用
    );

    std::vector<Real> audioResampled;
    resample->input("signal").set(audio);
    resample->output("signal").set(audioResampled);
    resample->compute();
    delete resample;
    audio = audioResampled;
  }


  // Compute key
  essentia::standard::Algorithm* key = factory.create("KeyExtractor",
    "frameSize", 4096,
    "hopSize", 4096, 
    "hpcpSize", 12,
    "maxFrequency", 3500.0,
    "maximumSpectralPeaks", 60,
    "minFrequency",25,
    "pcpThreshold", 0.2,
    "profileType", "bgate",
    "sampleRate", 16000,
    "spectralPeaksThreshold", 0.0001,
    "tuningFrequency",440.0,
    "weightType","cosine",
    "windowType","hann"
  );

  // capture Key outputs directly (standard API uses .set())
  std::string outKey;
  std::string outScale;
  Real outStrength;
  key->input("audio").set(audio);
  key->output("key").set(outKey);
  key->output("scale").set(outScale);
  key->output("strength").set(outStrength);
  key->compute();
  if (outKey.empty() || outScale.empty()) {
    return false;
  }
  memcpy(keybuff, outKey.c_str(), outKey.size()+1);
  memcpy(scalebuff, outScale.c_str(), outScale.size()+1);
  return true;
}

float EssentiaUtils::findbpm(const uint8_t *f32data, size_t sizeBytes, size_t sampleRate) {
  float bpm = 0.0;
  int offset = 2048;
  int maxCount = 10;
  int cnt = 0;
  while (bpm < 50 && cnt < maxCount) {
    bpm = _findbpm(f32data, sizeBytes, sampleRate, offset);
    offset += 1024;
    cnt++;
  }
  return (float)((int)(bpm+0.5));
}


float EssentiaUtils::_findbpm(const uint8_t *f32data, size_t sizeBytes, size_t sampleRate, int offset) {
  const size_t numSamples = sizeBytes / sizeof(float);
  const float* f32 = reinterpret_cast<const float*>(f32data);
  std::vector<Real> audio(numSamples);
  for (size_t i = 0; i < numSamples; ++i) {
    audio[i] = static_cast<Real>(f32[i]);
  }

  essentia::standard::AlgorithmFactory& factory = essentia::standard::AlgorithmFactory::instance();

  if (sampleRate != 16000) {
    essentia::standard::Algorithm* resample = factory.create("Resample",
      "inputSampleRate", static_cast<Real>(sampleRate),
      "outputSampleRate", static_cast<Real>(16000),
      "quality", 1   // 0=最佳质量, 4=最快。1 通常够用
    );

    std::vector<Real> audioResampled;
    resample->input("signal").set(audio);
    resample->output("signal").set(audioResampled);
    resample->compute();
    delete resample;
    audio = audioResampled;
  }


  // Compute key
  essentia::standard::Algorithm* key = factory.create("PercivalBpmEstimator",
    "frameSize", offset,
    "frameSizeOSS", 2 * offset, 
    "hopSize", 128,
    "hopSizeOSS", 128,
    "maxBPM", 210,
    "minBPM", 50,
    "sampleRate", 16000
  );

  Real outBpm;
  key->input("signal").set(audio);
  key->output("bpm").set(outBpm);
  key->compute();
  return float(outBpm);
}







