#ifndef DNET_GFX
#define DNET_GFX

#include "cfc.h"
#include "color.h"
#include "dvec2.h"
#include "image.h"

#include <boost/noncopyable.hpp>

using namespace std;

/*************
 * Setup and statistics
 */

void loadFonts();
void initGfx();
void initWindowing(float aspect); // this isn't really necessary if you plan to call updateResolution
void updateResolution(float aspect); // picks up viewport data from OpenGL

int getResolutionX();
int getResolutionY();

string printGraphicsStats();

void clearFrame(const Color &color);
void clearStencil();

void initFrame();
void setZoomAround(const CFC4 &bbox);
void setZoomCenter(float cx, float cy, float radius_y);
void setZoomVertical(float sx, float sy, float ey);
void setZoomHorizontal(float sx, float ex, float sy);

Float4 getZoom();

float getAspect();
float getScreenAspect();

void flushFrame();
void deinitFrame();

/*************
 * Window
 */

class GfxWindow : boost::noncopyable {
public:
  GfxWindow(const Float4 &bounds, float fade);
  ~GfxWindow();
};

class GfxStenciled : boost::noncopyable {
public:
  GfxStenciled();
  ~GfxStenciled();
};

class GfxInvertingStencil : boost::noncopyable {
public:
  GfxInvertingStencil();
  ~GfxInvertingStencil();
};

/*************
 * Primitives
 */
 
void setColor(float r, float g, float b);
void setColor(const Color &color);

void drawLine(float sx, float sy, float ex, float ey, float weight);
void drawLine(const Coord &sx, const Coord &sy, const Coord &ex, const Coord &ey, float weight);
void drawLine(const Float2 &s, const Float2 &e, float weight);
void drawLine(const Coord2 &s, const Coord2 &e, float weight);
void drawLine(const Float4 &loc, float weight);
void drawLine(const Coord4 &loc, float weight);

void buildLine(const Float4 &loc, vector<Float2> *out);

void drawPoint(const Float2 &pos, float weight);

void drawSolid(const Float4 &box);  // Background color only, and intentionally so
void drawSolidLoop(const vector<Float2> &verts); // Must be convex

void invertStencilLoop(const vector<Float2> &verts);
void invertStencilLoop(const vector<Coord2> &verts);

void drawImage(const Image &img, const Float4 &box, float alpha); // arglbargl

/*************
 * Composites
 */

void drawLinePath(const vector<Float2> &verts, float weight);
void drawLinePath(const vector<Coord2> &verts, float weight);

void drawLineLoop(const vector<Float2> &verts, float weight);
void drawLineLoop(const vector<Coord2> &verts, float weight);

void drawTransformedLinePath(const vector<Float2> &verts, float angle, Float2 translate, float weight);

void drawRect(const CFC4 &rect, float weight);
void drawRectAround(const CFC2 &pos, float rad, float weight);

void drawShadedRect(const Float4 &locs, float weight, float shadedens);

void drawCurve(const Float4 &ptah, const Float4 &ptbh, int midpoints, float weight);
void buildCurve(const Float4 &ptah, const Float4 &ptbh, int midpoints, vector<Float2> *out);

void drawCircle(const Float2 &center, float radius, float weight);

void drawText(const string &txt, float scale, const Float2 &pos);

float getTextWidth(const string &txt, float scale);
float getTextHeight(int lines, float scale);
float getFormattedTextHeight(const string &txt, float scale, float width);

float getTextBoxBorder(float scale);
float getTextBoxThickness(float scale);

void drawTextBoxAround(const Float4 &bounds, float textscale);

enum Justification { TEXT_MIN, TEXT_CENTER, TEXT_MAX };

void drawJustifiedText(const string &txt, float scale, const CFC2 &pos, Justification xjust, Justification yjust);
void drawJustifiedMultiText(const vector<string> &txt, float letterscale, const CFC2 &pos, Justification xjust, Justification yjust);

void drawFormattedText(const string &txt, float scale, Float4 bounds);
void drawFormattedText(const vector<string> &txt, float scale, Float4 bounds);
void drawParagraphedText(const string &txt, float scale, pair<float, float> x_bounds, float y);
void drawJustifiedParagraphedText(const string &txt, float scale, pair<float, float> x_bounds, float y, Justification yjust);

void drawFormattedTextBox(const vector<string> &txt, float scale, Float4 bounds, Color text, Color box);
void drawJustifiedTextBox(const vector<string> &txt, float scale, const CFC2 &pos, Justification xjust, Justification yjust, Color text, Color box);

// VectorPath objects have their own local coordinate system - this scales it by whatever, then translates its origin to the new origin.
// It is not necessarily an upper-left corner origin (it's more likely to be center, but that's not guaranteed either)
void drawVectorPath(const VectorPath &vecob, const pair<Float2, float> &coord, int midpoints, float weight);
void drawVectorPath(const VectorPath &vecob, const CFC4 &bounds, int midpoints, float weight);
void stencilVectorPath(const VectorPath &vecob, const pair<Float2, float> &bounds, int midpoints);
void stencilVectorPath(const VectorPath &vecob, const CFC4 &bounds, int midpoints);

void drawDvec2(const Dvec2 &vecob, const CFC4 &bounds, int midpoints, float weight);
void stencilDvec2(const Dvec2 &vecob, const CFC4 &bounds, int midpoints);

void drawSpokes(float x, float y, int dupes, int numer, int denom, float len, float weight);
void drawGrid(float spacing, float width);

void drawCrosshair(const CFC2 &pos, float rad, float weight);
void drawBlast(const CFC2 &center, float rad, float chaos, int vertices, int seed);

/*************
 * Color stack
 */

class ColorStack : boost::noncopyable {
public:
  ColorStack(const Color &color);
  ~ColorStack();
};

#endif
