
#include "dvec2.h"

#include "util.h"
#include "gfx.h"

void Parameter::update(const Button &l, const Button &r) {
    if(type == BOOLEAN) {
        if(l.repeat || r.repeat)
            bool_val = !bool_val;
    } else if(type == BOUNDED_INTEGER) {
        if(l.repeat)
            bi_val--;
        if(r.repeat)
            bi_val++;
        if(bi_val < bi_low)
            bi_val = bi_low;
        if(bi_val >= bi_high)
            bi_val = bi_high - 1;
        CHECK(bi_val >= bi_low && bi_val < bi_high);
    } else {
        CHECK(0);
    }
}

void Parameter::render(float x, float y, float h) const {
    string prefix = StringPrintf("%12s: ", name.c_str());
    if(type == BOOLEAN) {
        prefix += StringPrintf("%s", bool_val ? "true" : "false");
    } else if(type == BOUNDED_INTEGER) {
        prefix += StringPrintf("%d", bi_val);
    } else {
        CHECK(0);
    }
    drawText(prefix, h, x, y);
}

string Parameter::dumpTextRep() const {
    if(type == BOOLEAN) {
        if(hide_def && bool_val == bool_def)
            return "";
        return "  " + name + "=" + (bool_val ? "true" : "false") + "\n";
    } else if(type == BOUNDED_INTEGER) {
        if(hide_def && bi_val == bi_def)
            return "";
        return StringPrintf("  %s=%d\n", name.c_str(), bi_val);
    } else {
        CHECK(0);
    }
}

Parameter paramBool(const string &name, bool begin, bool hideDefault) {
    Parameter param;
    param.name = name;
    param.type = Parameter::BOOLEAN;
    param.hide_def = hideDefault;
    param.bool_val = begin;
    param.bool_def = begin;
    return param;
};

Parameter paramBoundint(const string &name, int begin, int low, int high, bool hideDefault) {
    Parameter param;
    param.name = name;
    param.type = Parameter::BOUNDED_INTEGER;
    param.hide_def = hideDefault;
    param.bi_val = begin;
    param.bi_def = begin;
    param.bi_low = low;
    param.bi_high = high;
    return param;
};

void Entity::initParams() {
    params.clear();
    if(type == ENTITY_TANKSTART) {
        params.push_back(paramBoundint("numerator", 0, 0, 100000, false));
        params.push_back(paramBoundint("denominator", 1, 1, 100000, false));
        params.push_back(paramBool("exist2", true, true));
        params.push_back(paramBool("exist3", true, true));
        params.push_back(paramBool("exist4", true, true));
        params.push_back(paramBool("exist5", true, true));
        params.push_back(paramBool("exist6", true, true));
        params.push_back(paramBool("exist7", true, true));
        params.push_back(paramBool("exist8", true, true));
        params.push_back(paramBool("exist9", true, true));
        params.push_back(paramBool("exist10", true, true));
        params.push_back(paramBool("exist11", true, true));
        params.push_back(paramBool("exist12", true, true));
    } else {
        CHECK(0);
    }
}

void VectorPoint::mirror() {
    swap(curvlx, curvrx);
    swap(curvly, curvry);
    swap(curvl, curvr);
}

void VectorPoint::transform(const Transform2d &ctd) {
    ctd.transform(&x, &y);
    ctd.transform(&curvlx, &curvly);
    ctd.transform(&curvrx, &curvry);
}

VectorPoint::VectorPoint() {
    x = y = 0;
    curvlx = curvly = curvrx = curvry = 16;
    curvl = curvr = false;
}

int rfg_repeats(int type, int dupe) {
    CHECK(dupe > 0);
    if(type == VECRF_SPIN)
        return dupe;
    else if(type == VECRF_SNOWFLAKE)
        return dupe * 2;
    else
        CHECK(0);
}

bool rfg_mirror(int type) {
    if(type == VECRF_SPIN)
        return false;
    else if(type == VECRF_SNOWFLAKE)
        return true;
    else
        CHECK(0);
}

Transform2d rfg_behavior(int type, int segment, int dupe, int numer, int denom) {
    if(type == VECRF_SPIN) {
        return t2d_rotate((float)segment / dupe * 2 * PI);
    } else if(type == VECRF_SNOWFLAKE) {
        int phase = segment / 2;
        bool flipped = segment % 2;
        // Step 1: We're offset from the 0 position by numer / denom. So first we rotate in that direction.
        Transform2d base;
        base *= t2d_rotate((float)numer / denom * 2 * PI);
        // Step 2: If we're flipped, we flip.
        if(flipped)
            base *= t2d_flip(0, 1, 0);
        // Step 3: Return to the 0 position.
        base *= t2d_rotate(-(float)numer / denom * 2 * PI);
        // Step 4: Rotate appropriately.
        base *= t2d_rotate((float)phase / dupe * 2 * PI);
        return base;
    } else {
        CHECK(0);
    }
}

int VectorPath::vpathCreate(int node) {
    CHECK(node >= 0 && node <= vpath.size());
    //dprintf("node is %d, vpath.size() is %d\n", vpath.size());
    pair<int, bool> can = getCanonicalNode(node);
    //dprintf("can is %d\n", can.first);
    if(can.second) can.first++;
    int phase = path.size() ? node / path.size() : 0;
    //dprintf("phase is %d\n", phase);
    // we're inserting a node to be the new can.first at dupe 0, but then we have to figure out what it ended up being at dupe #phase
    VectorPoint tv;
    tv.x = 0;
    tv.y = 0;   // this will look bad, and should be changed by the caller ASAP
    path.insert(path.begin() + can.first, tv);
    rebuildVpath();
    for(int i = 0; i < path.size(); i++) {
        pair<int, bool> ncan = getCanonicalNode(i + phase * path.size());
        if(ncan.first == can.first)
            return i + phase * path.size();
    }
    CHECK(0);
}

void VectorPath::vpathModify(int node) {
    CHECK(node >= 0 && node < vpath.size());
    VectorPoint orig = genNode(node);
    if(orig.curvl != vpath[node].curvl)
        setVRCurviness((node + vpath.size() - 1) % vpath.size(), vpath[node].curvl);
    if(orig.curvr != vpath[node].curvr)
        setVRCurviness(node, vpath[node].curvr);
    if(node >= path.size()) {
        pair<int, bool> canon = getCanonicalNode(node);
        VectorPoint tnode = vpath[node];
        if(canon.second)
            tnode.mirror();
        Transform2d trans = rfg_behavior(reflect, node / path.size(), dupes, ang_numer, ang_denom);
        trans.invert();
        tnode.transform(trans);
        path[canon.first] = tnode;
    } else {
        path[node] = vpath[node];
    }
    rebuildVpath();
}

void VectorPath::setVRCurviness(int node, bool curv) {
    pair<int, bool> canoa = getCanonicalNode(node);
    pair<int, bool> canob = getCanonicalNode((node + 1) % (path.size() * rfg_repeats(reflect, dupes)));
    //dprintf("svrc: %d, %d %d, %d %d\n", node, canoa.first, canoa.second, canob.first, canob.second);
    if(!canoa.second)
        path[canoa.first].curvr = curv;
    else
        path[canoa.first].curvl = curv;
    if(!canob.second)
        path[canob.first].curvl = curv;
    else
        path[canob.first].curvr = curv;
}

void VectorPath::vpathRemove(int node) {
    CHECK(node >= 0 && node < vpath.size());
    path.erase(path.begin() + getCanonicalNode(node).first);
    rebuildVpath();
}

VectorPoint VectorPath::genNode(int i) const {
    pair<int, bool> src = getCanonicalNode(i);
    VectorPoint srcp = path[src.first];
    if(src.second)
        srcp.mirror();
    srcp.transform(rfg_behavior(reflect, i / path.size(), dupes, ang_numer, ang_denom));
    return srcp;
}

vector<VectorPoint> VectorPath::genFromPath() const {
    vector<VectorPoint> nvpt;
    for(int i = 0; i < path.size() * rfg_repeats(reflect, dupes); i++)
        nvpt.push_back(genNode(i));
    return nvpt;
}

pair<int, bool> VectorPath::getCanonicalNode(int vnode) const {
    CHECK(vnode >= 0 && vnode <= path.size() * rfg_repeats(reflect, dupes));
    if(vnode == path.size() * rfg_repeats(reflect, dupes)) {
        if(rfg_mirror(reflect)) {
            CHECK(rfg_repeats(reflect, dupes) % 2 == 0);
            return make_pair(0, false);
        } else {
            return make_pair(path.size(), false);
        }
        CHECK(0);
    }
    CHECK(vnode < path.size() * rfg_repeats(reflect, dupes));
    int bank = vnode / path.size();
    int sub = vnode % path.size();
    CHECK(bank >= 0 && bank < rfg_repeats(reflect, dupes));
    if(bank % 2 == 1 && rfg_mirror(reflect)) {
        // this is a mirrored node
        return make_pair(path.size() - sub - 1, true);
    } else {
        // this is a normal node
        return make_pair(sub, false);
    }
}

void VectorPath::moveCenterOrReflect() {
    rebuildVpath();
}

void VectorPath::fixCurve() {
    bool changed = false;
    vector<VectorPoint> tvpath = genFromPath();
    for(int i = 0; i < tvpath.size(); i++) {
        int n = (i + 1) % tvpath.size();
        if(tvpath[i].curvr != tvpath[n].curvl) {
            dprintf("Splicing curves");
            setVRCurviness(i, true);
            changed = true;
        }
    }
    /*
    if(changed) {
        dprintf("Curves have been spliced");
        dprintf("Before:");
        for(int i = 0; i < tvpath.size(); i++)
            dprintf("%d %d", tvpath[i].curvl, tvpath[i].curvr);
        tvpath = genFromPath();
        dprintf("After:");
        for(int i = 0; i < tvpath.size(); i++)
            dprintf("%d %d", tvpath[i].curvl, tvpath[i].curvr);
        for(int i = 0; i < tvpath.size(); i++)
            CHECK(tvpath[i].curvr == tvpath[(i + 1) % tvpath.size()].curvl);
    }*/
}

void VectorPath::rebuildVpath() {
    fixCurve();
    vpath = genFromPath();
    CHECK(vpath.size() == path.size() * rfg_repeats(reflect, dupes));
    for(int i = 0; i < vpath.size(); i++)
        CHECK(vpath[i].curvr == vpath[(i + 1) % vpath.size()].curvl);
}

VectorPath::VectorPath() {
    centerx = centery = 0;
    reflect = VECRF_SPIN;
    dupes = 1;
    ang_numer = 0;
    ang_denom = 1;
}

Dvec2 loadDvec2(const char *fname) {
    Dvec2 rv;
    CHECK(0);
    /*
    ifstream fil(fname);
    CHECK(fil);
    string buf;
    while(getline(fil, buf)) {
        if(buf.size() == 0)
            break;
        vector<string> toks = tokenize(buf, " ");
        CHECK(toks.size() == 3);
        vector<int> lhc = sti(tokenize(toks[0], "(,)"));
        CHECK(lhc.size() == 0 || lhc.size() == 2);
        vector<int> mainc = sti(tokenize(toks[1], ","));
        CHECK(mainc.size() == 2);
        vector<int> rhc = sti(tokenize(toks[2], "(,)"));
        CHECK(rhc.size() == 0 || rhc.size() == 2);
        Vecpt tvecpt;
        tvecpt.x = mainc[0];
        tvecpt.y = mainc[1];
        if(lhc.size() == 2) {
            tvecpt.lhcurved = true;
            tvecpt.lhcx = lhc[0];
            tvecpt.lhcy = lhc[1];
        } else {
            tvecpt.lhcurved = false;
        }
        if(rhc.size() == 2) {
            tvecpt.rhcurved = true;
            tvecpt.rhcx = rhc[0];
            tvecpt.rhcy = rhc[1];
        } else {
            tvecpt.rhcurved = false;
        }
        rv.points.push_back(tvecpt);
    }
    int nx = 1000000;
    int ny = 1000000;
    int mx = -1000000;
    int my = -1000000;
    for(int i = 0; i < rv.points.size(); i++) {
        nx = min(nx, rv.points[i].x);
        ny = min(ny, rv.points[i].y);
        mx = max(mx, rv.points[i].x);
        my = max(my, rv.points[i].y);
    }
    CHECK(nx != 1000000);
    CHECK(ny != 1000000);
    CHECK(mx != -1000000);
    CHECK(my != -1000000);
    rv.width = mx - nx;
    rv.height = my - ny;
    for(int i = 0; i < rv.points.size(); i++) {
        rv.points[i].x -= nx;
        rv.points[i].y -= ny;
    }*/
    return rv;
}

/*
// this code should probably be salvaged sometime
    {
        static int firstrun = 1;
        if(firstrun) {
            firstrun = 0;
            // Test my matrix inversion! Wheeee
            for(int i = 0; i < VECRF_END; i++) {
                for(int j = 0; j < rfg_repeats(i, 1); j++) {
                    Transform2d orig = rfg_behavior(i, j, 1, 0, 1);
                    Transform2d inv = orig;
                    inv.invert();
                    //dprintf("-----");
                    //orig.display();
                    //dprintf("--");
                    //inv.display();
                    inv = orig * inv;
                    //dprintf("--");
                    //inv.display();
                    for(int x = 0; x < 3; x++) {
                        for(int y = 0; y < 3; y++) {
                            CHECK(fabs(inv.m[x][y] - (x == y)) < 1e-9);
                        }
                    }
                }
            }
        }
    }
*/
