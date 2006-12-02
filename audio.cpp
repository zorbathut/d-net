
#include "audio.h"
#include "cfcommon.h"
#include "debug.h"
#include "util.h"

#include <SDL.h>
#include <vorbis/vorbisfile.h>

class SoundState {
public:
  const Sound *sound;
  float volume;
  int sample;
};

vector<SoundState> sstv;

Sint16 mixsample(int channel) {
  int mix = 0;
  for(int i = 0; i < sstv.size(); i++)
    mix += (int)(sstv[i].sound->data[channel][sstv[i].sample] * sstv[i].volume);
  return clamp(mix, -32767, 32767);
} // whee megaslow

void sound_callback(void *userdata, Uint8 *stream, int len) {
  Sint16 *rstream = reinterpret_cast<Sint16 *>(stream);
  CHECK(len % 4 == 0);
  len /= 4;
  for(int i = 0; i < len; i++) {
    *rstream++ = mixsample(0);
    *rstream++ = mixsample(1);
    for(int i = 0; i < sstv.size(); i++) {
      sstv[i].sample++;
      if(sstv[i].sample == sstv[i].sound->data[0].size()) {
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
  SDL_AudioSpec spec;
  spec.freq = 44100;
  spec.format = AUDIO_S16LSB;
  spec.channels = 2;
  spec.samples = 1024;
  spec.callback = sound_callback;
  spec.userdata = NULL;
  
  CHECK(SDL_OpenAudio(&spec, NULL) == 0);
  
  SDL_PauseAudio(0);
  
  accept_local = loadSound("data/sound/accept");
  choose_local = loadSound("data/sound/choose");
  
  select_local = loadSound("data/sound/select");
  cursorover_local = loadSound("data/sound/cursorover");
  
  error_local = loadSound("data/sound/error");
  
  null_local.data[0].resize(1);
  null_local.data[1].resize(1);
}

void deinitAudio() {
  SDL_CloseAudio();
}

void queueSound(const Sound *sound, float volume) {
  CHECK(sound);
  CHECK(sound->data[0].size());
  CHECK(sound->data[1].size());
  SoundState stt;
  stt.sound = sound;
  stt.volume = volume;
  stt.sample = 0;
  SDL_LockAudio();
  sstv.push_back(stt);
  SDL_UnlockAudio();
}

Sound loadSoundOgg(const string &name) {
  FILE *file = fopen((name + ".ogg").c_str(), "rb");
  CHECK(file);
  
  OggVorbis_File ov;
  CHECK(!ov_open(file, &ov, NULL, 0));
  
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
