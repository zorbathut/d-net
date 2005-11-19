
#include "coord.h"

#include <map>
#include <set>

#include "float.h"
#include "composite-imp.h"
#include "util.h"

using namespace std;

class Coords {
public:
    typedef Coord T1;
    typedef Coord2 T2;
    typedef Coord4 T4;
    
    static Coord atan2(Coord a, Coord b) { return Coord(float(::atan2(a.toFloat(), b.toFloat()))); }
    static Coord sin(Coord a) { return cfsin(a); };
    static Coord cos(Coord a) { return cfcos(a); };
};

string Coord::rawstr() const {
    return StringPrintf("%08x%08x", (unsigned int)(d >> 32), (unsigned int)d);
}

Coord coordExplicit(const string &lhs) {
    CHECK(lhs.size() == 16);
    for(int i = 0; i < lhs.size(); i++)
        CHECK(isdigit(lhs[i]) || (lhs[i] >= 'a' && lhs[i] <= 'f'));
    long long dd = 0;
    for(int i = 0; i < 16; i++) {
        dd *= 16;
        if(isdigit(lhs[i]))
            dd += lhs[i] - '0';
        else
            dd += lhs[i] - 'a' + 10;
    }
    CHECK(coordExplicit(dd).rawstr() == lhs);
    return coordExplicit(dd);
}

Float2 Coord2::toFloat() const { return Float2(x.toFloat(), y.toFloat()); }
Coord2::Coord2(const Float2 &rhs) : x(rhs.x), y(rhs.y) { };

Float4 Coord4::toFloat() const { return Float4(sx.toFloat(), sy.toFloat(), ex.toFloat(), ey.toFloat()); }

/*************
 * Computational geometry
 */

Coord len(const Coord2 &in) { return imp_len<Coords>(in); };
Coord2 normalize(const Coord2 &in) { return imp_normalize<Coords>(in); };

Coord getAngle(const Coord2 &in) { return imp_getAngle<Coords>(in); };
Coord2 makeAngle(const Coord &in) { return imp_makeAngle<Coords>(in); };

int whichSide( const Coord4 &f4, const Coord2 &pta ) { return imp_whichSide<Coords>(f4, pta); };

Coord distanceFromLine(const Coord4 &line, const Coord2 &pt) {
    Coord u = ((pt.x - line.sx) * (line.ex - line.sx) + (pt.y - line.sy) * (line.ey - line.sy)) / ((line.ex - line.sx) * (line.ex - line.sx) + (line.ey - line.sy) * (line.ey - line.sy));
    if(u < 0 || u > 1)
        return min(len(pt - Coord2(line.sx, line.sy)), len(pt - Coord2(line.ex, line.ey)));
    Coord2 ipt = Coord2(line.sx, line.sy) + Coord2(line.ex - line.sx, line.ey - line.sy) * u;
    return len(ipt - pt);
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
    int solidval = 1;
    if(accum < 0) {
        accum = -accum;
        solidval = -1;
    }
    if(accum < Coord(0.5f)) {
        CHECK(accum < Coord(0.0001f));
        return 0;
    } else {
        CHECK(accum > Coord(0.9999f) && accum < Coord(1.0001f));
        return solidval;
    }
};

bool roughInPath(const Coord2 &point, const vector<Coord2> &path, int goal) {
    int dx[] = {0, 0, 0, 1, -1, 1, 1, -1, -1};
    int dy[] = {0, 1, -1, 0, 0, 1, -1, 1, -1};
    for(int i = 0; i < 9; i++)
        if((bool)inPath(point + Coord2(dx[i], dy[i]) / 65536, path) == goal)
            return true;
    return false;
}

class GetDifferenceHandler {
public:
    
    const vector<Coord2> &lhs;
    const vector<Coord2> &rhs;
    
    GetDifferenceHandler(const vector<Coord2> &in_lhs, const vector<Coord2> &in_rhs) : lhs(in_lhs), rhs(in_rhs) { };
    
    void dsp(const vector<Coord2> &inp, string title) const {
        dprintf("string %s[%d] = {", title.c_str(), inp.size() * 2);
        for(int i = 0; i < inp.size(); i++)
            dprintf("    \"%s\", \"%s\",", inp[i].x.rawstr().c_str(), inp[i].y.rawstr().c_str());
        dprintf("};");
    }
    void operator()() const {
        dsp(lhs, "lhs");
        dsp(rhs, "rhs");
    }
};

Coord2 getPointIn(const vector<Coord2> &path) {
    // TODO: find a point inside the polygon in a better fashion
    GetDifferenceHandler CrashHandler(path, path);
    Coord2 pt;
    bool found = false;
    for(int j = 0; j < path.size() && !found; j++) {
        Coord2 pospt = (path[j] + path[(j + 1) % path.size()] + path[(j + 2) % path.size()]) / 3;
        if(inPath(pospt, path)) {
            pt = pospt;
            found = true;
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
    } else if(lir && ril && getArea(lhs) < getArea(rhs)) {
        return PR_RHSENCLOSE;
    } else if(lir && ril && getArea(lhs) > getArea(rhs)) {
        return PR_LHSENCLOSE;
    } else {
        // dammit, don't send the same two paths! we deny!
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
                    if(!getLink(vertx, next).live[i]) {
                        dprintf("Next link isn't marked as live! %d, %f, %f\n", i, itr->first.x.toFloat(), itr->first.y.toFloat());
                        return false;
                    }
                    if(getLink(vertx, next).links[i][0] != now) {
                        dprintf("Next link doesn't return to proper place! %d, %f, %f vs %f, %f\n",
                                i, itr->first.x.toFloat(), itr->first.y.toFloat(),
                                getLink(vertx, next).links[i][0].y.toFloat(), getLink(vertx, next).links[i][0].y.toFloat());
                        return false;
                    }
                    now = next;
                } while(now != start);
                if(live != seen) {
                    dprintf("Live and seen don't match up, %d has %d vs %d\n", i, live, seen);
                    return false;
                }
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

void splice(map<Coord2, DualLink> *vertx, const DualLink &lines, const Coord2 &junct, int curve) {
    int junctadded = 0;
    for(int iline = 0; iline < 2; iline++) {
        for(int inode = 0; inode < 2; inode++) {
            for(int boe = 0; boe < 2; boe++) {
                if((*vertx)[lines.links[iline][inode]].links[curve][boe] == lines.links[iline][!inode]) {
                    CHECK((*vertx)[junct].live[curve] == false || junctadded == 1);
                    CHECK(junctadded == 0 || junctadded == 1);
                    (*vertx)[junct].live[curve] = true;
                    junctadded++;
                    (*vertx)[lines.links[iline][inode]].links[curve][boe] = junct;
                    (*vertx)[junct].links[curve][!boe] = lines.links[iline][inode];
                }
            }
        }
    }
    CHECK(junctadded == 0 || junctadded == 2);
}

vector<vector<Coord2> > getDifference(const vector<Coord2> &lhs, const vector<Coord2> &rhs) {
    #if 0       // Pre-split debugging
    {
        vector<vector<Coord2> > rv;
        rv.push_back(lhs);
        rv.push_back(rhs);
        return rv;
    }
    #endif
    GetDifferenceHandler CrashHandler(lhs, rhs);
    bool lhsInside = !pathReversed(lhs);
    CHECK(!pathReversed(rhs));
    {
        int state = getPathRelation(lhs, rhs);
        if(state == PR_SEPARATE)
            return vector<vector<Coord2> >(1, lhs);
        if(state == PR_RHSENCLOSE && !pathReversed(lhs))
            return vector<vector<Coord2> >();
        if(state == PR_RHSENCLOSE && pathReversed(lhs)) {
            vector<vector<Coord2> > rv;
            rv.push_back(rhs);
            reverse(rv[0].begin(), rv[0].end());
            return rv;
        }
        if(state == PR_LHSENCLOSE) {
            dprintf("LHS Enclose! intersection ignored");
            return vector<vector<Coord2> >(1, lhs);
        }
        CHECK(state == PR_INTERSECT);
    }
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
            if(links[i].end != itr->first && colinear(Coord4(links[i].start, links[i].end), itr->first)) {
                /*
                dprintf("COLINEAR  %f %f  %f %f  %f %f\n",
                        links[i].start.x.toFloat(), links[i].start.y.toFloat(),
                        itr->first.x.toFloat(), itr->first.y.toFloat(),
                        links[i].end.x.toFloat(), links[i].end.y.toFloat());
                dprintf("COLINEAR  %s %s  %s %s  %s %s\n",
                        links[i].start.x.rawstr().c_str(), links[i].start.y.rawstr().c_str(),
                        itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str(),
                        links[i].end.x.rawstr().c_str(), links[i].end.y.rawstr().c_str());
                */
                for(int j = 0; j < 2; j++) {
                    if(vertx[links[i].start].live[j] && !vertx[itr->first].live[j]) {
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
            if(links[i].end <= itr->first) {
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
                    if(links[i].start == itr->first || links[i].start == itr->second.links[p][k] ||
                       links[i].end == itr->first || links[i].end == itr->second.links[p][k]) {
                        continue;
                    }
                    Coord ofs = linelineintersectpos(Coord4(links[i].start, links[i].end), Coord4(itr->first, itr->second.links[p][k]));
                    if(ofs != 2) {
                        // These two lines intersect.
                        // Since we know our polys aren't self-intersecting, we know they can't contain the same sets
                        // And since both of these lines must be in at least one poly, we know we must have two different nodes passing through here
                        // This is not actually true - there's an ugly edge case where they're, well, on the edge. But we'll deal with this later.
                        Coord2 junct = links[i].start + (links[i].end - links[i].start) * ofs;
                        DualLink lines;
                        lines.links[0][0] = links[i].start;
                        lines.links[0][1] = links[i].end;
                        lines.links[1][0] = itr->first;
                        lines.links[1][1] = itr->second.links[p][k];
                        
                        /*
                        dprintf("Splitting at point %f, %f\n", junct.x.toFloat(), junct.y.toFloat());
                        dprintf("%f,%f %f,%f vs %f,%f %f,%f",
                                links[i].start.x.toFloat(), links[i].start.y.toFloat(), links[i].end.x.toFloat(), links[i].end.y.toFloat(),
                                itr->first.x.toFloat(), itr->first.y.toFloat(),
                                itr->second.links[p][k].x.toFloat(), itr->second.links[p][k].y.toFloat());
                        dprintf("%s,%s %s,%s vs %s,%s %s,%s",
                                links[i].start.x.rawstr().c_str(), links[i].start.y.rawstr().c_str(), links[i].end.x.rawstr().c_str(), links[i].end.y.rawstr().c_str(),
                                itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str(),
                                itr->second.links[p][k].x.rawstr().c_str(), itr->second.links[p][k].y.rawstr().c_str());
                        dprintf("%s %s\n", junct.x.rawstr().c_str(), junct.y.rawstr().c_str());*/
                        {
                            Coord2 closest;
                            Coord dist = 1000000000;
                            for(int lin = 0; lin < 2; lin++) {
                                for(int nod = 0; nod < 2; nod++) {
                                    Coord2 diff = lines.links[lin][nod] - junct;
                                    if(len(diff) < dist) {
                                        dist = len(diff);
                                        closest = lines.links[lin][nod];
                                    }
                                }
                            }
                            if(dist < Coord(0.000001f)) {
                                // One of these already exists, so a line must go through it
                                // but we're planning to merge, so a line must only go through one of 'em
                                CHECK(vertx[closest].live[0] != vertx[closest].live[1]);
                                int usedlin = vertx[closest].live[1];
                                
                                CHECK(!vertx[closest].live[!usedlin]);
                                CHECK(vertx[closest].live[usedlin]);
                                
                                splice(&vertx, lines, closest, !usedlin);
                                
                                links[i].end = closest;
                                
                                //CHECK(checkConsistent(vertx));
                                continue;
                            }
                        }

                        if(junct.x < itr->first.x) {
                            CHECK((itr->first.x - junct.x) < Coord(0.00001f));   // damn accuracy
                            junct.x = itr->first.x;
                        }
                        if(junct <= itr->first) {
                            if(junct == itr->first)
                                dprintf("EQUIVALENCY? wtf!");
                            dprintf("%f,%f vs %f,%f\n", itr->first.x.toFloat(), itr->first.y.toFloat(), junct.x.toFloat(), junct.y.toFloat());
                            dprintf("%s %s vs %s %s\n", itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str(), junct.x.rawstr().c_str(), junct.y.rawstr().c_str());
                            dprintf("%s\n", ofs.rawstr().c_str());
                            dprintf("%f\n", ofs.toFloat());
                            CHECK(0);
                        }

                        CHECK(vertx[junct].live[0] == false);
                        CHECK(vertx[junct].live[1] == false);
                        splice(&vertx, lines, junct, 0);
                        splice(&vertx, lines, junct, 1);
                        
                        //CHECK(checkConsistent(vertx));
                        
                        links[i].end = junct;
                    }
                }
            }
        }
        //dprintf("Intersected\n");
        //CHECK(checkConsistent(vertx));
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
    #if 0   // This code intercepts the "split" version and returns it as the results - good for debugging
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
    }
    #endif
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
                    //{
                        //if(!vertx[itr->second.links[0][1]].live[1]) {
                            //Coord2 tp = (itr->second.links[0][1] + vertx[itr->second.links[0][1]].links[0][1]) / 2;
                            //dprintf("%s, %s\n", tp.x.rawstr().c_str(), tp.y.rawstr().c_str());
                            //CHECK(roughInPath(tp, rhs, false));
                        //}
                    //}
                    seeds.insert(itr->second.links[0][1]);
                }
            }
            for(set<Coord2>::iterator itr = seeds.begin(); itr != seeds.end(); itr++) {
                if(seen.count(make_pair(false, *itr)))
                    continue;
                vector<Coord2> tpath;
                pair<bool, Coord2> now(false, *itr);
                //dprintf("Seeding at %f, %f\n", now.second.x.toFloat(), now.second.y.toFloat());
                while(!seen.count(now)) {
                    seen.insert(now);
                    tpath.push_back(now.second);
                    /*
                    dprintf("  %f, %f, %d:\n", now.second.x.toFloat(), now.second.y.toFloat(), now.first);
                    for(int i = 0; i < 2; i++) {
                        if(vertx[now.second].live[i]) {
                            dprintf("    %f, %f --> this --> %f, %f",
                                        vertx[now.second].links[i][0].x.toFloat(), vertx[now.second].links[i][0].y.toFloat(),
                                        vertx[now.second].links[i][1].x.toFloat(), vertx[now.second].links[i][1].y.toFloat());
                        } else {
                            dprintf("    NULL");
                        }
                    }*/
                    if(!now.first) {
                        // came in off a lhs path - switch to rhs if there is one, and if it doesn't immediately leave the valid area
                        if(vertx[now.second].live[1] && inPath((now.second + vertx[now.second].links[1][0]) / 2, lhs) == lhsInside) {
                            CHECK(inPath((now.second + vertx[now.second].links[1][0]) / 2, lhs) == lhsInside);
                            now = make_pair(true, vertx[now.second].links[1][0]);
                        } else {
                            CHECK(vertx[now.second].live[0]);
                            if(!vertx[now.second].live[1] || vertx[now.second].links[0][1] != vertx[now.second].links[1][0]) // parallel links cause some problems
                                CHECK(roughInPath((now.second + vertx[now.second].links[0][1]) / 2, rhs, false));
                            now = make_pair(false, vertx[now.second].links[0][1]);
                        }
                    } else {
                        // came in off a rhs path - switch to lhs if there is one
                        if(vertx[now.second].live[0] && !inPath((now.second + vertx[now.second].links[0][1]) / 2, rhs)) {
                            CHECK(!inPath((now.second + vertx[now.second].links[0][1]) / 2, rhs));
                            now = make_pair(false, vertx[now.second].links[0][1]);
                        } else {
                            CHECK(vertx[now.second].live[1]);
                            if(!vertx[now.second].live[0] || vertx[now.second].links[0][1] != vertx[now.second].links[1][0]) // parallel links cause some problems
                                CHECK(roughInPath((now.second + vertx[now.second].links[1][0]) / 2, lhs, lhsInside));
                            now = make_pair(true, vertx[now.second].links[1][0]);
                        }
                    }
                }
                //dprintf("Built path of %d vertices\n", tpath.size());
                if(tpath.size() > 2)
                    rv.push_back(tpath);
            }
        }
        return rv;
    }
}

Coord getArea(const vector<Coord2> &are) {
    Coord totare = 0;
    for(int i = 0; i < are.size(); i++) {
        int j = (i + 1) % are.size();
        totare += are[i].x * are[j].y - are[j].x * are[i].y;
    }
    totare /= 2;
    dprintf("%f\n", totare.toFloat());
    return totare;
}

bool colinear(const Coord4 &line, const Coord2 &pt) {
    Coord koord = distanceFromLine(line, pt);
    return koord < Coord(0.00001f);
}

/*************
 * Bounding box
 */

Coord4 startCBoundBox() { return imp_startBoundBox<Coords>(); };

void addToBoundBox(Coord4 *bbox, Coord x, Coord y) { return imp_addToBoundBox<Coords>(bbox, x, y); };
void addToBoundBox(Coord4 *bbox, const Coord2 &point) { return imp_addToBoundBox<Coords>(bbox, point); };
void addToBoundBox(Coord4 *bbox, const Coord4 &rect) { return imp_addToBoundBox<Coords>(bbox, rect); };

void expandBoundBox(Coord4 *bbox, Coord factor) { return imp_expandBoundBox<Coords>(bbox, factor); };

/*************
 * Math
 */

bool linelineintersect( const Coord4 &lhs, const Coord4 &rhs ) { return imp_linelineintersect<Coords>(lhs, rhs); };
Coord linelineintersectpos( const Coord4 &lhs, const Coord4 &rhs ) { return imp_linelineintersectpos<Coords>(lhs, rhs); };
