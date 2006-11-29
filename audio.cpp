
#include "audio.h"
#include "cfcommon.h"
#include "util.h"

#include <SDL.h>

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

void initAudio() {
  CHECK(SDL_AudioInit("dsound") == 0);
  
  SDL_AudioSpec spec;
  spec.freq = 44100;
  spec.format = AUDIO_S16LSB;
  spec.channels = 2;
  spec.samples = 2048;
  spec.callback = sound_callback;
  spec.userdata = NULL;
  
  CHECK(SDL_OpenAudio(&spec, NULL) == 0);
  
  SDL_PauseAudio(0);
  
  S::cancel = loadSound("data/sound/cancel");
  S::confirm = loadSound("data/sound/confirm");
  S::error = loadSound("data/sound/error");
  S::select = loadSound("data/sound/select");
}

void deinitAudio() {
  SDL_CloseAudio();
  SDL_AudioQuit();
}

void queueSound(const Sound &sound, float volume) {
  SoundState stt;
  stt.sound = &sound;
  stt.volume = volume;
  stt.sample = 0;
  SDL_LockAudio();
  sstv.push_back(stt);
  SDL_UnlockAudio();
}

Sound loadSound(const string &name) {
  CHECK(sizeof(short) == 2 && CHAR_BIT == 8);
  
  Sound sound;
  
  SDL_AudioSpec aspec;
  Uint8 *data;
  Uint32 len;
  CHECK(SDL_LoadWAV((name + ".wav").c_str(), &aspec, &data, &len));
  
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

Sound S::cancel;
Sound S::confirm;
Sound S::error;
Sound S::select;
