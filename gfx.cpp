
#include "gfx.h"

#include <cmath>
#include <fstream>
#include <map>
#include <vector>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>

#include "util.h"

Color::Color() { };
Color::Color(float in_r, float in_g, float in_b) :
    r(in_r), g(in_g), b(in_b) { };

vector< string > tokenize( string in, string kar ) {
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

vector< int > sti( const vector< string > &foo ) {
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
        CHECK(font);
        while(getline(font, line)) {
            line = string(line.begin(), find(line.begin(), line.end(), '#'));
            dprintf("Parsing font character \"%s\"\n", line.c_str());
            vector<string> first = tokenize(line, ":");
            CHECK(first.size() == 2 || line == "");
            if(first.size() == 2) {
                CHECK(first[0].size() == 1);
                vector< string > paths = tokenize(first[1], "|");
                CHECK(!fontdata.count(first[0][0]));
                fontdata[first[0][0]]; // creates it
                for(int i = 0; i < paths.size(); i++) {
                    vector< string > order = tokenize(paths[i], " ");
                    CHECK(order.size() != 1);
                    vector< pair< int, int > > tpath;
                    for(int j = 0; j < order.size(); j++) {
                        vector< int > out = sti(tokenize(order[j], ","));
                        CHECK(out.size() == 2);
                        tpath.push_back(make_pair(out[0], out[1]));
                    }
                    if(tpath.size()) {
                        CHECK(tpath.size() >= 2);
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

void setColor(const Color &color) {
    setColor(color.r, color.g, color.b);
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
	CHECK( iverts.size() % 2 == 0 );
    CHECK( iverts.size() >= 4 );
    vector< float > verts = iverts;
    if( loop ) {
        verts.push_back(verts[0]);
        verts.push_back(verts[1]);
    }
	for( int i = 0; i < verts.size() - 2; i += 2 )
		drawLine( verts[ i ], verts[ i + 1 ], verts[ i + 2 ], verts[ i + 3 ], weight );
};

void drawBox( const Float4 &box, float weight ) {
    vector<float> verts;
    verts.push_back(box.sx);
    verts.push_back(box.sy);
    verts.push_back(box.sx);
    verts.push_back(box.ey);
    verts.push_back(box.ex);
    verts.push_back(box.ey);
    verts.push_back(box.ex);
    verts.push_back(box.sy);
    drawLinePath(verts, weight, true);
}

void drawBoxAround(float x, float y, float rad, float weight) {
    drawBox(Float4(x - rad, y - rad, x + rad, y + rad), weight);
}

void drawShadedBox(const Float4 &locs, float weight, float shadedens) {
    drawBox(locs, weight);
    for(float i = locs.sx; i < locs.ex - ( locs.ey - locs.sy ); i += shadedens)
        drawLine(i, locs.sy, i + ( locs.ey - locs.sy ), locs.ey, weight);
}

float bezinterp(float x0, float x1, float x2, float x3, float t) {
    float cx = 3 * ( x1 - x0 );
    float bx = 3 * ( x2 - x1 ) - cx;
    float ax = x3 - x0 - cx - bx;
    return ax * t * t * t + bx * t * t + cx * t + x0;
}

void drawCurve( const Float4 &ptah, const Float4 &ptbh, float weight ) {
    vector<float> verts;
    for(int i = 0; i <= 100; i++) {
        verts.push_back(bezinterp(ptah.sx, ptah.ex, ptbh.sx, ptbh.ex, i / 100.0));
        verts.push_back(bezinterp(ptah.sy, ptah.ey, ptbh.sy, ptbh.ey, i / 100.0));
    }
    drawLinePath(verts, weight, false);
}

void drawCurveControls( const Float4 &ptah, const Float4 &ptbh, float spacing, float weight ) {
    drawBoxAround( ptah.sx, ptah.sy, spacing, weight );
    drawBoxAround( ptah.ex, ptah.ey, spacing, weight );
    drawBoxAround( ptbh.sx, ptbh.sy, spacing, weight );
    drawBoxAround( ptbh.ex, ptbh.ey, spacing, weight );
    drawLine( ptah, weight );
    drawLine( ptbh, weight );
}

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
        if(!fontdata.count(kar)) {
            dprintf("Can't find font for character \"%c\"", kar);
            CHECK(0);
        }
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

Vecpt Vecpt::mirror() const {
    Vecpt gn = *this;
    swap(gn.lhcx, gn.rhcx);
    swap(gn.lhcy, gn.rhcy);
    swap(gn.lhcurved, gn.rhcurved);
    return gn;
}

Vecpt::Vecpt() {
    lhcx = 16;
    lhcy = 16;
    rhcx = 16;
    rhcy = 16;
}

VectorObject loadVectors(const char *fname) {
    VectorObject rv;
    ifstream fil(fname);
    CHECK(fil);
    string buf;
    while(getline(fil, buf)) {
        if(buf.size() == 0)
            break;
        vector<string> toks = tokenize(buf, " ");
        CHECK(toks.size() == 3);
        vector<int> lhc = sti(tokenize(toks[0], "(,)"));
        CHECK(lhc.size() == 0 || lhc.size() == 2);
        vector<int> mainc = sti(tokenize(toks[1], ","));
        CHECK(mainc.size() == 2);
        vector<int> rhc = sti(tokenize(toks[2], "(,)"));
        CHECK(rhc.size() == 0 || rhc.size() == 2);
        Vecpt tvecpt;
        tvecpt.x = mainc[0];
        tvecpt.y = mainc[1];
        if(lhc.size() == 2) {
            tvecpt.lhcurved = true;
            tvecpt.lhcx = lhc[0];
            tvecpt.lhcy = lhc[1];
        } else {
            tvecpt.lhcurved = false;
        }
        if(rhc.size() == 2) {
            tvecpt.rhcurved = true;
            tvecpt.rhcx = rhc[0];
            tvecpt.rhcy = rhc[1];
        } else {
            tvecpt.rhcurved = false;
        }
        rv.points.push_back(tvecpt);
    }
    int nx = 1000000;
    int ny = 1000000;
    int mx = -1000000;
    int my = -1000000;
    for(int i = 0; i < rv.points.size(); i++) {
        nx = min(nx, rv.points[i].x);
        ny = min(ny, rv.points[i].y);
        mx = max(mx, rv.points[i].x);
        my = max(my, rv.points[i].y);
    }
    CHECK(nx != 1000000);
    CHECK(ny != 1000000);
    CHECK(mx != -1000000);
    CHECK(my != -1000000);
    rv.width = mx - nx;
    rv.height = my - ny;
    for(int i = 0; i < rv.points.size(); i++) {
        rv.points[i].x -= nx;
        rv.points[i].y -= ny;
    }
    return rv;
}

void drawVectors(const VectorObject &vecob, float x, float y, float width, float weight) {
    drawVectors(vecob, Float4(x, y, x + width, y + 1000000), true, false, weight);
}

void drawVectors(const VectorObject &vecob, const Float4 &bounds, bool cx, bool cy, float weight) {
    CHECK(bounds.isNormalized());
    float maxwidth = bounds.ex - bounds.sx;
    float maxheight = bounds.ey - bounds.sy;
    float widscale = maxwidth / vecob.width;
    float heiscale = maxheight / vecob.height;
    float scale = -1.0;
    bool wid = false;
    if(widscale < heiscale) {
        scale = widscale;
        wid = true;
    } else {
        scale = heiscale;
        wid = false;
    }
    CHECK(wid || bounds.sy + 1000000 != bounds.ey);
    CHECK(scale != -1.0);
    float widused = vecob.width * scale;
    float heiused = vecob.height * scale;
    float transx = bounds.sx;
    float transy = bounds.sy;
    if(cx)
        transx += (maxwidth - widused) / 2;
    if(cy)
        transy += (maxheight - heiused) / 2;
    Float4 translator = Float4(transx, transy, transx, transy);
    for(int i = 0; i < vecob.points.size(); i++) {
        int j = ( i + 1 ) % vecob.points.size();
        CHECK(vecob.points[i].rhcurved == vecob.points[j].lhcurved);
        if(vecob.points[i].rhcurved) {
            drawCurve(
                Float4(vecob.points[i].x, vecob.points[i].y, vecob.points[i].x + vecob.points[i].rhcx, vecob.points[i].y + vecob.points[i].rhcy) * scale + translator,
                Float4(vecob.points[j].x + vecob.points[j].lhcx, vecob.points[j].y + vecob.points[j].lhcy, vecob.points[j].x, vecob.points[j].y) * scale + translator,
                weight);
        } else {
            drawLine(Float4(vecob.points[i].x, vecob.points[i].y, vecob.points[j].x, vecob.points[j].y) * scale + translator, weight);
        }
    }
}

