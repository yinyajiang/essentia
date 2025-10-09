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
using namespace std;
using namespace essentia;
using namespace essentia::streaming;
using namespace essentia::scheduler;

Real ReplayGain(const string& filename) {
  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio       = factory.create("EqloudLoader",
                                          "filename", filename,
                                          "sampleRate", 44100,
                                          "downmix", "mix");

  Algorithm* replay_gain = factory.create("ReplayGain", "applyEqloud", false);

  audio->output("audio")             >>  replay_gain->input("signal");

  // capture replayGain directly
  std::vector<Real> replayGainValues;
  replay_gain->output("replayGain")  >>  replayGainValues;

  Network(audio).run();

  return replayGainValues.empty() ? 0.0 : replayGainValues.back();
}

Real TuningFrequency(const string& filename,
                     int framesize, int hopsize, int zeropadding,
                     Real rgain) {
  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio         = factory.create("EasyLoader",
                                            "filename", filename,
                                            "sampleRate", 44100,
                                            "replayGain", rgain,
                                            "endTime", 60.0,
                                            "downmix", "mix");

  Algorithm* frameCutter   = factory.create("FrameCutter",
                                            "frameSize", framesize,
                                            "hopSize", hopsize,
                                            "silentFrames", "noise",
                                            "startFromZero", false);

  Algorithm* window        = factory.create("Windowing", 
                                            "type", "blackmanharris62",
                                            "zeroPadding", zeropadding);

  Algorithm* spectrum      = factory.create("Spectrum");

  Algorithm* spectralPeaks = factory.create("SpectralPeaks",
                                            "sampleRate", 44100,
                                            "maxPeaks", 10000,
                                            "maxFrequency", 5000.,
                                            "minFrequency", 40.,
                                            "magnitudeThreshold", 0.00001,
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

  return tuningFrequencyValues.empty() ? 0.0 : mean(tuningFrequencyValues);
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
                                            "sampleRate", 44100,
                                            "replayGain", rgain,
                                            "endTime", 60.0,
                                            "downmix", "mix");

  Algorithm* frameCutter   = factory.create("FrameCutter",
                                            "frameSize", framesize,
                                            "hopSize", hopsize,
                                            "silentFrames", "noise",
                                            "startFromZero", false);

  Algorithm* window        = factory.create("Windowing", 
                                            "type", "blackmanharris62",
                                            "zeroPadding", zeropadding);

  Algorithm* spectrum      = factory.create("Spectrum");

  Algorithm* spectralPeaks = factory.create("SpectralPeaks",
                                            "sampleRate", 44100,
                                            "maxPeaks", 10000,
                                            "maxFrequency", 5000,
                                            "minFrequency", 40,
                                            "magnitudeThreshold", 0.00001,
                                            "orderBy", "frequency");

  Algorithm* key           = factory.create("Key",
                                            "numHarmonics", 4,
                                            "pcpSize", 36,
                                            "profileType", "temperley",
                                            "slope", 0.6,
                                            "usePolyphony", true,
                                            "useThreeChords", true);

  Algorithm* hpcp          = factory.create("HPCP",
                                            "size", 36,
                                            "referenceFrequency", tuningFrequency,
                                            "bandPreset", false,
                                            "minFrequency", 40.,
                                            "maxFrequency", 5000.,
                                            "weightType", "squaredCosine",
                                            "nonLinear", false,
                                            "windowSize", 4./3.,
                                            "sampleRate", 44100);

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
  uint framesize = 4096;
  uint hopsize = 2048;
  uint zeropadding = 0;

  essentia::init();

  // Compute replay gain
  Real rgain = ReplayGain(filename);

  // Compute tuning frequency
  Real tuningFrequency = TuningFrequency(filename, framesize, hopsize, zeropadding, rgain);
  cout << "tuning frequency:\t" << tuningFrequency << endl;

  // Compute key
  KeyResult keyResult = TonalDescriptors(filename, framesize, hopsize, zeropadding,  rgain, tuningFrequency);
  
  cout << "key:" << "\t" << keyResult.key 
       << "  " << keyResult.scale << endl;

  essentia::shutdown();

  return 0;
}
