
#include "gfx.h"

#include <cmath>
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
	glLineWidth( weight * 600 * 0.001f );
	glBegin( GL_LINES );
	glVertex2f( ( sx - map_sx ) / map_zoom, ( sy - map_sy ) / map_zoom );
	glVertex2f( ( ex - map_sx ) / map_zoom, ( ey - map_sy ) / map_zoom );
	glEnd();
}

const float tank_width = 5;
const float tank_length = tank_width*1.3;

float coords[3][2] =  {
	{-tank_width / 2, -tank_length / 3},
	{ tank_width / 2, -tank_length / 3},
	{ 0, tank_length * 2 / 3 }
};

void drawTank( float x, float y, float dir ) {
	glColor3f( 0.8f, 0.8f, 0.8f );
	float xtx = cos( dir );
	float xty = sin( dir );
	float ytx = sin( dir );
	float yty = -cos( dir );
	for( int i = 0; i < 3; i++ ) {
		drawLine(
			x + coords[ i ][ 0 ] * xtx + coords[ i ][ 1 ] * xty, y + coords[ i ][ 1 ] * yty + coords[ i ][ 0 ] * ytx,
			x + coords[ ( i + 1 ) % 3 ][ 0 ] * xtx + coords[ ( i + 1 ) % 3 ][ 1 ] * xty, y + coords[ ( i + 1 ) % 3 ][ 1 ] * yty + coords[ ( i + 1 ) % 3 ][ 0 ] * ytx,
			1 );
	}
};
