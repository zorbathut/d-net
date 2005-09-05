
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

