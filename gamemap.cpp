
#include "gamemap.h"

#include "gfx.h"
#include "args.h"
#include "rng.h"

DECLARE_bool(debugGraphics);

void Gamemap::render() const {
    CHECK(paths.size());
	setColor(0.5f, 0.5f, 0.5f);
    for(int i = 0; i < paths.size(); i++) {
        for(int j = 0; j < paths[i].size(); j++) {
            int k = (j + 1) % paths[i].size();
            drawLine(paths[i][j], paths[i][k], 0.5);
            if(FLAGS_debugGraphics) {
                int m = (j + 2) % paths[i].size();
                Coord2 jk = paths[i][j] - paths[i][k];
                Coord2 mk = paths[i][m] - paths[i][k];
                Coord ja = getAngle(jk);
                Coord ma = getAngle(mk);
                Coord ad = ma - ja;
                while(ad < 0)
                    ad += COORDPI * 2;
                Coord2 out = makeAngle(ja + ad / 2) * 5;
                //dprintf("%f, %f\n", out.x.toFloat(), out.y.toFloat());
                drawLine(paths[i][k], paths[i][k] + out, 2.0);
            }
        }
    }
}
void Gamemap::addCollide(Collider *collider) const {
    vector<Coord4> collide;
    for(int i = 0; i < paths.size(); i++) {
        for(int j = 0; j < paths[i].size(); j++) {
            int k = (j + 1) % paths[i].size();
            collider->token(Coord4(paths[i][j], paths[i][k]));
        }
    }

}

const vector<vector<Coord2> > &Gamemap::getCollide() const {
    return paths;
}

Coord4 Gamemap::getBounds() const {
    Coord4 bounds = startCBoundBox();
    for(int i = 0; i < paths.size(); i++) {
        for(int j = 0; j < paths[i].size(); j++) {
            addToBoundBox(&bounds, paths[i][j]);
        }
    }
    CHECK(bounds.isNormalized());
    return bounds;
}

void Gamemap::removeWalls(Coord2 center, float radius) {
    vector<vector<Coord2> > oat;
    vector<Coord2> inters;
    {
        vector<float> rv;
        int vct = int(frand() * 3) + 3;
        float ofs = frand() * 2 * PI / vct;
        float maxofs = 2 * PI / vct / 2;
        for(int i = 0; i < vct; i++) {
            rv.push_back(i * 2 * PI / vct + ofs + powerRand(2) * maxofs);
        }
        for(int i = 0; i < rv.size(); i++)
            inters.push_back(center + makeAngle(Coord(rv[i])) * Coord(radius));
    }
    CHECK(!pathReversed(inters));
    for(int i = 0; i < paths.size(); i++) {
        vector<vector<Coord2> > ntp = getDifference(paths[i], inters);
        for(int i = 0; i < ntp.size(); i++)
            oat.push_back(ntp[i]);
    }
    paths = oat;
}

Gamemap::Gamemap() { };
Gamemap::Gamemap(const Level &lev) {
    CHECK(lev.paths.size());
    paths = lev.paths;
}
