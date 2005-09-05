
#include "input.h"

/*************
 * User input
 */
 
Button::Button() {
    down = push = release = repeat = false;
    up = true;
    dur = 0;
    sincerep = 0;
}

void Button::newState(const Button &other) {
    newState(other.down);
}

void Button::newState(bool pushed) {
    if(pushed == down) {
        push = false;
        release = false;
    } else {
        dur = 0;
        sincerep = 0;
        if(pushed) {
            push = true;
        } else {
            release = true;
        }
        down = pushed;
        up = !pushed;
    }
    repeat = false;
    dur++;
    if(down) {
        if(sincerep % 10 == 0) {
            repeat = true;
        }
        sincerep++;
    }
}

void Controller::newState(const Controller &nst) {
    x = nst.x;
    y = nst.y;
    u.newState(nst.u);
    d.newState(nst.d);
    l.newState(nst.l);
    r.newState(nst.r);
    CHECK(keys.size() == nst.keys.size());
    for(int i = 0; i < keys.size(); i++)
        keys[i].newState(nst.keys[i]);
}

void Keystates::newState(const Keystates &nst) {
    u.newState(nst.u);
    d.newState(nst.d);
    l.newState(nst.l);
    r.newState(nst.r);
    f.newState(nst.f);
}

Keystates::Keystates() {
};
