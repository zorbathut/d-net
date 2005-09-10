
#include "level.h"

Level loadLevel(const string &str) {
    dprintf("Loading %s\n", str.c_str());
    Level rv;
    Dvec2 dv = loadDvec2(str);
    for(int i = 0; i < dv.paths.size(); i++) {
        vector<Float2> tp;
        for(int j = 0; j < dv.paths[i].vpath.size(); j++) {
            CHECK(dv.paths[i].vpath[j].curvl == false && dv.paths[i].vpath[j].curvr == false);
            tp.push_back(Float2(dv.paths[i].vpath[j].x + dv.paths[i].centerx, dv.paths[i].vpath[j].y + dv.paths[i].centery));
        }
        tp.erase(unique(tp.begin(), tp.end()), tp.end());
        CHECK(tp.size());
        if(tp.back() == tp[0])
            tp.pop_back();
        CHECK(tp.size());
        {
            set<Float2> flup(tp.begin(), tp.end());
            CHECK(flup.size() == tp.size());
        }
        rv.paths.push_back(tp);
    }
    dprintf("%d paths parsed\n", rv.paths.size());
    CHECK(rv.paths.size() == dv.paths.size());
    {
        map<int, int> entallow;
        for(int i = 0; i < dv.entities.size(); i++) {
            for(int j = 2; j <= dv.entities.size(); j++) {
                string estr = StringPrintf("exist%d", j);
                CHECK(dv.entities[i].getParameter(estr) && dv.entities[i].getParameter(estr)->bool_val == true);
                entallow[j]++;
            }
        }
        for(map<int, int>::iterator itr = entallow.begin(); itr != entallow.end(); itr++) {
            if(itr->second >= itr->first) {
                dprintf("Entities valid for %d players, got %d starts", itr->first, itr->second);
                rv.playersValid.insert(itr->first);
                for(int i = 0; i < dv.entities.size(); i++) {
                    CHECK(dv.entities[i].getParameter("numerator") && dv.entities[i].getParameter("denominator"));
                    rv.playerStarts[itr->first].push_back(make_pair(Float2(dv.entities[i].x, dv.entities[i].y), PI * 2 * dv.entities[i].getParameter("numerator")->bi_val / dv.entities[i].getParameter("denominator")->bi_val));
                }
                
            }
        }
    }
    return rv;
}
