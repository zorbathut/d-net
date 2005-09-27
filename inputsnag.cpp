
#include "inputsnag.h"
#include "args.h"
#include "rng.h"

#include <utility>

using namespace std;

DEFINE_string( readTarget, "", "File to replay from" );

enum { CIP_KEYBOARD, CIP_JOYSTICK, CIP_AI, CIP_PRERECORD };

static vector<pair<int, int> > sources;
static vector<SDL_Joystick *> joysticks;
static FILE *infile;

static vector<Controller> last;
static vector<Controller> now;

int playerone[] = { SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_u, SDLK_i, SDLK_o, SDLK_p, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH };
int playertwo[] = { SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_r, SDLK_t, SDLK_y, SDLK_f, SDLK_g, SDLK_h, SDLK_v, SDLK_b, SDLK_n };

int *baseplayermap[2] = { playerone, playertwo };
int baseplayersize[2] = { sizeof(playerone) / sizeof(*playerone), sizeof(playertwo) / sizeof(*playertwo) };    

vector<Controller> controls_init() {
    CHECK(sources.size() == 0);
    if(FLAGS_readTarget != "") {
        dprintf("Reading state record from file %s\n", FLAGS_readTarget.c_str());
        infile = fopen(FLAGS_readTarget.c_str(), "rb");
        CHECK(infile);
        int dat;
        fread(&dat, 1, sizeof(dat), infile);
        CHECK(dat == 2);
        fread(&dat, 1, sizeof(dat), infile);
        sfrand(dat);
        fread(&dat, 1, sizeof(dat), infile);
        dprintf("%d controllers\n", dat);
        now.resize(dat);
        for(int i = 0; i < now.size(); i++) {
            fread(&dat, 1, sizeof(dat), infile);
            dprintf("%d: %d buttons\n", i, dat);
            now[i].keys.resize(dat);
            sources.push_back(make_pair((int)CIP_PRERECORD, i));
        }
    } else {
        // Keyboard init
        sources.push_back(make_pair((int)CIP_KEYBOARD, 0));
        sources.push_back(make_pair((int)CIP_KEYBOARD, 1));
        
        now.resize(2);
        now[0].keys.resize(baseplayersize[0]);
        now[1].keys.resize(baseplayersize[1]);
        
        // Joystick init
        printf("%d joysticks detected\n", SDL_NumJoysticks());
        
        now.resize(2 + SDL_NumJoysticks());
    
        for(int i = 0; i < SDL_NumJoysticks(); i++) {
            dprintf("Opening %d: %s\n", i, SDL_JoystickName(i));
            joysticks.push_back(SDL_JoystickOpen(i));
            CHECK(SDL_JoystickNumAxes(joysticks.back()) >= 2);
            CHECK(SDL_JoystickNumButtons(joysticks.back()) >= 1);
            dprintf("%d axes, %d buttons\n", SDL_JoystickNumAxes(joysticks.back()), SDL_JoystickNumButtons(joysticks.back()));
        }
        
        dprintf("Done opening joysticks\n");
        
        for(int i = 0; i < joysticks.size(); i++) {
            sources.push_back(make_pair((int)CIP_JOYSTICK, i));
            now[i + 2].keys.resize(SDL_JoystickNumButtons(joysticks[i]));
        }
    }
    CHECK(sources.size() != 0);
    CHECK(sources.size() == now.size());
    last = now;
    return now;
}

void controls_key(const SDL_KeyboardEvent *key) {
    bool *ps = NULL;
    for(int i = 0; i < sizeof(baseplayermap) / sizeof(*baseplayermap); i++) {
        for(int j = 0; j < baseplayersize[i]; j++) {
            if(key->keysym.sym == baseplayermap[i][j]) {
                if(j == 0)
                    ps = &now[i].u.down;
                else if(j == 1)
                    ps = &now[i].d.down;
                else if(j == 2)
                    ps = &now[i].l.down;
                else if(j == 3)
                    ps = &now[i].r.down;
                else
                    ps = &now[i].keys[j-4].down;
            }
        }
    }
    if( !ps )
		return;
	if( key->type == SDL_KEYUP )
		*ps = 0;
	if( key->type == SDL_KEYDOWN )
		*ps = 1;
}

vector<Controller> controls_next() {
    
    if(infile) {
        for(int i = 0; i < now.size(); i++) {
            fread(&now[i].x, 1, sizeof(now[i].x), infile);
            fread(&now[i].y, 1, sizeof(now[i].y), infile);
            fread(&now[i].u.down, 1, sizeof(now[i].u.down), infile);
            fread(&now[i].d.down, 1, sizeof(now[i].d.down), infile);
            fread(&now[i].l.down, 1, sizeof(now[i].l.down), infile);
            fread(&now[i].r.down, 1, sizeof(now[i].r.down), infile);
            for(int j = 0; j < now[i].keys.size(); j++)
                fread(&now[i].keys[j].down, 1, sizeof(now[i].keys[j].down), infile);
        }
        CHECK(!feof(infile));
        int cpos = ftell(infile);
        int x;
        fread(&x, 1, sizeof(x), infile);
        if(feof(infile))
            dprintf("EOF on frame %d\n", frameNumber);
        fseek(infile, cpos, SEEK_SET);
        CHECK(cpos == ftell(infile));
    } else {
        SDL_JoystickUpdate();
        for(int i = 0; i < now.size(); i++) {
            if(sources[i].first == CIP_JOYSTICK) {
                int jstarget = sources[i].second;
                now[i].x = SDL_JoystickGetAxis(joysticks[jstarget], 0) / 32768.0f;
                now[i].y = -(SDL_JoystickGetAxis(joysticks[jstarget], 1) / 32768.0f);
                for(int j = 0; j < now[i].keys.size(); j++)
                    now[i].keys[j].down = SDL_JoystickGetButton(joysticks[jstarget], j);
            }
        }
        
        // Now we update the parts that have to be implied
        
        for(int i = 0; i < now.size(); i++) {
            if(sources[i].first == CIP_KEYBOARD) {
                now[i].x = now[i].r.down - now[i].l.down;
                now[i].y = now[i].u.down - now[i].d.down;
            } else if(sources[i].first == CIP_JOYSTICK) {
                if(now[i].x < -0.7) {
                    now[i].r.down = false;
                    now[i].l.down = true;
                } else if(now[i].x > 0.7) {
                    now[i].r.down = true;
                    now[i].l.down = false;
                } else {
                    now[i].r.down = false;
                    now[i].l.down = false;
                }
                if(now[i].y < -0.7) {
                    now[i].u.down = false;
                    now[i].d.down = true;
                } else if(now[i].y > 0.7) {
                    now[i].u.down = true;
                    now[i].d.down = false;
                } else {
                    now[i].u.down = false;
                    now[i].d.down = false;
                }
            } else {
                CHECK(0);
            }
        }
    }

    // Now we do the deltas
    
    for(int i = 0; i < now.size(); i++) {
        last[i].newState(now[i]);
    }
    
    return last;
    
}

bool controls_users() {
    return !infile;
}

void controls_shutdown() {
    for(int i = 0; i < joysticks.size(); i++)
        SDL_JoystickClose(joysticks[i]);
    fclose(infile);
}
