
#include "gfx.h"

#include <cmath>
#include <assert.h>
using namespace std;

#include <SDL.h>
#include <GL/gl.h>

static float map_sx;
static float map_sy;
static float map_zoom;

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

void drawLine( float sx, float sy, float ex, float ey, float weight ) {
	glLineWidth( weight / map_zoom * 600 );	// has to end up in pixels - that 600 should be the y-dimension
	glBegin( GL_LINES );
	glVertex2f( ( sx - map_sx ) / map_zoom, ( sy - map_sy ) / map_zoom );
	glVertex2f( ( ex - map_sx ) / map_zoom, ( ey - map_sy ) / map_zoom );
	glEnd();
}

void drawLinePath( const vector< float > &verts, float weight ) {
	assert( verts.size() % 2 == 0 );
	for( int i = 0; i < verts.size(); i += 2 )
		drawLine( verts[ i ], verts[ i + 1 ], verts[ ( i + 2 ) % verts.size() ], verts[ ( i + 3 ) % verts.size() ], weight );
};
