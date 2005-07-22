#ifndef DNET_GFX
#define DNET_GFX

#include <vector>
using namespace std;

#include "util.h"

void initGfx();

void initFrame();
void setZoom( float sx, float sy, float ey );

void setColor( float r, float g, float b );
void drawLine( float sx, float sy, float ex, float ey, float weight );
void drawLine( const Float4 &loc, float weight );
void drawLinePath( const vector< float > &verts, float weight, bool loop );

void drawBox( const Float4 &locs, float weight );
void drawBoxAround( float x, float y, float rad, float weight );

void drawCurve( const Float4 &ptah, const Float4 &ptbh, float weight );
void drawCurveControls( const Float4 &ptah, const Float4 &ptbh, float spacing, float weight );

void drawPoint( float x, float y, float weight );

void drawRect( const Float4 &rect, float weight );

void drawText( const char *txt, float scale, float sx, float sy );

void deinitFrame();

#endif
