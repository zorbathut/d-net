#ifndef DNET_AUDIO
#define DNET_AUDIO


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

void queueSound(const Sound *sound);

Sound loadSound(const string &filename);

/*************
 * Modifiers
 */

class AudioToner {
public:
  AudioToner(float shift);
  ~AudioToner();
};

class AudioShifter {
public:
  AudioShifter(float l, float r);
  ~AudioShifter();
};

/*************
 * Global sounds
 */

namespace S {
  extern const Sound *const accept;  // major choices (finish menus)
  extern const Sound *const choose;  // minor choices (keys, buying items)
  
  extern const Sound *const select;  // menu choices
  extern const Sound *const cursorover;  // anything really passive
  
  extern const Sound *const error; // errors (dur)
  
  extern const Sound *const null; // the null sound
};

#endif
