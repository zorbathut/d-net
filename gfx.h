#ifndef DNET_GFX
#define DNET_GFX

#include <vector>
using namespace std;

#include "util.h"

void initFrame();
void setZoom( float sx, float sy, float ey );

void setColor( float r, float g, float b );
void drawLine( float sx, float sy, float ex, float ey, float weight );
void drawLinePath( const vector< float > &verts, float weight );

void drawRect( const Float4 &rect, float weight );

void deinitFrame();

#endif
