
#include "interface.h"
#include "game.h"
#include "gfx.h"

#include <string>
#include <vector>

using namespace std;

class StdMenu {
    
    vector<string> items;
    int cpos;
    
public:

    void pushMenuItem(const string &name);

    int tick(const Keystates &keys);
    void render() const;

    StdMenu();

};

void StdMenu::pushMenuItem(const string &name) {
    items.push_back(name);
}

int StdMenu::tick(const Keystates &keys) {
    if(keys.firing)
        return cpos;
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
        drawText(items[i].c_str(), 5, 2, 2 + 6 * i);
    }
}

StdMenu::StdMenu() {
    cpos = 0;
}

class InterfaceMain {
    
    enum { IFM_MAINMENU, IFM_PLAYING };
    int interface_mode;
    
    Game game;
    
    StdMenu mainmenu;
    
public:

    bool tick(const vector< Keystates > &keys);
    void render() const;
    InterfaceMain();

};

bool InterfaceMain::tick(const vector< Keystates > &keys) {
      
    assert(keys.size() >= 1);
    
    if(interface_mode == IFM_MAINMENU) {
        int mrv;
        mrv = mainmenu.tick(keys[0]);
        if(mrv == 0) {
            game = Game();
            interface_mode = IFM_PLAYING;
        } else if(mrv == 1) {
            return true;
        } else {
            assert(mrv == -1);
        }
    } else if(interface_mode == IFM_PLAYING) {
        if(game.runTick(keys)) {
            interface_mode = IFM_MAINMENU;
        }
    } else {
        assert(0);
    }
    
    return false;
    
}

void InterfaceMain::render() const {
    
    if(interface_mode == IFM_MAINMENU) {
        mainmenu.render();
    } else if(interface_mode == IFM_PLAYING) {
        game.renderToScreen(RENDERTARGET_SPECTATOR);
        setColor(1.0, 1.0, 1.0);
        drawText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 5, 0, 0);
        drawText("the quick brown fox jumped over the lazy dog", 5, 0, 6);
    } else {
        assert(0);
    }
    
};

InterfaceMain::InterfaceMain() {
    interface_mode = IFM_MAINMENU;
    mainmenu.pushMenuItem("New game");
    mainmenu.pushMenuItem("Exit");
}

InterfaceMain ifm;

Game game;

void interfaceInit() {
    ifm = InterfaceMain();
}

bool interfaceRunTick( const vector< Keystates > &keys ) {
    return ifm.tick(keys);
}
    
void interfaceRenderToScreen() {
    ifm.render();
}
