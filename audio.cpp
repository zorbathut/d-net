
#include "audio.h"
#include "itemdb.h"
#include "os_sdl.h"

#include <vorbis/vorbisfile.h>
using namespace std;



DEFINE_bool(audio, true, "Turn sound on or off entirely");

class SoundState {
public:
  const Sound *sound;
  float shiftl, shiftr;
  float tone;
  float sample;
};

vector<SoundState> sstv;

template<float (SoundState::*pt)> Sint16 mixsample(int channel) {
  int mix = 0;
  for(int i = 0; i < sstv.size(); i++)
    mix += (int)(sstv[i].sound->data[channel][(int)sstv[i].sample] * sstv[i].*pt);
  return clamp(mix, -32767, 32767);
} // whee megaslow

void sound_callback(void *userdata, Uint8 *stream, int len) {
  Sint16 *rstream = reinterpret_cast<Sint16 *>(stream);
  CHECK(len % 4 == 0);
  len /= 4;
  for(int i = 0; i < len; i++) {
    *rstream++ = mixsample<&SoundState::shiftl>(0);
    *rstream++ = mixsample<&SoundState::shiftr>(1);
    for(int i = 0; i < sstv.size(); i++) {
      sstv[i].sample += sstv[i].tone;
      if(sstv[i].sample >= sstv[i].sound->data[0].size()) {
        sstv.erase(sstv.begin() + i);
        i--;
      }
    } // god this is inefficient
  }
}

static Sound choose_local;
static Sound accept_local;
static Sound select_local;
static Sound cursorover_local;
static Sound error_local;
static Sound null_local;

const Sound *const S::choose = &choose_local;
const Sound *const S::accept = &accept_local;
const Sound *const S::select = &select_local;
const Sound *const S::cursorover = &cursorover_local;
const Sound *const S::error = &error_local;
const Sound *const S::null = &null_local;

void initAudio() {
  StackString sst("AudioInit");
  
  if(FLAGS_audio) {
    StackString sst("SDL");
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = AUDIO_S16LSB;
    spec.channels = 2;
    spec.samples = 1024;
    spec.callback = sound_callback;
    spec.userdata = NULL;
    
    bool audioallowed = (SDL_OpenAudio(&spec, NULL) == 0);
    if(audioallowed) {
      SDL_PauseAudio(0);
    } else {
      dprintf("Audio initialization failed, disabling audio\n");
      FLAGS_audio = false;
    }
  }
  
  accept_local = loadSound(FLAGS_fileroot + "sound/accept");
  choose_local = loadSound(FLAGS_fileroot + "sound/choose");
  
  select_local = loadSound(FLAGS_fileroot + "sound/select");
  cursorover_local = loadSound(FLAGS_fileroot + "sound/cursorover");
  
  error_local = loadSound(FLAGS_fileroot + "sound/error");
  
  null_local.data[0].resize(1);
  null_local.data[1].resize(1);
}

void deinitAudio() {
  if(FLAGS_audio) {
    SDL_CloseAudio();
  }
}

static vector<float> tonemults;
static float tone = 1.0;

void reCalcTone() {
  tone = 1.0;
  for(int i = 0; i < tonemults.size(); i++)
    tone *= tonemults[i];
}

AudioToner::AudioToner(float t) {
  tonemults.push_back(t);
  reCalcTone();
}

AudioToner::~AudioToner() {
  tonemults.pop_back();
  reCalcTone();
}

static vector<pair<float, float> > shiftvols;
static float shiftl = 1.0;
static float shiftr = 1.0;

void reCalcShift() {
  shiftl = 1.0;
  shiftr = 1.0;
  for(int i = 0; i < shiftvols.size(); i++) {
    shiftl *= shiftvols[i].first;
    shiftr *= shiftvols[i].second;
  }
}

AudioShifter::AudioShifter(float l, float r) {
  shiftvols.push_back(make_pair(l, r));
  reCalcShift();
}

AudioShifter::~AudioShifter() {
  shiftvols.pop_back();
  reCalcShift();
}

DECLARE_int(fastForwardTo);

void queueSound(const Sound *sound) {
  CHECK(sound);
  CHECK(sound->data[0].size());
  CHECK(sound->data[1].size());
  if(!FLAGS_audio || FLAGS_fastForwardTo)
      return;
  SoundState stt;
  stt.sound = sound;
  stt.shiftl = shiftl;
  stt.shiftr = shiftr;
  stt.tone = tone;
  stt.sample = 0;
  SDL_LockAudio();
  sstv.push_back(stt);
  SDL_UnlockAudio();
}

Sound loadSoundOgg(const string &name) {
  StackString sst("loadSoundOgg");
  
  OggVorbis_File ov;
  CHECK(!ov_fopen((char *)(name + ".ogg").c_str(), &ov));
  
  vorbis_info *inf = ov_info(&ov, -1);
  CHECK(inf);
  CHECK(inf->channels == 2);
  CHECK(inf->rate == 44100);

  Sound foo;

  while(1) {
    char buf[4096];
    int bitstream;
    int rv = ov_read(&ov, buf, sizeof(buf), 0, 2, 1, &bitstream);
    CHECK(bitstream == 0);
    CHECK(rv >= 0);
    CHECK(rv <= sizeof(buf));
    CHECK(rv % 4 == 0);
    if(rv == 0)
      break;
    short *pt = reinterpret_cast<short*>(buf);
    rv /= 4;
    for(int i = 0; i < rv; i++) {
      foo.data[0].push_back(*pt++);
      foo.data[1].push_back(*pt++);
    }
  }
  
  CHECK(!ov_clear(&ov));

  return foo;
}

Sound loadSound(const string &name) {
  StackString sst("loadSound");
  
  CHECK(sizeof(short) == 2 && CHAR_BIT == 8);
  
  Sound sound;
  
  SDL_AudioSpec aspec;
  Uint8 *data;
  Uint32 len;
  if(!SDL_LoadWAV((name + ".wav").c_str(), &aspec, &data, &len)) {
    return loadSoundOgg(name);
  }
  
  CHECK(aspec.freq == 44100);
  CHECK(aspec.format == AUDIO_S16LSB);
  CHECK(aspec.channels = 2);
  
  Sint16 *rdata = reinterpret_cast<Sint16 *>(data);
  for(int i = 0; i < len / 4; i++) {
    sound.data[0].push_back(*rdata++);
    sound.data[1].push_back(*rdata++);
  }
  
  SDL_FreeWAV(data);

  return sound;
}
