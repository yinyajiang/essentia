#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/vectoroutput.h>
#include <essentia/streaming/algorithms/devnull.h>
#include <chrono>
using namespace std;
using namespace essentia;
using namespace essentia::streaming;
using namespace essentia::scheduler;

#define ENDTIME 60.0
#define SAMPLE_RATE 11025 //44100
#define SpectralPeaksMaxFrequency 500.0 //5000.0
#define SpectralPeaksMaxPeaks 30 //10000
#define SpectralPeaksMagnitudeThreshold 1e-3 //1e-5
#define WINDOW_TYPE "blackmanharris62" //blackmanharris62
#define FRAMESIZE 4096 //4096
#define HOPSIZE 4096 //2048
#define HPCP_SIZE 24 //36
#define HPCP_WINDOWSIZE 1.0/1.0 //4./3.
#define KEY_numHarmonics 1 //4
#define KEY_usePolyphony false //true
#define KEY_useThreeChords false //true
#define Key_pcpSize HPCP_SIZE //36
//#define USE_TUNING_FREQUENCY
//#define USE_REPLAY_GAIN 


Real ReplayGain(const string& filename) {
#ifndef USE_REPLAY_GAIN
  return 0;
#else
  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio       = factory.create("EqloudLoader",
                                          "filename", filename,
                                          "sampleRate", SAMPLE_RATE,
                                          "endTime", ENDTIME,
                                          "downmix", "mix");

  Algorithm* replay_gain = factory.create("ReplayGain", "applyEqloud", false);

  audio->output("audio")             >>  replay_gain->input("signal");

  // capture replayGain directly
  std::vector<Real> replayGainValues;
  replay_gain->output("replayGain")  >>  replayGainValues;

  Network(audio).run();

  return replayGainValues.empty() ? 0.0 : replayGainValues.back();
#endif
}

Real TuningFrequency(const string& filename,
                     int framesize, int hopsize, int zeropadding,
                     Real rgain) {
#ifndef USE_TUNING_FREQUENCY
  return 440;
#else
  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio         = factory.create("EasyLoader",
                                            "filename", filename,
                                            "sampleRate", SAMPLE_RATE,
#ifdef USE_REPLAY_GAIN
                                            "replayGain", rgain,
#endif
                                            "endTime", ENDTIME,
                                            "downmix", "mix");

  Algorithm* frameCutter   = factory.create("FrameCutter",
                                            "frameSize", framesize,
                                            "hopSize", hopsize,
                                            "silentFrames", "noise",
                                            "startFromZero", false);

  Algorithm* window        = factory.create("Windowing", 
                                            "type", WINDOW_TYPE,
                                            "zeroPadding", zeropadding);

  Algorithm* spectrum      = factory.create("Spectrum");

  Algorithm* spectralPeaks = factory.create("SpectralPeaks",
                                            "sampleRate", SAMPLE_RATE,
                                            "maxPeaks", SpectralPeaksMaxPeaks,
                                            "maxFrequency", SpectralPeaksMaxFrequency,
                                            "minFrequency", 40.,
                                            "magnitudeThreshold", SpectralPeaksMagnitudeThreshold,
                                            "orderBy", "magnitude");

  Algorithm* tuningFreq    = factory.create("TuningFrequency", "resolution", 1.0);

  // make connectinons:
  audio->output("audio")                 >>  frameCutter->input("signal");
  frameCutter->output("frame")           >>  window->input("frame");
  window->output("frame")                >>  spectrum->input("frame");
  spectrum->output("spectrum")           >>  spectralPeaks->input("spectrum");
  spectralPeaks->output("frequencies")   >>  tuningFreq->input("frequencies");
  spectralPeaks->output("magnitudes")    >>  tuningFreq->input("magnitudes");

  // capture outputs directly
  std::vector<Real> tuningFrequencyValues;
  tuningFreq->output("tuningFrequency")  >>  tuningFrequencyValues;
  // avoid unconnected output warning for tuningCents
  tuningFreq->output("tuningCents")      >>  DEVNULL;

  Network(audio).run();

  return tuningFrequencyValues.empty() ? 440 : mean(tuningFrequencyValues);
#endif
}

struct KeyResult {
  std::string key;
  std::string scale;
  Real strength;
};

KeyResult TonalDescriptors(const string& filename,
                           int framesize, int hopsize, int zeropadding,
                           Real rgain, Real tuningFrequency) {

  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio         = factory.create("EasyLoader",
                                            "filename", filename,
                                            "sampleRate", SAMPLE_RATE,
#ifdef USE_REPLAY_GAIN
                                            "replayGain", rgain,
#endif
                                            "endTime", ENDTIME,
                                            "downmix", "mix");

  Algorithm* frameCutter   = factory.create("FrameCutter",
                                            "frameSize", framesize,
                                            "hopSize", hopsize,
                                            "silentFrames", "noise",
                                            "startFromZero", false);

  Algorithm* window        = factory.create("Windowing", 
                                            "type", WINDOW_TYPE,
                                            "zeroPadding", zeropadding);

  Algorithm* spectrum      = factory.create("Spectrum");

  Algorithm* spectralPeaks = factory.create("SpectralPeaks",
                                            "sampleRate", SAMPLE_RATE,
                                            "maxPeaks", SpectralPeaksMaxPeaks,
                                            "maxFrequency", SpectralPeaksMaxFrequency,
                                            "minFrequency", 40,
                                            "magnitudeThreshold", SpectralPeaksMagnitudeThreshold,
                                            "orderBy", "frequency");

  Algorithm* key           = factory.create("Key",
                                            "numHarmonics", KEY_numHarmonics,
                                            "pcpSize", Key_pcpSize,
                                            "profileType", "temperley",
                                            "slope", 0.6,
                                            "usePolyphony", KEY_usePolyphony,
                                            "useThreeChords", KEY_useThreeChords);

  Algorithm* hpcp          = factory.create("HPCP",
                                            "size", HPCP_SIZE,
                                            "referenceFrequency", tuningFrequency,
                                            "bandPreset", false,
                                            "minFrequency", 40.,
                                            "maxFrequency", 5000.,
                                            "weightType", "squaredCosine",
                                            "nonLinear", false,
                                            "windowSize", HPCP_WINDOWSIZE,
                                            "sampleRate", SAMPLE_RATE);

  // make connectinons:
  audio->output("audio")                >>  frameCutter->input("signal");
  frameCutter->output("frame")          >>  window->input("frame");
  window->output("frame")               >>  spectrum->input("frame");
  spectrum->output("spectrum")          >>  spectralPeaks->input("spectrum");
  spectralPeaks->output("frequencies")  >>  hpcp->input("frequencies");
  spectralPeaks->output("magnitudes")   >>  hpcp->input("magnitudes");
  hpcp->output("hpcp")                  >>  key->input("pcp");

  // capture Key outputs directly
  std::vector<std::string> keyOut;
  std::vector<std::string> scaleOut;
  std::vector<Real> strengthOut;
  key->output("key")                    >>  keyOut;
  key->output("scale")                  >>  scaleOut;
  key->output("strength")               >>  strengthOut;

  Network(audio).run();

  KeyResult result;
  result.key = keyOut.empty() ? std::string() : keyOut.back();
  result.scale = scaleOut.empty() ? std::string() : scaleOut.back();
  result.strength = strengthOut.empty() ? 0.0 : strengthOut.back();
  return result;
}

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

  if (argc != 2) {
    cout << "Error: wrong number of arguments" << endl;
    cout << "Usage: " << argv[0] << " input_audiofile" << endl;  
    exit(1);
  }

  string filename = t2u8(argv[1]);

  // Parameters
  uint framesize = FRAMESIZE;
  uint hopsize = HOPSIZE;
  uint zeropadding = 0;

  auto start = chrono::high_resolution_clock::now();

  essentia::init();
  // Compute replay gain
  Real rgain = ReplayGain(filename);
  cout << "replay gain: " << rgain << endl;

  // Compute tuning frequency
  Real tuningFrequency = TuningFrequency(filename, framesize, hopsize, zeropadding, rgain);
  cout << "tuning frequency: " << tuningFrequency << endl;

  // Compute key
  KeyResult keyResult = TonalDescriptors(filename, framesize, hopsize, zeropadding,  rgain, tuningFrequency);
  
  cout << "*key: " << keyResult.key << ";" << keyResult.scale << endl;

  essentia::shutdown();
  auto end = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::seconds>(end - start);
  cout << "compute duration: " << duration.count() << "s" << endl;
  return 0;
}
