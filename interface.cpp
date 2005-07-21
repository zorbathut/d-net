
#include "interface.h"
#include "game.h"
#include "gfx.h"

#include <string>
#include <vector>

using namespace std;

class StdMenu {
    
    vector<pair<string, int> > items;
    int cpos;
    
public:

    void pushMenuItem(const string &name, int triggeraction);

    int tick(const Keystates &keys);
    void render() const;

    StdMenu();

};

void StdMenu::pushMenuItem(const string &name, int triggeraction) {
    items.push_back(make_pair(name, triggeraction));
}

int StdMenu::tick(const Keystates &keys) {
    if(keys.firing)
        return items[cpos].second;
    if(keys.forward)
        cpos++;
    if(keys.back)
        cpos--;
    cpos += items.size();
    cpos %= items.size();
    return -1;
}

void StdMenu::render() const {
    setZoom(0, 0, 100);
    for(int i = 0; i < items.size(); i++) {
        if(i == cpos) {
            setColor(1.0, 1.0, 1.0);
        } else {
            setColor(0.5, 0.5, 0.5);
        }
        drawText(items[i].first.c_str(), 5, 2, 2 + 6 * i);
    }
}

StdMenu::StdMenu() {
    cpos = 0;
}

class InterfaceMain {
    
    enum { IFM_S_MAINMENU, IFM_S_PLAYING };
    enum { IFM_M_NEWGAME, IFM_M_EXIT };
    int interface_mode;
    
    Game game;
    
    StdMenu mainmenu;
    
public:

    bool tick(const vector< Controller > &control, const Keystates &keyb);
    void render() const;
    InterfaceMain();

};

vector< Keystates > ctk(const vector< Controller > &cont) {
    vector<Keystates> out(cont.size());
    for(int i = 0; i < cont.size(); i++) {
        if(cont[i].x < -.5)
            out[i].left = 1;
        if(cont[i].x > .5)
            out[i].right = 1;
        if(cont[i].y < -.5)
            out[i].back = 1;
        if(cont[i].y > .5)
            out[i].forward = 1;
        if(cont[i].keys[0])
            out[i].firing = 1;
    }
    return out;
}

bool InterfaceMain::tick(const vector< Controller > &control, const Keystates &keyb) {
          
    if(interface_mode == IFM_S_MAINMENU) {
        int mrv;
        mrv = mainmenu.tick(keyb);
        if(mrv == IFM_M_NEWGAME) {
            game = Game();
            interface_mode = IFM_S_PLAYING;
        } else if(mrv == IFM_M_EXIT) {
            return true;
        } else {
            assert(mrv == -1);
        }
    } else if(interface_mode == IFM_S_PLAYING) {
        if(game.runTick(ctk(control))) {
            interface_mode = IFM_S_MAINMENU;
        }
    } else {
        assert(0);
    }
    
    return false;
    
}

void InterfaceMain::render() const {
    
    if(interface_mode == IFM_S_MAINMENU) {
        mainmenu.render();
    } else if(interface_mode == IFM_S_PLAYING) {
        game.renderToScreen(RENDERTARGET_SPECTATOR);
        setColor(1.0, 1.0, 1.0);
        drawText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 5, 0, 0);
        drawText("the quick brown fox jumped over the lazy dog", 5, 0, 6);
    } else {
        assert(0);
    }
    
};

InterfaceMain::InterfaceMain() {
    interface_mode = IFM_S_MAINMENU;
    mainmenu.pushMenuItem("New game", IFM_M_NEWGAME);
    mainmenu.pushMenuItem("Exit", IFM_M_EXIT);
}

InterfaceMain ifm;

Game game;

void interfaceInit() {
    ifm = InterfaceMain();
}

bool interfaceRunTick( const vector< Controller > &control, const Keystates &keyb ) {
    return ifm.tick(control, keyb);
}
    
void interfaceRenderToScreen() {
    ifm.render();
}
