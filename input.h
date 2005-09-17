#ifndef DNET_INPUT
#define DNET_INPUT

#include <vector>

using namespace std;

/*************
 * User input
 */
 
class Button {
public:
    bool down;
    bool up;
    bool push;
    bool release;
    bool repeat;
    int dur;
    int sincerep;
    
    void newState(bool pushed);
    void newState(const Button &other);

    Button();
};

class Controller {
public:
    float x;
    float y;
    Button u;
    Button d;
    Button l;
    Button r;
    vector<Button> keys;

    void newState(const Controller &nst);

};

enum { KSAX_UDLR, KSAX_ABSOLUTE, KSAX_TANK };

class Keystates {
public:
    float ax[2];
    int axmode;
    Button u,d,l,r,f;
};

#endif
