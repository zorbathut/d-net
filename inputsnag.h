#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"
#include "ai.h"

#include <SDL.h>

#include <vector>

using namespace std;

vector<Controller> controls_init();
void controls_key(const SDL_KeyboardEvent *key);
vector<Controller> controls_next();
vector<Ai *> controls_ai();
bool controls_users();
void controls_shutdown();

#endif
