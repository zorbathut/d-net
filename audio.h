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

Sound loadSound(const string &filename);

void queueSound(const Sound &sound, float volume);

/*************
 * Global sounds
 */

namespace S {
  extern Sound cancel;
  extern Sound confirm;
  extern Sound error;
  extern Sound select;
};

#endif
