#ifndef DNET_AUDIO
#define DNET_AUDIO

#include <string>
#include <vector>

using namespace std;

/*************
 * Sound class
 */
 
class Sound {
public:
  vector<short> data[2];
};

/*************
 * API
 */

void initAudio();
void deinitAudio();

void queueSound(const Sound &sound, float volume);

Sound loadSound(const string &filename);

/*************
 * Global sounds
 */

namespace S {
  extern Sound cancel;
  extern Sound accept;
  extern Sound error;
  extern Sound select;
};

#endif
