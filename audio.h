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
  extern Sound accept;  // major choices (finish menus)
  extern Sound choose;  // minor choices (keys, buying items)
  
  extern Sound select;  // menu choices
  extern Sound cursorover;  // anything really passive
  
  extern Sound error; // errors (dur)
  
  // not really used for anything yet
  extern Sound cancel;
};

#endif
