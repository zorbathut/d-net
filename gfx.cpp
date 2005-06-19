
#include "gfx.h"

#include <cmath>
#include <fstream>
#include <map>
#include <vector>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>

#include "util.h"

static vector< string > tokenize( string in, string kar ) {
	string::iterator cp = in.begin();
	vector< string > oot;
	while( cp != in.end() ) {
		while( cp != in.end() && count( kar.begin(), kar.end(), *cp ) )
			cp++;
		if( cp != in.end() )
			oot.push_back( string( cp, find_first_of( cp, in.end(), kar.begin(), kar.end() ) ) );
		cp = find_first_of( cp, in.end(), kar.begin(), kar.end() );
	};
	return oot;
};

static vector< int > sti( const vector< string > &foo ) {
	int i;
	vector< int > bar;
	for( i = 0; i < foo.size(); i++ ) {
		bar.push_back( atoi( foo[ i ].c_str() ) );
	}
	return bar;
};

static float map_sx;
static float map_sy;
static float map_zoom;

static map< char, vector< vector< pair< int, int > > > > fontdata;

void initGfx() {
    {
        ifstream font("data/font.txt");
        string line;
        assert(font);
        while(getline(font, line)) {
            line = string(line.begin(), find(line.begin(), line.end(), '#'));
            vector<string> first = tokenize(line, ":");
            assert(first.size() == 2 || line == "");
            if(first.size() == 2) {
                assert(first[0].size() == 1);
                vector< string > paths = tokenize(first[1], "|");
                assert(!fontdata.count(first[0][0]));
                fontdata[first[0][0]]; // creates it
                for(int i = 0; i < paths.size(); i++) {
                    vector< string > order = tokenize(paths[i], " ");
                    assert(order.size() != 1);
                    vector< pair< int, int > > tpath;
                    for(int j = 0; j < order.size(); j++) {
                        vector< int > out = sti(tokenize(order[j], ","));
                        assert(out.size() == 2);
                        tpath.push_back(make_pair(out[0], out[1]));
                    }
                    if(tpath.size()) {
                        assert(tpath.size() >= 2);
                        fontdata[first[0][0]].push_back(tpath);
                    }
                }
            }
        }
    }
    {
        GLfloat flipy[16]= { 1, 0, 0, 0,   0, -1, 0, 0,   0, 0, 1, 0,    0, 0, 0, 1 };
        glMultMatrixf(flipy);
        glTranslatef(0, -1, 0);
    }
}

void initFrame() {
	glClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	//glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST );
	//glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

}

void deinitFrame() {
	glFlush();
	SDL_GL_SwapBuffers(); 
}

void setZoom( float in_sx, float in_sy, float in_ey ) {
	map_sx = in_sx;
	map_sy = in_sy;
	map_zoom = in_ey - in_sy;
}

void setColor( float r, float g, float b ) {
	glColor3f( r, g, b );
}

void drawLine( float sx, float sy, float ex, float ey, float weight ) {
	glLineWidth( weight / map_zoom * 600 );   // GL uses pixels internally for this unit, so I have to translate from game-meters
	glBegin( GL_LINES );
	glVertex2f( ( sx - map_sx ) / map_zoom, ( sy - map_sy ) / map_zoom );
	glVertex2f( ( ex - map_sx ) / map_zoom, ( ey - map_sy ) / map_zoom );
	glEnd();
}

void drawLine( const Float4 &pos, float weight ) {
	drawLine( pos.sx, pos.sy, pos.ex, pos.ey, weight );
}

void drawLinePath( const vector< float > &iverts, float weight, bool loop ) {
	assert( iverts.size() % 2 == 0 );
    assert( iverts.size() >= 4 );
    vector< float > verts = iverts;
    if( loop ) {
        verts.push_back(verts[0]);
        verts.push_back(verts[1]);
    }
	for( int i = 0; i < verts.size() - 2; i += 2 )
		drawLine( verts[ i ], verts[ i + 1 ], verts[ i + 2 ], verts[ i + 3 ], weight );
};

void drawPoint( float x, float y, float weight ) {
    glPointSize( weight / map_zoom * 600 );   // GL uses pixels internally for this unit, so I have to translate from game-meters
    glBegin( GL_POINTS );
	glVertex2f( ( x - map_sx ) / map_zoom, ( y - map_sy ) / map_zoom );
	glEnd();
}

void drawRect( const Float4 &rect, float weight ) {
	vector< float > verts;
	verts.push_back( rect.sx );
	verts.push_back( rect.sy );
	verts.push_back( rect.ex );
	verts.push_back( rect.sy );
	verts.push_back( rect.ex );
	verts.push_back( rect.ey );
	verts.push_back( rect.sx );
	verts.push_back( rect.ey );
	drawLinePath( verts, weight, true );
};

void drawText( const char *txt, float scale, float sx, float sy ) {
    scale /= 9;
    for(int i = 0; txt[i]; i++) {
        char kar = toupper(txt[i]);
        assert(fontdata.count(kar));
        const vector<vector<pair<int, int> > > &pathdat = fontdata[kar];
        for(int i = 0; i < pathdat.size(); i++) {
            vector<float> verts;
            for(int j = 0; j < pathdat[i].size(); j++) {
                verts.push_back(sx + pathdat[i][j].first * scale);
                verts.push_back(sy + pathdat[i][j].second * scale);
            }
            drawLinePath(verts, scale / 5, false);
        }
        sx += scale * 8;
    }
}

