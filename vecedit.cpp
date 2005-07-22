
#include "vecedit.h"
#include "gfx.h"

bool vecEditTick(const Controller &keys) {
    return false;
}
void vecEditRender(void) {
    setZoom(0,0,100);
    setColor(1.0,1.0,1.0);
    drawCurve(Float4(20,20,40,40), Float4(60,20,80,40), 0.5);
    drawCurveControls(Float4(20,20,40,40), Float4(60,20,80,40), 1, 0.2);
}
