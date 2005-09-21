
#include "gamemap.h"

#include "gfx.h"

void Gamemap::render() const {
    CHECK(paths.size());
	setColor(0.5f, 0.5f, 0.5f);
	for(int i = 0; i < paths.size(); i++)
        drawLine(paths[i], 0.5);
}
void Gamemap::addCollide( Collider *collider ) const {
    CHECK(paths.size());
    for(int i = 0; i < paths.size(); i++)
        collider->token(paths[i], Coord4(0, 0, 0, 0));
}

vector<Coord4> Gamemap::getCollide() const {
    return paths;
}

Coord4 Gamemap::getBounds() const {
    Coord4 bounds = startCBoundBox();
    for(int i = 0; i < paths.size(); i++) {
        addToBoundBox(&bounds, Coord2(paths[i].sx, paths[i].sy));
        addToBoundBox(&bounds, Coord2(paths[i].ex, paths[i].ey));
    }
    CHECK(bounds.isNormalized());
    return bounds;
}

Gamemap::Gamemap() { };
Gamemap::Gamemap(const Level &lev) {
    CHECK(lev.paths.size());
    for(int i = 0; i < lev.paths.size(); i++) {
        for(int j = 0; j < lev.paths[i].size(); j++) {
            int k = (j + 1) % lev.paths[i].size();
            paths.push_back(Coord4(lev.paths[i][j], lev.paths[i][k]));
        }
    }
}
