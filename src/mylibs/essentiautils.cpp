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

using namespace std;
using namespace essentia;
using namespace essentia::streaming;
using namespace essentia::scheduler;

#define ENDTIME 60.0
#define SAMPLE_RATE 16000

static std::atomic<int> essentia_initialized(0);


EssentiaUtils::EssentiaUtils() {
  if (essentia_initialized.fetch_add(1) == 0) {
    essentia::init();
  }
}

EssentiaUtils::~EssentiaUtils() {
  if (essentia_initialized.fetch_sub(1) == 1) {
    essentia::shutdown();
  }
}


bool EssentiaUtils::findKey(const char* filename, char* keybuff, char* scalebuff) {

  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  streaming::Algorithm* audio         = factory.create("EasyLoader",
                                            "filename", std::string(filename),
                                            "sampleRate", SAMPLE_RATE,
                                            "endTime", ENDTIME,
                                            "downmix", "mix");

  // Compute key
  Algorithm* key = factory.create("KeyExtractor",
    "frameSize", 4096,
    "hopSize", 4096, 
    "hpcpSize", 12,
    "maxFrequency", 3500.0,
    "maximumSpectralPeaks", 60,
    "minFrequency",25,
    "pcpThreshold", 0.2,
    "profileType", "bgate",
    "sampleRate", SAMPLE_RATE,
    "spectralPeaksThreshold", 0.0001,
    "tuningFrequency",440.0,
    "weightType","cosine",
    "windowType","hann"
  );
  audio->output("audio") >> key->input("audio");

  // capture Key outputs directly
  std::vector<std::string> keyOut;
  std::vector<std::string> scaleOut;
  std::vector<Real> strengthOut;
  key->output("key")                    >>  keyOut;
  key->output("scale")                  >>  scaleOut;
  key->output("strength")               >>  strengthOut;


  Network(audio).run();
  if (keyOut.empty() || scaleOut.empty()) {
    return false;
  }

  auto outkey = keyOut.empty() ? std::string() : keyOut.back();
  auto outscale = scaleOut.empty() ? std::string() : scaleOut.back();
  memcpy(keybuff, outkey.c_str(), outkey.size()+1);
  memcpy(scalebuff, outscale.c_str(), outscale.size()+1);
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

  if (sampleRate != SAMPLE_RATE) {
    essentia::standard::Algorithm* resample = factory.create("Resample",
      "inputSampleRate", static_cast<Real>(sampleRate),
      "outputSampleRate", static_cast<Real>(SAMPLE_RATE),
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
    "sampleRate", SAMPLE_RATE,
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
  int offset = 1024;
  int maxCount = 10;
  int cnt = 0;
  while (bpm < 50 && cnt < maxCount) {
    bpm = _findbpm(f32data, sizeBytes, sampleRate, offset);
    offset += 1024;
    cnt++;
  }
  return bpm;
}


float EssentiaUtils::_findbpm(const uint8_t *f32data, size_t sizeBytes, size_t sampleRate, int offset) {
  const size_t numSamples = sizeBytes / sizeof(float);
  const float* f32 = reinterpret_cast<const float*>(f32data);
  std::vector<Real> audio(numSamples);
  for (size_t i = 0; i < numSamples; ++i) {
    audio[i] = static_cast<Real>(f32[i]);
  }

  essentia::standard::AlgorithmFactory& factory = essentia::standard::AlgorithmFactory::instance();

  if (sampleRate != SAMPLE_RATE) {
    essentia::standard::Algorithm* resample = factory.create("Resample",
      "inputSampleRate", static_cast<Real>(sampleRate),
      "outputSampleRate", static_cast<Real>(SAMPLE_RATE),
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
    "sampleRate", SAMPLE_RATE
  );

  Real outBpm;
  key->input("audio").set(audio);
  key->output("bpmEstimate").set(outBpm);
  key->compute();
  return float(outBpm);
}







