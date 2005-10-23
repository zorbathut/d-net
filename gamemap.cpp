
#include "gamemap.h"

#include "gfx.h"
#include "args.h"

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
void Gamemap::addCollide( Collider *collider ) const {
    vector<Coord4> collide = getCollide();
    for(int i = 0; i < collide.size(); i++)
        collider->token(collide[i], Coord4(0, 0, 0, 0));
}

vector<Coord4> Gamemap::getCollide() const {
    vector<Coord4> collide;
    for(int i = 0; i < paths.size(); i++) {
        for(int j = 0; j < paths[i].size(); j++) {
            int k = (j + 1) % paths[i].size();
            collide.push_back(Coord4(paths[i][j], paths[i][k]));
        }
    }
    return collide;
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

Gamemap::Gamemap() { };
Gamemap::Gamemap(const Level &lev) {
    CHECK(lev.paths.size());
    paths = lev.paths;
}
