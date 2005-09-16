
#include "coord.h"

/*************
 * Bounding box
 */

Coord4 startCBoundBox() {
    return Coord4(1000000000, 1000000000, -1000000000, -1000000000);
};

void addToBoundBox(Coord4 *bbox, Coord x, Coord y) {
    bbox->sx = min(bbox->sx, x);
    bbox->sy = min(bbox->sy, y);
    bbox->ex = max(bbox->ex, x);
    bbox->ey = max(bbox->ey, y);
};
void addToBoundBox(Coord4 *bbox, const Coord2 &point) {
    addToBoundBox(bbox, point.x, point.y);
};
void addToBoundBox(Coord4 *bbox, const Coord4 &rect) {
    CHECK(rect.isNormalized());
    addToBoundBox(bbox, rect.sx, rect.sy);
    addToBoundBox(bbox, rect.ex, rect.ey);
};
