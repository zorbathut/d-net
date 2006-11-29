
#include "audio.h"
#include "cfcommon.h"
#include "util.h"

#include <SDL.h>

int pt = 0;

void sound_callback(void *userdata, Uint8 *stream, int len) {
  Sint16 *rstream = reinterpret_cast<Sint16 *>(stream);
  len /= 2;
  for(int i = 0; i < len; i++)
    rstream[i] = (Sint16)(fsin(pt++ / 200.0) * 30000);
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
}

void deinitAudio() {
  SDL_CloseAudio();
}

