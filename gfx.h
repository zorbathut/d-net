#ifndef DNET_GFX
#define DNET_GFX

#include <vector>
using namespace std;

#include "util.h"
#include "coord.h"
#include "dvec2.h"

/*************
 * Color struct
 */
 
struct Color {
public:
  float r, g, b;

  Color();
  Color(float in_r, float in_g, float in_b);
};

Color colorFromString(const string &str);

Color operator+(const Color &lhs, const Color &rhs);
Color operator*(const Color &lhs, float rhs);
Color operator/(const Color &lhs, float rhs);

const Color &operator+=(Color &lhs, const Color &rhs);

/*************
 * Setup and statistics
 */

void setDefaultResolution(int cur_width, int cur_height, bool fullscreen);
int getResolutionX();
int getResolutionY();
void resDown();

void initGfx();
string printGraphicsStats();

void clearFrame(const Color &color);

void initFrame();
void setZoom( float sx, float sy, float ey );
void setZoomAround(const Coord4 &bbox);
void setZoomCenter(float cx, float cy, float radius_y);

float getAspect();

void deinitFrame();

/*************
 * Window
 */

class GfxWindow {
public:
  GfxWindow(const Float4 &bounds);
  ~GfxWindow();
};

/*************
 * Primitives
 */
 
void setColor( float r, float g, float b );
void setColor( const Color &color );

void drawLine( float sx, float sy, float ex, float ey, float weight );
void drawLine( const Float2 &s, const Float2 &e, float weight );
void drawLine( const Coord2 &s, const Coord2 &e, float weight );
void drawLine( const Float4 &loc, float weight );
void drawLine( const Coord4 &loc, float weight );

void drawPoint( float x, float y, float weight );

void drawSolid(const Float4 &box);

/*************
 * Composites
 */

void drawLinePath( const vector<Float2> &verts, float weight);

void drawLineLoop( const vector<Float2> &verts, float weight);
void drawLineLoop( const vector<Coord2> &verts, float weight);

void drawTransformedLinePath(const vector<Float2> &verts, float angle, Float2 translate, float weight);

void drawRect(const Float4 &rect, float weight);
void drawRectAround( float x, float y, float rad, float weight );

void drawShadedRect(const Float4 &locs, float weight, float shadedens);

void drawCurve(const Float4 &ptah, const Float4 &ptbh, int midpoints, float weight);
void drawCurveControls(const Float4 &ptah, const Float4 &ptbh, float spacing, float weight);

void drawCircle( const Float2 &center, float radius, float weight );
void drawCircle( const Coord2 &center, Coord radius, Coord weight );

void drawText( const char *txt, float scale, float sx, float sy );
void drawText( const string &txt, float scale, float sx, float sy );
void drawText( const string &txt, float scale, const Float2 &pos );

enum { TEXT_MIN, TEXT_CENTER, TEXT_MAX };

void drawJustifiedText(const string &txt, float scale, float sx, float sy, int xps, int yps);
void drawJustifiedMultiText(const vector<string> &txt, float letterscale, float gapscale, Float2 pos, int xps, int yps);

// VectorPath objects have their own local coordinate system - this scales it by whatever, then translates its origin to the new origin.
// It is not necessarily an upper-left corner origin (it's more likely to be center, but that's not guaranteed either)
void drawVectorPath(const VectorPath &vecob, const pair<pair<float, float>, float> &coord, int midpoints, float weight);
void drawVectorPath(const VectorPath &vecob, const Float4 &bounds, int midpoints, float weight);

void drawDvec2(const Dvec2 &vecob, const Float4 &bounds, int midpoints, float weight);

void drawSpokes(float x, float y, int dupes, int numer, int denom, float len, float weight);
void drawGrid(float spacing, float width);

void drawCrosshair(float x, float y, float rad, float weight);


#endif
