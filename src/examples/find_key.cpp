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
using namespace std;
using namespace essentia;
using namespace essentia::streaming;
using namespace essentia::scheduler;

#define ENDTIME 60.0
#define SAMPLE_RATE 16000 //44100

struct KeyResult {
  std::string key;
  std::string scale;
  Real strength;
};




KeyResult KeyExtract(const string& filename) {

  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio         = factory.create("EasyLoader",
                                            "filename", filename,
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

  KeyResult result;
  result.key = keyOut.empty() ? std::string() : keyOut.back();
  result.scale = scaleOut.empty() ? std::string() : scaleOut.back();
  result.strength = strengthOut.empty() ? 0.0 : strengthOut.back();
  return result;
}


Real bpmExtract(const string& filename) {
  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

  Algorithm* audio         = factory.create("EasyLoader",
                                            "filename", filename,
                                            "sampleRate", SAMPLE_RATE,
                                            "endTime", ENDTIME,
                                            "downmix", "mix");
  // Loop BPM estimation
  Algorithm* percivalBPM = factory.create("PercivalBpmEstimator");
  audio->output("audio") >> percivalBPM->input("signal");

  Network(audio).run();
  std::vector<Real> bpmOut;
  percivalBPM->output("bpm") >> bpmOut;
  return bpmOut.empty() ? 0.0 : bpmOut.back();
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

  std::string filename = t2u8(argv[1]);
  auto start = chrono::high_resolution_clock::now();
  essentia::init();
  KeyResult keyResult = KeyExtract(filename);
  cout << "*key: " << keyResult.key << ";" << keyResult.scale << endl;
  essentia::shutdown();

  auto end = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::seconds>(end - start);
  cout << "compute duration: " << duration.count() << "s" << endl;
  return 0;
}
