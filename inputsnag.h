#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"

#include <SDL.h>

#include <vector>

using namespace std;

vector<Controller> controls_init();
void controls_key(const SDL_KeyboardEvent *key);
vector<Controller> controls_next();
bool controls_users();
void controls_shutdown();

#endif
