
#include "coord.h"

#include <map>
#include <set>

using namespace std;

string Coord::rawstr() const {
    return StringPrintf("%08x%08x", (unsigned int)(d >> 32), (unsigned int)d);
}

Coord linelineintersectpos( Coord x1, Coord y1, Coord x2, Coord y2, Coord x3, Coord y3, Coord x4, Coord y4 ) {
	Coord denom = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );
	Coord ua = ( ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 ) ) / denom;
	Coord ub = ( ( x2 - x1 ) * ( y1 - y3 ) - ( y2 - y1 ) * ( x1 - x3 ) ) / denom;
	if( ua > 0 && ua < 1 && ub > 0 && ub < 1 )
		return ua;
	else
		return 2;
}

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

void expandBoundBox(Coord4 *bbox, Coord factor) {
    Coord x = bbox->ex - bbox->sx;
    Coord y = bbox->ey - bbox->sy;
    Coord xc = ( bbox->sx + bbox->ex ) / 2;
    Coord yc = ( bbox->sy + bbox->ey ) / 2;
    x *= factor;
    y *= factor;
    x /= 2;
    y /= 2;
    bbox->sx = xc - x;
    bbox->sy = yc - y;
    bbox->ex = xc + x;
    bbox->ey = yc + y;
}

int inPath(const Coord2 &point, const vector<Coord2> &path) {
    CHECK(path.size());
    Coord accum = 0;
    Coord cpt = getAngle(path.back() - point);
    for(int i = 0; i < path.size(); i++) {
        Coord npt = getAngle(path[i] - point);
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

bool pathReversed(const vector<Coord2> &path) {
    int rv = inPath(getPointIn(path), path);
    CHECK(rv != 0);
    return rv == -1;
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
    bool ril = inPath(getPointIn(rhs), lhs);
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

class DualLink {
public:
    Coord2 links[2][2];
    bool live[2];
    
    DualLink() {
        live[0] = live[1] = false;
    }
};

class LiveLink {
public:
    Coord2 start;
    Coord2 end;
};

const DualLink &getLink(const map<Coord2, DualLink> &vertx, Coord2 node) {
    CHECK(vertx.count(node) == 1);
    return vertx.find(node)->second;
}

bool checkConsistent(const map<Coord2, DualLink> &vertx) {
    for(int i = 0; i < 2; i++) {
        int seen = 0;
        for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++)
            if(itr->second.live[i])
                seen++;
        for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
            if(itr->second.live[i]) {
                int live = 0;
                Coord2 start = itr->first;
                Coord2 now = itr->first;
                do {
                    live++;
                    Coord2 next = getLink(vertx, now).links[i][1];
                    CHECK(getLink(vertx, next).live[i]);
                    CHECK(getLink(vertx, next).links[i][0] == now);
                    now = next;
                } while(now != start);
                CHECK(live == seen);
                break;
            }
        }
    }
    return true;
}

void printState(const map<Coord2, DualLink> &vertx) {
    //return;
    for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
        dprintf("%f, %f:\n", itr->first.x.toFloat(), itr->first.y.toFloat());
        for(int i = 0; i < 2; i++) {
            if(itr->second.live[i]) {
                dprintf("    %f, %f --> this --> %f, %f",
                            itr->second.links[i][0].x.toFloat(), itr->second.links[i][0].y.toFloat(),
                            itr->second.links[i][1].x.toFloat(), itr->second.links[i][1].y.toFloat());
            } else {
                dprintf("    NULL");
            }
        }
    }
}

vector<vector<Coord2> > getDifference(const vector<Coord2> &lhs, const vector<Coord2> &rhs) {
    map<Coord2, DualLink> vertx;
    const vector<Coord2> tv[2] = {lhs, rhs};
    //dprintf("Early parsing\n");
    for(int k = 0; k < 2; k++) {
        for(int i = 0; i < tv[k].size(); i++) {
            CHECK(vertx[tv[k][i]].live[k] == false);
            vertx[tv[k][i]].links[k][0] = tv[k][(i + tv[k].size() - 1) % tv[k].size()];
            vertx[tv[k][i]].links[k][1] = tv[k][(i + 1) % tv[k].size()];
            vertx[tv[k][i]].live[k] = true;
        }
    }
    //dprintf("First consistency\n");
    //printState(vertx);
    CHECK(checkConsistent(vertx));
    //dprintf("Passed\n");
    vector<LiveLink> links;
    for(map<Coord2, DualLink>::iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
        //dprintf("Looping, %d live links\n", links.size());
        // First we check if any lines need to be split at this point
        for(int i = 0; i < links.size(); i++) {
            if(itr->first == links[i].end)
                continue;
            if(colinear(links[i].start, itr->first, links[i].end)) {
                //dprintf("COLINEAR  %f %f  %f %f  %f %f\n",
                        //links[i].start.x.toFloat(), links[i].start.y.toFloat(),
                        //itr->first.x.toFloat(), itr->first.y.toFloat(),
                        //links[i].end.x.toFloat(), links[i].end.y.toFloat());
                for(int j = 0; j < 2; j++) {
                    if(vertx[links[i].start].live[j]) {
                        for(int k = 0; k < 2; k++) {
                            if(vertx[links[i].start].links[j][k] == links[i].end) {
                                CHECK(!vertx[itr->first].live[j]);
                                CHECK(vertx[links[i].end].live[j]);
                                vertx[itr->first].live[j] = true;
                                vertx[links[i].start].links[j][k] = itr->first;
                                vertx[itr->first].links[j][!k] = links[i].start;
                                vertx[itr->first].links[j][k] = links[i].end;
                                vertx[links[i].end].links[j][!k] = itr->first;
                            }
                        }
                    }
                }
                links[i].end = itr->first;
            }
        }
        // Next we remove all the links that end at this point
        for(int i = 0; i < links.size(); i++) {
            if(links[i].end == itr->first) {
                links.erase(links.begin() + i);
                i--;
            }
        }
        //dprintf("Culled, %d live links\n", links.size());
        // Now we calculate intersections from this point
        for(int p = 0; p < 2; p++) {
            if(!itr->second.live[p])
                continue;
            for(int k = 0; k < 2; k++) {
                if(itr->second.links[p][k] < itr->first)
                    continue;
                for(int i = 0; i < links.size(); i++) {
                    Coord ofs = linelineintersectpos(Coord4(links[i].start, links[i].end), Coord4(itr->first, itr->second.links[p][k]));
                    if(ofs != 2) {
                        // These two lines intersect.
                        // Since we know our polys aren't self-intersecting, we know they can't contain the same sets
                        // And since both of these lines must be in at least one poly, we know we must have two different nodes passing through here
                        // This is not actually true - there's an ugly edge case where they're, well, on the edge. But we'll deal with this later.
                        Coord2 junct = links[i].start + (links[i].end - links[i].start) * ofs;
                        /*dprintf("Splitting at point %f, %f\n", junct.x.toFloat(), junct.y.toFloat());
                        dprintf("%f,%f %f,%f vs %f,%f %f,%f",
                                links[i].start.x.toFloat(), links[i].start.y.toFloat(), links[i].end.x.toFloat(), links[i].end.y.toFloat(),
                                itr->first.x.toFloat(), itr->first.y.toFloat(),
                                itr->second.links[p][k].x.toFloat(), itr->second.links[p][k].y.toFloat());
                        dprintf("%s,%s %s,%s vs %s,%s %s,%s",
                                links[i].start.x.rawstr().c_str(), links[i].start.y.rawstr().c_str(), links[i].end.x.rawstr().c_str(), links[i].end.y.rawstr().c_str(),
                                itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str(),
                                itr->second.links[p][k].x.rawstr().c_str(), itr->second.links[p][k].y.rawstr().c_str());*/
                        if(junct.x < itr->first.x) {
                            CHECK((itr->first.x - junct.x) < Coord(0.00001f));   // damn accuracy
                            junct.x = itr->first.x;
                        }
                        if(junct <= itr->first) {
                            if(junct == itr->first)
                                dprintf("EQUIVALENCY? wtf!");
                            dprintf("%f,%f vs %f,%f\n", itr->first.x.toFloat(), itr->first.y.toFloat(), junct.x.toFloat(), junct.y.toFloat());
                            dprintf("%s %s vs %s %s\n", itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str(), junct.x.rawstr().c_str(), junct.y.rawstr().c_str());
                            dprintf("%f\n", ofs.toFloat());
                            CHECK(0);
                        }
                        DualLink lines;
                        lines.links[0][0] = links[i].start;
                        lines.links[0][1] = links[i].end;
                        lines.links[1][0] = itr->first;
                        lines.links[1][1] = itr->second.links[p][k];
                        bool junctadded[2] = {false, false};
                        CHECK(vertx[junct].live[0] == false);
                        CHECK(vertx[junct].live[1] == false);
                        for(int iline = 0; iline < 2; iline++) {
                            for(int inode = 0; inode < 2; inode++) {
                                for(int curve = 0; curve < 2; curve++) {
                                    for(int boe = 0; boe < 2; boe++) {
                                        if(vertx[lines.links[iline][inode]].links[curve][boe] == lines.links[iline][!inode]) {
                                            CHECK(vertx[junct].live[curve] == false || junctadded[curve] == true);
                                            vertx[junct].live[curve] = true;
                                            junctadded[curve] = true;
                                            vertx[lines.links[iline][inode]].links[curve][boe] = junct;
                                            vertx[junct].links[curve][!boe] = lines.links[iline][inode];
                                        }
                                    }
                                }
                            }
                        }
                        CHECK(checkConsistent(vertx));
                        
                        links[i].end = junct;
                    }
                }
            }
        }
        //dprintf("Intersected\n");
        CHECK(checkConsistent(vertx));
        // Now we add new links from this point
        for(int p = 0; p < 2; p++) {
            if(!itr->second.live[p])
                continue;
            for(int k = 0; k < 2; k++) {
                if(itr->second.links[p][k] < itr->first)
                    continue;
                LiveLink nll;
                nll.start = itr->first;
                nll.end = itr->second.links[p][k];
                links.push_back(nll);
            }
        }
        //dprintf("Unlooped, %d links\n", links.size());
    }
    CHECK(links.size() == 0);
    //dprintf("Done\n");
    //printState(vertx);
    CHECK(checkConsistent(vertx));
    /*
    {
        vector<vector<Coord2> > rv;
        for(int i = 0; i < 2; i++) {
            vector<Coord2> trv;
            for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
                if(itr->second.live[i]) {
                    Coord2 start = itr->first;
                    Coord2 now = itr->first;
                    do {
                        Coord2 next = getLink(vertx, now).links[i][1];
                        CHECK(getLink(vertx, next).live[i]);
                        CHECK(getLink(vertx, next).links[i][0] == now);
                        trv.push_back(now);
                        now = next;
                    } while(now != start);
                    break;
                }
            }
            rv.push_back(trv);
        }
        return rv;
    }*/
    {
        vector<vector<Coord2> > rv;
        {
            set<Coord2> seeds;
            set<pair<bool, Coord2> > seen;
            for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
                if(itr->second.live[0] && !inPath((itr->first + itr->second.links[0][1]) / 2, rhs) && !(itr->second.live[1] && itr->second.links[0][1] == itr->second.links[1][1])) {
                    //dprintf("seeding %f, %f\n", itr->second.links[0][1].x.toFloat(), itr->second.links[0][1].y.toFloat());
                    //dprintf("from %f, %f\n", itr->first.x.toFloat(), itr->first.y.toFloat());
                    //dprintf("%f, %f link compare to %f, %f\n", itr->second.links[0][1].x.toFloat(), itr->second.links[0][1].y.toFloat(), itr->second.links[1][1].x.toFloat(), itr->second.links[1][1].y.toFloat());
                    seeds.insert(itr->second.links[0][1]);
                }
            }
            for(set<Coord2>::iterator itr = seeds.begin(); itr != seeds.end(); itr++) {
                if(seen.count(make_pair(false, *itr)))
                    continue;
                vector<Coord2> tpath;
                pair<bool, Coord2> now(false, *itr);
                while(!seen.count(now)) {
                    seen.insert(now);
                    tpath.push_back(now.second);
                    if(!now.first) {
                        // came in off a lhs path - switch to rhs if there is one, and if it doesn't immediately leave the valid area
                        if(vertx[now.second].live[1] && inPath((now.second + vertx[now.second].links[1][0]) / 2, lhs)) {
                            CHECK(inPath((now.second + vertx[now.second].links[1][0]) / 2, lhs));
                            now = make_pair(true, vertx[now.second].links[1][0]);
                        } else {
                            CHECK(vertx[now.second].live[0]);
                            if(vertx[now.second].links[0][1] != vertx[now.second].links[1][0]) // parallel links cause some problems
                                CHECK(!inPath((now.second + vertx[now.second].links[0][1]) / 2, rhs));
                            now = make_pair(false, vertx[now.second].links[0][1]);
                        }
                    } else {
                        // came in off a rhs path - switch to lhs if there is one
                        if(vertx[now.second].live[0] && !inPath((now.second + vertx[now.second].links[0][1]) / 2, rhs)) {
                            CHECK(!inPath((now.second + vertx[now.second].links[0][1]) / 2, rhs));
                            now = make_pair(false, vertx[now.second].links[0][1]);
                        } else {
                            CHECK(vertx[now.second].live[1]);
                            if(vertx[now.second].links[0][1] != vertx[now.second].links[1][0]) // parallel links cause some problems
                                CHECK(inPath((now.second + vertx[now.second].links[1][0]) / 2, lhs));
                            now = make_pair(true, vertx[now.second].links[1][0]);
                        }
                    }
                }
                //dprintf("Built path of %d vertices\n", tpath.size());
                rv.push_back(tpath);
            }
        }
        return rv;
    }
}

bool colinear(const Coord2 &a, const Coord2 &b, const Coord2 &c) {
    Coord2 ab = b - a;
    Coord2 ac = c - a;
    if((ab.x == 0) != (ac.x == 0))
        return false;
    if((ab.y == 0) != (ac.y == 0))
        return false;
    return ab.x * ac.y == ac.x * ab.y;
}
