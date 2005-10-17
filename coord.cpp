
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

int inPath(const Coord2 &point, const vector<Coord2> &path) {
    CHECK(path.size());
    Coord accum = 0;
    Coord cpt = angle(path.back() - point);
    for(int i = 0; i < path.size(); i++) {
        Coord npt = angle(path[i] - point);
        //dprintf("%f to %f, %f\n", cpt.toFloat(), npt.toFloat(), (npt-cpt).toFloat());
        Coord diff = npt - cpt;
        if(diff < -COORDPI)
            diff += COORDPI * 2;
        if(diff > COORDPI)
            diff -= COORDPI * 2;
        accum += diff;
        cpt = npt;
    }
    accum /= COORDPI * 2;
    int solidval = -1;
    if(accum < 0) {
        accum = -accum;
        solidval = 1;
    }
    if(accum < Coord(0.5f)) {
        CHECK(accum < Coord(0.0001f));
        return 0;
    } else {
        CHECK(accum > Coord(0.9999f) && accum < Coord(1.0001f));
        return solidval;
    }
};

Coord2 getPointIn(const vector<Coord2> &path) {
    // TODO: find a point inside the polygon in a better fashion
    Coord2 pt;
    bool found = false;
    for(int j = 0; j < path.size() && !found; j++) {
        for(int k = 0; k < 4 && !found; k++) {
            const int dx[] = {0, 0, 1, -1};
            const int dy[] = {1, -1, 0, 0};
            Coord2 pospt = path[j] + Coord2(dx[k], dy[k]);
            if(inPath(pospt, path)) {
                pt = pospt;
                found = true;
            }
        }
    }
    CHECK(found);
    return pt;
}

int getPathRelation(const vector<Coord2> &lhs, const vector<Coord2> &rhs) {
    for(int i = 0; i < lhs.size(); i++) {
        int i2 = (i + 1) % lhs.size();
        for(int j = 0; j < rhs.size(); j++) {
            int j2 = (j + 1) % rhs.size();
            if(linelineintersect(Coord4(lhs[i], lhs[i2]), Coord4(rhs[j], rhs[j2])))
                return PR_INTERSECT;
        }
    }
    bool lir = inPath(getPointIn(lhs), rhs);
    bool ril = inPath(getPointIn(lhs), rhs);
    if(!lir && !ril) {
        return PR_SEPARATE;
    } else if(lir && !ril) {
        return PR_RHSENCLOSE;
    } else if(!lir && ril) {
        return PR_LHSENCLOSE;
    } else if(lir && ril) {
        // valid result, we *should* check their areas, but I'm currently lazy
        CHECK(0);
    } else {
        // not valid result :P
        CHECK(0);
    }
}
