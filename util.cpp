
#include "util.h"
#include "debug.h"

#include <stdarg.h>

#include <numeric>

using namespace std;

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

/*************
 * Fast sin/cos
 */

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

/*************
 * Matrixtastic
 */

void Transform2d::hflip() {
    for(int i = 0; i < 3; i++)
        m[0][i] *= -1;
}
void Transform2d::vflip() {
    for(int i = 0; i < 3; i++)
        m[1][i] *= -1;
}
void Transform2d::dflip() {
    for(int i = 0; i < 3; i++)
        swap(m[0][i], m[1][i]);
}

float Transform2d::det() {
    float rv = 0;
    for(int x = 0; x < 3; x++) {
        float tv = m[x][0] * detchunk(x, 0);
        if(x % 2)
            tv = -tv;
        rv += tv;
    }
    return rv;
}

vector<int> allExcept(int t) {
    CHECK(t >= 0 && t < 3);
    vector<int> rv;
    for(int i = 0; i < 3; i++)
        rv.push_back(i);
    rv.erase(find(rv.begin(), rv.end(), t));
    CHECK(rv.size() == 2);
    return rv;
}

float Transform2d::detchunk(int x, int y) {
    // this code sucks.
    vector<int> xv = allExcept(x);
    vector<int> yv = allExcept(y);
    CHECK(xv.size() == 2 && yv.size() == 2);
    return m[xv[0]][yv[0]] * m[xv[1]][yv[1]] - m[xv[0]][yv[1]] * m[xv[1]][yv[0]];
}

void Transform2d::invert() {
    // hahahahahhahah.
    Transform2d res;
    for(int x = 0; x < 3; x++) {
        for(int y = 0; y < 3; y++) {
            res.m[x][y] = detchunk(y, x) / det();
            if((x + y) % 2)
                res.m[x][y] = -res.m[x][y];
        }
    }
    *this = res;
}

float Transform2d::mx(float x, float y) const {
    float ox = 0.0;
    ox += m[0][0] * x;
    ox += m[0][1] * y;
    ox += m[0][2];
    return ox;
}

float Transform2d::my(float x, float y) const {
    float oy = 0.0;        
    oy += m[1][0] * x;
    oy += m[1][1] * y;
    oy += m[1][2];
    return oy;
}

void Transform2d::transform(float *x, float *y) const {
    float px = *x;
    float py = *y;
    *x = mx(px, py);
    *y = my(px, py);
}

void Transform2d::display() const {
    for(int i = 0; i < 3; i++)
        dprintf("  %f %f %f\n", m[0][i], m[1][i], m[2][i]);
}

Transform2d::Transform2d() {
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            m[i][j] = ( i == j );
}

Transform2d operator*(const Transform2d &lhs, const Transform2d &rhs) {
    Transform2d rv;
    for(int x = 0; x < 3; x++) {
        for(int y = 0; y < 3; y++) {
            rv.m[x][y] = 0;
            for(int z = 0; z < 3; z++) {
                rv.m[x][y] += lhs.m[z][y] * rhs.m[x][z];
            }
        }
    }
    return rv;
}

Transform2d &operator*=(Transform2d &lhs, const Transform2d &rhs) {
    lhs = lhs * rhs;
    return lhs;
}

Transform2d t2d_identity() {
    return Transform2d();
}
Transform2d t2d_flip(bool h, bool v, bool d) {
    Transform2d o;
    if(h)
        o.hflip();
    if(v)
        o.vflip();
    if(d)
        o.dflip();
    return o;
}
Transform2d t2d_rotate(float rads) {
    Transform2d o;
    o.m[0][0] = cos(rads);
    o.m[0][1] = sin(rads);
    o.m[1][0] = -sin(rads);
    o.m[1][1] = cos(rads);
    return o;
}
