#ifndef DNET_GFX
#define DNET_GFX

#include <vector>
using namespace std;

void initFrame();
void setZoom( float sx, float sy, float ey );

void setColor( float r, float g, float b );
void drawLine( float sx, float sy, float ex, float ey, float weight );
void drawLinePath( const vector< float > &verts, float weight );

void deinitFrame();

#endif
