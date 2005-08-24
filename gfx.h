#ifndef DNET_GFX
#define DNET_GFX

#include <vector>
using namespace std;

#include "util.h"

struct Vecpt {
public:
    int x;
    int y;
    int lhcx;
    int lhcy;
    int rhcx;
    int rhcy;
    bool lhcurved;
    bool rhcurved;

    Vecpt mirror() const;
    
    Vecpt();
    
};

struct VectorObject {
public:
    vector<Vecpt> points;
    int width;
    int height;
};

VectorObject loadVectors(const char *fname);

struct Color {
public:
    float r, g, b;

    Color();
    Color(float in_r, float in_g, float in_b);
};

Color operator*( const Color &lhs, float rhs );
Color operator/( const Color &lhs, float rhs );
Color operator+( const Color &lhs, const Color &rhs );

void initGfx();
int getAccumulatedClusterCount();

void clearFrame(const Color &color);

void initFrame();
void setZoom( float sx, float sy, float ey );

void setColor( float r, float g, float b );
void setColor( const Color &color );
void drawLine( float sx, float sy, float ex, float ey, float weight );
void drawLine( const Float4 &loc, float weight );
void drawLinePath( const vector< float > &verts, float weight, bool loop );

void drawBox( const Float4 &locs, float weight );
void drawBoxAround( float x, float y, float rad, float weight );

void drawShadedBox(const Float4 &locs, float weight, float shadedens);

void drawCurve( const Float4 &ptah, const Float4 &ptbh, float weight );
void drawCurveControls( const Float4 &ptah, const Float4 &ptbh, float spacing, float weight );

void drawPoint( float x, float y, float weight );

void drawRect( const Float4 &rect, float weight );

void drawText( const char *txt, float scale, float sx, float sy );
void drawText( const string &txt, float scale, float sx, float sy );

void drawVectors(const VectorObject &vecob, float x, float y, float width, float weight);
void drawVectors(const VectorObject &vecob, const Float4 &bounds, bool cx, bool cy, float weight);

void deinitFrame();

#endif
