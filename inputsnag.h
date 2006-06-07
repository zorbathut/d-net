#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include <vector>

using namespace std;

#include <SDL.h>

#include "input.h"

class Ai;

vector<Controller> controls_init();
void controls_key(const SDL_KeyboardEvent *key);
vector<Controller> controls_next();
vector<Ai *> controls_ai();
bool controls_users();
bool controls_recordable();
void controls_shutdown();

#endif
