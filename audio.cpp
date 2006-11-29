
#include "audio.h"
#include "cfcommon.h"
#include "util.h"

#include <SDL.h>

void sound_callback(void *userdata, Uint8 *stream, int len) {
  Sint16 *rstream = reinterpret_cast<Sint16 *>(stream);
  len /= 2;
  for(int i = 0; i < len; i++)
    rstream[i] = 0;
    
}

void initAudio() {
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
