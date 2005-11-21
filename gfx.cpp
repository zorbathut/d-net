
#include "gfx.h"

#include <cmath>
#include <fstream>
#include <map>
#include <vector>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>

#include "util.h"
#include "parse.h"
#include "args.h"

DEFINE_int(resolution_x, -1, "X resolution (Y is X/4*3), -1 for autodetect");

void setDefaultResolution(int width, int height, bool fullscreen) {
    dprintf("Current detected resolution: %d/%d\n", width, height);
    if(FLAGS_resolution_x == -1) {
        FLAGS_resolution_x = width;
        if(!fullscreen)
            resDown();
    }
}
    
int getResolutionX() {
    CHECK(FLAGS_resolution_x > 0);
    CHECK(FLAGS_resolution_x % 4 == 0);
    return FLAGS_resolution_x;
}
int getResolutionY() {
    CHECK(FLAGS_resolution_x > 0);
    CHECK(FLAGS_resolution_x % 4 == 0);
    return FLAGS_resolution_x / 4 * 3;
}

void resDown() {
    const int reses[] = { 1600, 1400, 1280, 1152, 1024, 800, 640, -1 };
    CHECK(FLAGS_resolution_x > 0);
    for(int i = 0; i < sizeof(reses) / sizeof(*reses); i++) {
        if(FLAGS_resolution_x > reses[i]) {
            FLAGS_resolution_x = reses[i];
            break;
        }
    }
    CHECK(FLAGS_resolution_x > 0);
}

Color::Color() { };
Color::Color(float in_r, float in_g, float in_b) :
    r(in_r), g(in_g), b(in_b) { };

static float map_sx;
static float map_sy;
static float map_ex;
static float map_ey;
static float map_zoom;

static map< char, vector< vector< pair< int, int > > > > fontdata;

Color operator*( const Color &lhs, float rhs ) {
    return Color(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs);
}
Color operator/( const Color &lhs, float rhs ) {
    return Color(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs);
}
Color operator+( const Color &lhs, const Color &rhs ) {
    return Color(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b);
}

void initGfx() {
    {
        ifstream font("data/font.txt");
        string line;
        CHECK(font);
        while(getLineStripped(font, line)) {
            dprintf("Parsing font character \"%s\"\n", line.c_str());
            vector<string> first = tokenize(line, ":");
            if(first.size() == 1)
                first.push_back("");
            CHECK(first.size() == 2);
            if(first.size() == 2) {
                if(first[0] == "colon")
                    first[0] = ":";
                if(first[0] == "hash")
                    first[0] = "#";
                if(first[0] == "space")
                    first[0] = " ";
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
        GLfloat flipy[16]= { (5.0/4.0) / (4.0/3.0), 0, 0, 0,   0, -1, 0, 0,   0, 0, 1, 0,    0, 0, 0, 1 };
        glMultMatrixf(flipy);
        glTranslatef(0, -1, 0);
    }
}

float curWeight = -1.f;
int lineCount = 0;
int clusterCount = 0;

int getAccumulatedClusterCount() {
    int tempcount = clusterCount;
    clusterCount = 0;
    return tempcount;
}

void beginLineCluster(float weight) {
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK(curWeight == -1.f);
    glLineWidth( weight / map_zoom * getResolutionY() );   // GL uses pixels internally for this unit, so I have to translate from game-meters
    CHECK(glGetError() == GL_NO_ERROR);
    glBegin(GL_LINES);
    curWeight = weight;
    lineCount = 0;
    clusterCount++;
}

void finishLineCluster() {
    if(curWeight != -1.f) {
        curWeight = -1.f;
        glEnd();
        lineCount = 0;
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

void initFrame() {
    CHECK(curWeight == -1.f);
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	//glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST );
	//glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    clearFrame(Color(0.1, 0.1, 0.1));
    CHECK(glGetError() == GL_NO_ERROR);
}

static Color curcolor;
static Color clearcolor;

void clearFrame(const Color &color) {
    finishLineCluster();
    glClearColor( color.r, color.g, color.b, 0.0f );
	glClear(GL_COLOR_BUFFER_BIT);
    clearcolor = color;
}

void deinitFrame() {
    finishLineCluster();
	glFlush();
	SDL_GL_SwapBuffers(); 
    CHECK(glGetError() == GL_NO_ERROR);
}

void setZoom( float in_sx, float in_sy, float in_ey ) {
    finishLineCluster();
	map_sx = in_sx;
	map_sy = in_sy;
	map_zoom = in_ey - in_sy;
    map_ey = map_sy + map_zoom;
    map_ex = map_sx + map_zoom * 4 / 3;
}

float getZoomSx() { return map_sx; }
float getZoomSy() { return map_sy; }
float getZoomEx() { return map_ex; }
float getZoomEy() { return map_ey; }
float getZoomDx() { return map_ex - map_sx; }
float getZoomDy() { return map_ey - map_sy; }

void setColor( float r, float g, float b ) {
	glColor3f( r, g, b );
    curcolor = Color(r, g, b);
}

void setColor(const Color &color) {
    setColor(color.r, color.g, color.b);
}

void localVertex2f(float x, float y) {
    glVertex2f( ( x - map_sx ) / map_zoom, ( y - map_sy ) / map_zoom );
}

void drawLine( float sx, float sy, float ex, float ey, float weight ) {
    CHECK(weight > 0);
    if(weight != curWeight || lineCount > 1000) {
        finishLineCluster();
        beginLineCluster(weight);
    }
    localVertex2f(sx, sy);
    localVertex2f(ex, ey);
    lineCount++;
}
void drawLine(const Float2 &s, const Float2 &e, float weight) {
    drawLine(s.x, s.y, e.x, e.y, weight);
}
void drawLine(const Coord2 &s, const Coord2 &e, float weight) {
    drawLine(s.toFloat(), e.toFloat(), weight);
}
void drawLine( const Float4 &pos, float weight ) {
	drawLine( pos.sx, pos.sy, pos.ex, pos.ey, weight );
}
void drawLine( const Coord4 &loc, float weight ) {
    drawLine(loc.toFloat(), weight);
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
void drawLinePath( const vector<Float2> &iverts, float weight, bool loop ) {
    CHECK( iverts.size() >= 1 );
    vector<Float2> verts = iverts;
    if( loop )
        verts.push_back(verts[0]);
	for( int i = 0; i < verts.size() - 1; i++ )
		drawLine( verts[i], verts[i + 1], weight );
};
void drawLinePath( const vector<Coord2> &iverts, float weight, bool loop ) {
    CHECK( iverts.size() >= 1 );
    vector<Coord2> verts = iverts;
    if( loop )
        verts.push_back(verts[0]);
	for( int i = 0; i < verts.size() - 1; i++ )
		drawLine( verts[i], verts[i + 1], weight );
};

void drawTransformedLinePath(const vector<Float2> &verts, float angle, Float2 transform, float weight) {
    vector<Float2> transed;
    for(int i = 0; i < verts.size(); i++)
        transed.push_back(rotate(verts[i], angle) + transform);
    drawLinePath(transed, weight, true);
};

void drawSolid(const Float4 &box) {
    CHECK(box.isNormalized());
    finishLineCluster();
    CHECK(glGetError() == GL_NO_ERROR);
    glColor3f(clearcolor.r, clearcolor.g, clearcolor.b);
    glBlendFunc( GL_ONE, GL_ZERO );
    glBegin(GL_TRIANGLE_STRIP);
    localVertex2f(box.sx, box.sy);
    localVertex2f(box.sx, box.ey);
    localVertex2f(box.ex, box.sy);
    localVertex2f(box.ex, box.ey);
    glEnd();
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    CHECK(glGetError() == GL_NO_ERROR);
    setColor(curcolor);
}

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
    float sp = locs.sx - locs.ey + locs.sy;
    sp = sp - fmod(sp, shadedens) + shadedens;
    for(float xp = sp; xp < locs.ex; xp += shadedens) {
        Float2 spos;
        Float2 epos;
        if(xp >= locs.sx) {
            spos = Float2(xp, locs.sy);
        } else {
            spos = Float2(locs.sx, locs.sx - xp + locs.sy);
        }
        if(xp + locs.ey - locs.sy < locs.ex) {
            epos = Float2(xp + locs.ey - locs.sy, locs.ey);
        } else {
            epos = Float2(locs.ex, locs.ex - xp + locs.sy);
        }
        drawLine(spos, epos, weight);
    }
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

void drawCircle( const Float2 &center, float radius, float weight ) {
    vector<Float2> verts;
    for(int i = 0; i < 16; i++)
        verts.push_back(makeAngle(i * PI / 8) * radius + center);
    drawLinePath(verts, weight, true);
}

void drawPoint( float x, float y, float weight ) {
    finishLineCluster();
    glPointSize( weight / map_zoom * getResolutionY() );   // GL uses pixels internally for this unit, so I have to translate from game-meters
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

void drawText(const string &txt, float scale, float sx, float sy) {
    drawText(txt.c_str(), scale, sx, sy);
}

void drawText(const string &txt, float scale, const Float2 &pos) {
    drawText(txt.c_str(), scale, pos.x, pos.y);
}

void drawJustifiedText(const string &txt, float scale, float sx, float sy, int xps, int yps) {
    float lscale = scale / 9;
    float wid = lscale * ( 8 * txt.size() - 3 );
    if(xps == TEXT_MIN) {
    } else if(xps == TEXT_CENTER) {
        sx -= wid / 2;
    } else if(xps == TEXT_MAX) {
        sx -= wid;
    }
    if(yps == TEXT_MIN) {
    } else if(yps == TEXT_CENTER) {
        sy -= lscale * 9 / 2;
    } else if(yps == TEXT_MAX) {
        sy -= lscale * 9;
    }
    drawText(txt, scale, sx, sy);
}

void drawVectorPath(const VectorPath &vecob, const pair<Float2, float> &coord, float weight) {
    for(int i = 0; i < vecob.vpath.size(); i++) {
        int j = (i + 1) % vecob.vpath.size();
        CHECK(vecob.vpath[i].curvr == vecob.vpath[j].curvl);
        float lx = vecob.centerx + vecob.vpath[i].x;
        float ly = vecob.centery + vecob.vpath[i].y;
        float rx = vecob.centerx + vecob.vpath[j].x;
        float ry = vecob.centery + vecob.vpath[j].y;
        
        // these are invalid and meaningless if it's not a curve, but hey!
        float lcx = lx + vecob.vpath[i].curvrx;
        float lcy = ly + vecob.vpath[i].curvry;
        float rcx = rx + vecob.vpath[j].curvlx;
        float rcy = ry + vecob.vpath[j].curvly;
        
        lx *= coord.second;
        ly *= coord.second;
        rx *= coord.second;
        ry *= coord.second;
        
        lcx *= coord.second;
        lcy *= coord.second;
        rcx *= coord.second;
        rcy *= coord.second;
        
        lx += coord.first.x;
        ly += coord.first.y;
        rx += coord.first.x;
        ry += coord.first.y;
        
        lcx += coord.first.x;
        lcy += coord.first.y;
        rcx += coord.first.x;
        rcy += coord.first.y;
        if(vecob.vpath[i].curvr) {
            drawCurve(Float4(lx, ly, lcx, lcy), Float4(rcx, rcy, rx, ry), weight);
        } else {
            drawLine(Float4(lx, ly, rx, ry), weight);
        }
    }
}

void drawVectorPath(const VectorPath &vecob, const Float4 &bounds, float weight) {
    CHECK(bounds.isNormalized());
    drawVectorPath(vecob, fitInside(bounds, vecob.boundingBox()), weight);
}

void drawDvec2(const Dvec2 &vecob, const Float4 &bounds, float weight) {
    CHECK(vecob.entities.size() == 0);
    pair<Float2, float> dimens = fitInside(vecob.boundingBox(), bounds);
    //dprintf("fit %f,%f,%f,%f into %f,%f,%f,%f, got %f,%f, %f\n", vecob.boundingBox().sx, vecob.boundingBox().sy,
            //vecob.boundingBox().ex, vecob.boundingBox().ey, bounds.sx, bounds.sy, bounds.ex, bounds.ey,
            //dimens.first.first, dimens.first.second, dimens.second);
    for(int i = 0; i < vecob.paths.size(); i++)
        drawVectorPath(vecob.paths[i], dimens, weight);
}

void drawSpokes(float x, float y, int dupes, int numer, int denom, float len, float weight) {
    for(int i = 0; i < dupes; i++) {
        float ang = ((float)numer / denom + (float)i / dupes) * 2 * PI;
        drawLine(x, y, x + cos(ang) * len, y + sin(ang) * len, weight);
    }
}

void drawGrid(float spacing, float size) {
    CHECK(map_sx <= 0 && map_ex >= 0 && map_sy <= 0 && map_ey >= 0);
    for(float s = 0; s < map_ex; s += spacing)
        drawLine(s, map_sy, s, map_ey, size);
    for(float s = -spacing; s > map_sx; s -= spacing)
        drawLine(s, map_sy, s, map_ey, size);
    for(float s = 0; s < map_ey; s += spacing)
        drawLine(map_sx, s, map_ex, s, size);
    for(float s = -spacing; s > map_sy; s -= spacing)
        drawLine(map_sx, s, map_ex, s, size);
}

void drawCrosshair(float x, float y, float rad, float weight) {
    drawLine(x - rad, y, x + rad, y, weight);
    drawLine(x, y - rad, x, y + rad, weight);
}
