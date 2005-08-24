
#include "util.h"
#include "debug.h"

#include <stdarg.h>

#include <numeric>
using namespace std;

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

float sin_table[ SIN_TABLE_SIZE + 1 ];

class sinTableMaker {
public:
	sinTableMaker() {
		for( int i = 0; i < SIN_TABLE_SIZE + 1; i++ ) {
			sin_table[ i ] = sin( i * PI / SIN_TABLE_SIZE / 2 );
		}

		//for( float x = 0; x < PI * 2; x += 0.01 )
			//dprintf( "%f-%f, %f-%f\n", fsin( x ), sin( x ), fcos( x ), cos( x ) );
	}
};

sinTableMaker sinInit;

string StringPrintf( const char *bort, ... ) {

	static vector< char > buf(2);
	va_list args;

	int done = 0;
	do {
		if( done )
			buf.resize( buf.size() * 2 );
		va_start( args, bort );
		done = vsnprintf( &(buf[ 0 ]), buf.size() - 1,  bort, args );
		assert( done < (int)buf.size() );
		va_end( args );
	} while( done == buf.size() - 1 || done == -1);

	CHECK( done < (int)buf.size() );

    return string(buf.begin(), buf.begin() + done);

};
