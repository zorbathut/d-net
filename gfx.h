#ifndef DNET_GFX
#define DNET_GFX

#include <vector>
using namespace std;

#include "util.h"
#include "dvec2.h"

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

void drawVectorPath(const VectorPath &vecob, float x, float y, float width, float weight);
void drawVectorPath(const VectorPath &vecob, const Float4 &bounds, bool cx, bool cy, float weight);

void drawDvec2(const Dvec2 &vecob, const Float4 &bounds, bool cx, bool cy, float weight);

void drawSpokes(float x, float y, int dupes, int numer, int denom, float len, float weight);

void deinitFrame();

#endif
