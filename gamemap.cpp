
#include "gamemap.h"

#include "gfx.h"

void Gamemap::render() const {
    CHECK(paths.size());
	setColor( 0.5f, 0.5f, 0.5f );
	for(int i = 0; i < paths.size(); i++)
        drawLinePath( paths[i], 0.5, true );
}
void Gamemap::addCollide( Collider *collider ) const {
    CHECK(paths.size());
    for(int i = 0; i < paths.size(); i++) {
        for(int j = 0; j < paths[i].size(); j++) {
            int k = (j + 1) % paths[i].size();
            collider->token(Float4(paths[i][j], paths[i][k]), Float4( 0, 0, 0, 0 ));
        }
    }
}

Float4 Gamemap::getBounds() const {
    Float4 bounds = startBoundBox();
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
    paths = lev.paths;
}
