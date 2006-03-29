
#include "coord.h"
#include "util.h"

#include <vector>
#include <set>
#include <map>

using namespace std;

bool dumpBooleanDetail = true;

class GetDifferenceHandler : public StackPrinter {
public:
  
  const vector<Coord2> &lhs;
  const vector<Coord2> &rhs;
  
  GetDifferenceHandler(const vector<Coord2> &in_lhs, const vector<Coord2> &in_rhs) : lhs(in_lhs), rhs(in_rhs) { };
  
  void dsp(const vector<Coord2> &inp, string title) const {
    dprintf("  string %s[%d] = {", title.c_str(), inp.size() * 2);
    for(int i = 0; i < inp.size(); i++)
      dprintf("    \"%s\", \"%s\",", inp[i].x.rawstr().c_str(), inp[i].y.rawstr().c_str());
    dprintf("  };");
  }
  virtual void Print() const {
    if(dumpBooleanDetail) {
      dprintf("  Coord error! Data follows:");
      dsp(lhs, "lhs");
      dsp(rhs, "rhs");
    } else {
      dprintf("  Coord error! Not dumping.");
    }
  }
};

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

  string bigstring() const {
    return StringPrintf("%s %s %s %s", start.x.rawstr().c_str(), start.y.rawstr().c_str(), end.x.rawstr().c_str(), end.y.rawstr().c_str());
  }
  static string bignullstring() {
    LiveLink foo;
    foo.start = Coord2(0, 0);
    foo.end = Coord2(0, 0);
    return foo.bigstring();
  }
};

bool operator<(const LiveLink &lhs, const LiveLink &rhs) {
  if(lhs.start != rhs.start) return lhs.start < rhs.start;
  return lhs.end < rhs.end;
}

bool operator==(const LiveLink &lhs, const LiveLink &rhs) {
  return lhs.start == rhs.start && lhs.end == rhs.end;
}

const DualLink &getLink(const map<Coord2, DualLink> &vertx, Coord2 node) {
  CHECK(vertx.count(node) == 1);
  return vertx.find(node)->second;
}

bool checkConsistent(const map<Coord2, DualLink> &vertx, const vector<LiveLink> &links) {
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
  for(int i = 0; i < links.size(); i++) {
    bool validated = false;
    for(int v = 0; v < 2; v++)
      for(int d = 0; d < 2; d++)
        if(getLink(vertx, links[i].start).live[v] && getLink(vertx, links[i].start).links[v][d] == links[i].end)
          validated = true;
    if(!validated) {
      dprintf("Current live link doesn't match up in any expected way");
      return false;
    }
  }
  return true;
}

void checkLineConsistency(const map<Coord2, DualLink> &vertx, const vector<LiveLink> &links, const Coord2 &cpos) {
  vector<LiveLink> corrlinks;
  for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
    for(int i = 0; i < 2; i++) {
      if(itr->second.live[i]) {
        for(int s = 0; s < 2; s++) {
          Coord2 ptz[2];
          ptz[0] = itr->second.links[i][s];
          ptz[1] = itr->first;
          sort(ptz, ptz + 2);
          if(ptz[0] <= cpos && ptz[1] > cpos) {
            LiveLink ll;
            ll.start = ptz[0];
            ll.end = ptz[1];
            corrlinks.push_back(ll);
          }
        }
      }
    }
  }
  vector<LiveLink> tlinks = links;
  
  sort(corrlinks.begin(), corrlinks.end());
  corrlinks.erase(unique(corrlinks.begin(), corrlinks.end()), corrlinks.end());
  
  sort(tlinks.begin(), tlinks.end());
  CHECK(unique(tlinks.begin(), tlinks.end()) == tlinks.end());
  
  if(corrlinks != tlinks) {
    int crlp = 0;
    int tlp = 0;
    dprintf("Here is %s %s\n", cpos.x.rawstr().c_str(), cpos.y.rawstr().c_str());
    while(crlp != corrlinks.size() && tlp != tlinks.size()) {
      if(crlp != corrlinks.size() && tlp != tlinks.size() && corrlinks[crlp] == tlinks[tlp]) {
        dprintf("%s <==> %s\n", corrlinks[crlp].bigstring().c_str(), tlinks[tlp].bigstring().c_str());
        crlp++;
        tlp++;
      } else if(tlp == tlinks.size() || crlp != corrlinks.size() && corrlinks[crlp] < tlinks[tlp]) {
        dprintf("%s <=== %s\n", corrlinks[crlp].bigstring().c_str(), LiveLink::bignullstring().c_str());
        crlp++;
      } else {
        dprintf("%s ===> %s\n", LiveLink::bignullstring().c_str(), tlinks[tlp].bigstring().c_str());
        tlp++;
      }
    }
    CHECK(0);
  }
};

void printNode(const Coord2 &coord, const DualLink &link) {
  dprintf("%f, %f (%s, %s):\n", coord.x.toFloat(), coord.y.toFloat(), coord.x.rawstr().c_str(), coord.y.rawstr().c_str());
  for(int i = 0; i < 2; i++) {
    if(link.live[i]) {
      dprintf("  %f, %f --> this --> %f, %f",
            link.links[i][0].x.toFloat(), link.links[i][0].y.toFloat(),
            link.links[i][1].x.toFloat(), link.links[i][1].y.toFloat());
      dprintf("  %s, %s --> this --> %s, %s",
            link.links[i][0].x.rawstr().c_str(), link.links[i][0].y.rawstr().c_str(),
            link.links[i][1].x.rawstr().c_str(), link.links[i][1].y.rawstr().c_str());
    } else {
      dprintf("  NULL");
    }
  }
}

void printState(const map<Coord2, DualLink> &vertx) {
  for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
    printNode(itr->first, itr->second);
  }
}

void splice(map<Coord2, DualLink> *vertx, const DualLink &lines, const Coord2 &junct, int curve) {
  /*
  for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++)
      printNode(lines.links[x][y], (*vertx)[lines.links[x][y]]);
  */
  int junctadded = 0;
  for(int iline = 0; iline < 2; iline++) {
    for(int inode = 0; inode < 2; inode++) {
      for(int boe = 0; boe < 2; boe++) {
        if((*vertx)[lines.links[iline][inode]].links[curve][boe] == lines.links[iline][!inode] && (*vertx)[lines.links[iline][inode]].live[curve]) {
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

vector<vector<Coord2> > createSplitLines(const map<Coord2, DualLink> &vertx) {
  {
    vector<LiveLink> llv;
    CHECK(checkConsistent(vertx, llv));
  }
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

vector<vector<Coord2> > mergeAndSplit(const vector<Coord2> &in) {
  if(set<Coord2>(in.begin(), in.end()).size() != in.size()) {
    dprintf("Loop detected, going insane");
    Coord2 dupe;
    {
      set<Coord2> tvta;
      for(int i = 0; i < in.size(); i++) {
        if(tvta.count(in[i]))
          dupe = in[i];
        tvta.insert(in[i]);
      }
      if(tvta.size() == in.size()) {
        dprintf("what the christ");
        CHECK(0);
      }
    }
    for(int i = 0; i < in.size(); i++) {
      if(in[i] == dupe) {
        vector<Coord2> cl;
        vector<vector<Coord2> > acu;
        for(int k = 1; k <= in.size(); k++) {
          cl.push_back(in[(i + k) % in.size()]);
          if(in[(i + k) % in.size()] == dupe) {
            acu.push_back(cl);
            cl.clear();
          }
        }
        CHECK(cl.size() == 0);
        return acu;
      }
    }
    CHECK(0);
  } else {
    return vector<vector<Coord2> >(1, in);
  }
}

void deloop(vector<Coord2> *in) {
  CHECK(in->size() >= 3);
  while(1) {
    bool found = false;
    for(int i = 0; i < in->size(); i++) {
      if((*in)[i] == (*in)[(i + 2) % in->size()]) {
        int rma = i;
        int rmb = (i + 1) % in->size();
        if(rma < rmb)
          swap(rma, rmb);
        in->erase(in->begin() + rma);
        in->erase(in->begin() + rmb);
        found = true;
        dprintf("Delooped!\n");
      }
      if((*in)[i] == (*in)[(i + 1) % in->size()]) {
        in->erase(in->begin() + i);
        found = true;
        dprintf("Deduped!\n");
      }
    }
    if(!found)
      break;
  }
}

const int megaverbose = 0;

Coord2 truncate(const Coord2 &c2, int bits) {
  return Coord2((c2.x + (coordExplicit(1) << (bits - 1))) >> bits << bits, (c2.y + (coordExplicit(1) << (bits - 1))) >> bits << bits);
}

//DEFINE_string(getdiffstorage, "", "Where to store an archive of all the getDifference calls for profiling purposes");
//FILE *gds = NULL;

bool getDifferenceInstaCrashy = false;

vector<vector<Coord2> > getDifference(const vector<Coord2> &lhs, const vector<Coord2> &rhs) {
  #if 0    // Pre-split debugging
  {
    vector<vector<Coord2> > rv;
    rv.push_back(lhs);
    rv.push_back(rhs);
    return rv;
  }
  #endif
  GetDifferenceHandler gdhst(lhs, rhs);
  if(getDifferenceInstaCrashy)
    CHECK(0);
  //if(frameNumber == 921696)
    //CrashHandler();
  bool lhsInside = !pathReversed(lhs);
  CHECK(!pathReversed(rhs));
  #if 1
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
      //dprintf("LHS Enclose! intersection ignored");
      return vector<vector<Coord2> >(1, lhs);
    }
    CHECK(state == PR_INTERSECT);
  }
  map<Coord2, DualLink> vertx;
  vector<Coord2> tv[2] = {lhs, rhs};
  {
    // Let's be crazy and strip off a few bits of precision.
    for(int i = 0; i < 2; i++) {
      for(int k = 0; k < tv[i].size(); k++) {
        tv[i][k] = truncate(tv[i][k], 12);
      }
    }
  }
  /*  // I don't think this is necessary anymore because of the truncation
  while(1) {
    const Coord mergebounds = Coord(0.00001f);
    bool changed = false;
    vector<Coord2> allvtx = tv[0];
    allvtx.insert(allvtx.end(), tv[1].begin(), tv[1].end());
    sort(allvtx.begin(), allvtx.end());
    allvtx.erase(unique(allvtx.begin(), allvtx.end()), allvtx.end());
    for(int i = 0; i < allvtx.size() && !changed; i++) {
      for(int d = -1; d <= 1 && !changed; d += 2) {
        for(int j = i + d; j >= 0 && j < allvtx.size() && !changed; j += d) {
          if(abs(allvtx[i].x - allvtx[j].x) > mergebounds)
            break;
          if(len(allvtx[i] - allvtx[j]) < mergebounds) {
            dprintf("Holy crap, merging! %s,%s vs %s,%s (diff %s,%s) (ofs %d, %d)\n",
                allvtx[i].x.rawstr().c_str(), allvtx[i].y.rawstr().c_str(),
                allvtx[j].x.rawstr().c_str(), allvtx[j].y.rawstr().c_str(),
                abs(allvtx[i].x - allvtx[j].x).rawstr().c_str(),
                abs(allvtx[i].y - allvtx[j].y).rawstr().c_str(),
                i, j);
            Coord2 dest = (allvtx[i] + allvtx[j]) / 2;
            int foundcount[2] = {0, 0};
            for(int p = 0; p < 2; p++) {
              for(int n = 0; n < tv[p].size(); n++) {
                if(tv[p][n] == allvtx[i] || tv[p][n] == allvtx[j]) {
                  tv[p][n] = dest;
                  foundcount[p]++;
                }
              }
              tv[p].erase(unique(tv[p].begin(), tv[p].end()), tv[p].end());
              while(tv[p].back() == tv[p][0])
                tv[p].pop_back();
            }
            CHECK(foundcount[0] + foundcount[1] >= 2);
            allvtx.erase(allvtx.begin() + i);
            if(j > i)
              j--;
            allvtx.erase(allvtx.begin() + j);
            allvtx.push_back(dest);
            changed = true;
          }
        }
      }
    }
    if(!changed)
      break;
  }
  */
  deloop(&tv[0]);
  deloop(&tv[1]);
  if(set<Coord2>(tv[0].begin(), tv[0].end()).size() != tv[0].size()) {
    map<Coord2, int> ct;
    for(int i = 0; i < tv[0].size(); i++) {
      ct[tv[0][i]]++;
      dprintf("%s, %s\n", tv[0][i].x.rawstr().c_str(), tv[0][i].y.rawstr().c_str());
      if(ct[tv[0][i]] > 1)
        dprintf("Dupe at %s, %s\n", tv[0][i].x.rawstr().c_str(), tv[0][i].y.rawstr().c_str());
    }
  }
  CHECK(set<Coord2>(tv[0].begin(), tv[0].end()).size() == tv[0].size());
  CHECK(set<Coord2>(tv[1].begin(), tv[1].end()).size() == tv[1].size());
  //dprintf("Early parsing\n");
  for(int k = 0; k < 2; k++) {
    for(int i = 0; i < tv[k].size(); i++) {
      CHECK(vertx[tv[k][i]].live[k] == false);
      vertx[tv[k][i]].links[k][0] = tv[k][(i + tv[k].size() - 1) % tv[k].size()];
      vertx[tv[k][i]].links[k][1] = tv[k][(i + 1) % tv[k].size()];
      vertx[tv[k][i]].live[k] = true;
    }
  }
  //return createSplitLines(vertx);
  //dprintf("First consistency\n");
  //printState(vertx);
  //dprintf("Passed\n");
  vector<LiveLink> links;
  CHECK(checkConsistent(vertx, links));
  for(map<Coord2, DualLink>::iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
    
    if(!itr->second.live[0] && !itr->second.live[1]) {
      dprintf("Dead node\n");
      continue;
    }
    
    //printNode(itr->first, itr->second);
    //dprintf("Looping, %d live links\n", links.size());
    // First we check if any lines need to be split at this point
    for(int i = 0; i < links.size(); i++) {
      if(itr->first == links[i].end)
        continue;
      //{
        //Coord dist = distanceFromLine(Coord4(links[i].start, links[i].end), itr->first);
        //if(dist < Coord(0.01))
          //dprintf("Colin %f at vert %s %s\n", dist.toFloat(), itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str());
      //}
      if(links[i].end != itr->first && colinear(Coord4(links[i].start, links[i].end), itr->first)) {
        
        dprintf("COLINEAR  %f %f  %f %f  %f %f\n",
            links[i].start.x.toFloat(), links[i].start.y.toFloat(),
            itr->first.x.toFloat(), itr->first.y.toFloat(),
            links[i].end.x.toFloat(), links[i].end.y.toFloat());
        dprintf("COLINEAR  %s %s  %s %s  %s %s\n",
            links[i].start.x.rawstr().c_str(), links[i].start.y.rawstr().c_str(),
            itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str(),
            links[i].end.x.rawstr().c_str(), links[i].end.y.rawstr().c_str());
        
        if(megaverbose)
          dprintf("  Combining colinear\n");
        for(int j = 0; j < 2; j++) {
          if(vertx[links[i].start].live[j] && vertx[links[i].end].live[j] && !vertx[itr->first].live[j]) {
            // If there exists a line that passes through this vertex, and this vertex isn't on it . . .
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
            CHECK(links[i].end != itr->first);
            links[i].end = itr->first;
          }
        }
      }
    }
    // Next we remove all the links that end at this point
    for(int i = 0; i < links.size(); i++) {
      if(links[i].end == itr->first) {
        bool found = false;      
        for(int p = 0; p < 2; p++)
          for(int k = 0; k < 2; k++)
            if(itr->second.links[p][k] == links[i].start)
              found = true;
        CHECK(found);
        //CHECK(checkConsistent(vertx, links));
        if(megaverbose)
          dprintf("  Removing link %f,%f %f,%f\n", links[i].start.x.toFloat(), links[i].start.y.toFloat(), links[i].end.x.toFloat(), links[i].end.y.toFloat());
        links.erase(links.begin() + i);
        i--;
        
      }
    }
    //dprintf("Culled, %d live links\n", links.size());
    // Next we combine any vertices linked from this one that are really really close
    /*for(int i = 0; i < 2; i++) {
      if(!itr->second.live[i])
        continue;
      for(int j = 0; j < 2; j++) {
        dprintf("%s\n", len(itr->second.links[i][j] - itr->first).rawstr().c_str());
        if(len(itr->second.links[i][j] - itr->first) < Coord(0.00001f)) {
          dprintf("Combining close nodes\n");
          vertx[itr->second.links[i][j]].live[i] = false;
          vertx[vertx[itr->second.links[i][j]].links[i][j]].links[i][!j] = itr->first;
          itr->second.links[i][j] = vertx[itr->second.links[i][j]].links[i][j];
        }
      }
    }
    dprintf("Done combining\n");*/
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
            dprintf("%s %s\n", junct.x.rawstr().c_str(), junct.y.rawstr().c_str());
            */
            
            {
              Coord2 closest;
              Coord dist = 1000000000;
              for(int lin = 0; lin < 2; lin++) {
                for(int nod = 0; nod < 2; nod++) {
                  Coord2 diff = lines.links[lin][nod] - junct;
                  if(len(diff) < dist && lines.links[lin][nod] >= itr->first) {
                    dist = len(diff);
                    closest = lines.links[lin][nod];
                  }
                }
              }
              if(dist < Coord(0.000001f)) {
                if(megaverbose)
                  dprintf("  Merging to %s, %s from %s, %s\n", closest.x.rawstr().c_str(), closest.y.rawstr().c_str(), itr->first.x.rawstr().c_str(), itr->first.y.rawstr().c_str());
                // One of these already exists, so a line must go through it
                // but we're planning to merge, so a line must only go through one of 'em
                CHECK(closest >= itr->first);
                if(!(vertx[closest].live[0] != vertx[closest].live[1])) {
                  dprintf("Crazy collective splice\n");
                  // Whups, we've got a bit of a problem - we're merging with a point that already exists.
                  // This can happen (for example) when we have a long horizontal line and we're making an extremely
                  // sharp angle on a vertical V.
                  // Note that we've already, in theory, created a point at one intersection of the V and the horizontal line.
                  // Solution: Check to see if we're forming a V, and if we are, chop off the top of it.
                  CHECK(vertx[closest].live[0] && vertx[closest].live[1]);
                  
                  int changed = 0;
                  for(int q = 0; q < 2; q++) {
                    for(int w = 0; w < 2; w++) {
                      // This would be the point of the V.
                      Coord2 nexus = lines.links[q][w];
                      for(int v = 0; v < 2; v++) {
                        if(!itr->second.live[v])
                            continue;
                        for(int m = 0; m < 2; m++) {
                            // What we should have is vertx[nexus].links[v][!m] -> nexus -> vertx[nexus].links[v][m]
                            // and this should be the *same* as closest -> lines.links[q][w] -> lines.links[q][!w]
                            // This is also the same as V-horiz-intersection -> V-point -> other-point
                            if(vertx[nexus].links[v][!m] != closest)
                                continue;
                            if(vertx[nexus].links[v][m] != lines.links[q][!w])
                                continue;
                            
                            CHECK(changed == 0);
                            changed++;
                            
                            // Now we splice this together to be vertx[nexus].links[v][!m] -> vertx[nexus].links[v][m],
                            // effectively chopping off the tip of the v entirely
                            vertx[nexus].live[v] = false;
                            vertx[vertx[nexus].links[v][!m]].links[v][m] = vertx[nexus].links[v][m];
                            vertx[vertx[nexus].links[v][m]].links[v][!m] = vertx[nexus].links[v][!m];
                            
                            // And update our links
                            if(!q) {
                              // If q is false, we've spliced links[i].start and links[i].end
                              if(!w) {
                                // lines.links[q][w] == links[i].start
                                // lines.links[q][!w] == links[i].end
                                // links[i].start is now missing, so this line should vanish, along with the other one that starts here
                                CHECK(vertx[links[i].start].live[0] == false && vertx[links[i].start].live[1] == false);
                                int removed = 0;
                                for(int k = 0; k < links.size(); k++) {
                                  if(links[k].start == lines.links[q][w]) {
                                    links.erase(links.begin() + k);
                                    if(k <= i)
                                      i--;
                                    k--;
                                    removed++;
                                  }
                                }
                                CHECK(removed == 2);
                                CHECK(closest > itr->first);  // since otherwise we need to add more lines
                              } else {
                                // lines.links[q][w] == links[i].end
                                // lines.links[q][!w] == links[i].start
                                // links[i].end is now missing, so this line should truncate itself down to the new start pos
                                // unless that new start position is the current vertex, in which case it should just go away
                                if(closest != itr->first) {
                                  links[i].end = closest;
                                } else {
                                  links.erase(links.begin() + i);
                                  i--;
                                }
                              }
                            } else {
                              // Otherwise, we've spliced itr->first and itr->second.links[p][k], and I'm not even sure that makes sense
                              CHECK(0);
                            }
                            
                            // and we're done
                        }
                      }
                    }
                  }
                  CHECK(changed == 1);
                  
                  CHECK(checkConsistent(vertx, links));
                  dprintf("Collective splice is sane\n");
                  //return createSplitLines(vertx);
                  continue;
                } else {
                  int usedlin = vertx[closest].live[1];
                  
                  CHECK(!vertx[closest].live[!usedlin]);
                  CHECK(vertx[closest].live[usedlin]);
                  
                  splice(&vertx, lines, closest, !usedlin);
                  
                  links[i].end = closest;
                  
                  //CHECK(checkConsistent(vertx, links));
                  continue;
                }
              }
            }
            
            if(megaverbose)
              dprintf("  Splitting");

            if(junct.x < itr->first.x) {
              CHECK((itr->first.x - junct.x) < Coord(0.00001f));   // damn accuracy
              junct.x = itr->first.x;
            }
            if(junct <= itr->first) {
              // goddamn it
              dprintf("Stupid vertical lines fuck everything up!\n");
              junct.x = coordExplicit(junct.x.raw() + 1);
              CHECK(junct > itr->first);
            }
            
            if(!dumpBooleanDetail)
              CHECK(checkConsistent(vertx, links));
            
            CHECK(vertx[junct].live[0] == false);
            CHECK(vertx[junct].live[1] == false);
            
            //dprintf("a%d\n", k++);
            splice(&vertx, lines, junct, 0);
            //dprintf("b%d\n", k++);
            splice(&vertx, lines, junct, 1);
            //dprintf("c%d\n", k++);
            
            links[i].end = junct;
            
            if(!dumpBooleanDetail)
              CHECK(checkConsistent(vertx, links));
          }
        }
      }
    }
    //dprintf("Intersected\n");
    // Now we add new links from this point
    for(int p = 0; p < 2; p++) {
      if(!itr->second.live[p])
        continue;
      for(int k = 0; k < 2; k++) {
        if(itr->second.links[p][k] < itr->first)
          continue;
        if(megaverbose)
          dprintf("  Adding link\n");
        LiveLink nll;
        nll.start = itr->first;
        nll.end = itr->second.links[p][k];
        links.push_back(nll);
      }
    }
    if(!dumpBooleanDetail) {
      //CHECK(checkConsistent(vertx, links));
      //checkLineConsistency(vertx, links, itr->first);
      //dprintf("Unlooped, %d links\n", links.size());
    }
    CHECK(links.size() % 2 == 0);
  }
  CHECK(links.size() == 0);
  //dprintf("Done\n");
  //printState(vertx);
  CHECK(checkConsistent(vertx, links));
  #if 0   // This code intercepts the "split" version and returns it as the results - good for debugging
  {
    return createSplitLines(vertx);
  }
  #endif
  
  vector<vector<Coord2> > rv;
  {
    set<Coord2> seeds;
    set<pair<bool, Coord2> > seen;
    for(map<Coord2, DualLink>::const_iterator itr = vertx.begin(); itr != vertx.end(); itr++) {
      if(itr->second.live[0] && !inPath((itr->first + itr->second.links[0][1]) / 2, rhs) && !(itr->second.live[1] && itr->second.links[0][1] == itr->second.links[1][1])) {
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
            dprintf("  %f, %f --> this --> %f, %f",
                  vertx[now.second].links[i][0].x.toFloat(), vertx[now.second].links[i][0].y.toFloat(),
                  vertx[now.second].links[i][1].x.toFloat(), vertx[now.second].links[i][1].y.toFloat());
          } else {
            dprintf("  NULL");
          }
        }*/
        if(!now.first) {
          // came in off a lhs path - switch to rhs if there is one, and if it doesn't immediately leave the valid area
          if(vertx[now.second].live[1] && inPath((now.second + vertx[now.second].links[1][0]) / 2, tv[0]) == lhsInside) {
            CHECK(inPath((now.second + vertx[now.second].links[1][0]) / 2, tv[0]) == lhsInside);
            now = make_pair(true, vertx[now.second].links[1][0]);
          } else {
            CHECK(vertx[now.second].live[0]);
            if(!vertx[now.second].live[1] || vertx[now.second].links[0][1] != vertx[now.second].links[1][0]) // parallel links cause some problems
              CHECK(roughInPath((now.second + vertx[now.second].links[0][1]) / 2, tv[1], false));
            now = make_pair(false, vertx[now.second].links[0][1]);
          }
        } else {
          // came in off a rhs path - switch to lhs if there is one
          if(vertx[now.second].live[0] && !inPath((now.second + vertx[now.second].links[0][1]) / 2, tv[1])) {
            CHECK(!inPath((now.second + vertx[now.second].links[0][1]) / 2, tv[1]));
            now = make_pair(false, vertx[now.second].links[0][1]);
          } else {
            CHECK(vertx[now.second].live[1]);
            if(!vertx[now.second].live[0] || vertx[now.second].links[0][1] != vertx[now.second].links[1][0]) // parallel links cause some problems
              CHECK(roughInPath((now.second + vertx[now.second].links[1][0]) / 2, tv[0], lhsInside));
            now = make_pair(true, vertx[now.second].links[1][0]);
          }
        }
      }
      //dprintf("Built path of %d vertices\n", tpath.size());
      if(tpath.size() > 2)
        rv.push_back(tpath);
    }
  }
  #else
  vector<vector<Coord2> > rv(1,tv[0]);
  #endif
  vector<vector<Coord2> > rrv;
  bool gotReversedPath = false;
  int rvpathID = -1;
  vector<vector<Coord2> > rvpt;
  for(int i = 0; i < rv.size(); i++) {
    vector<vector<Coord2> > mas = mergeAndSplit(rv[i]);
    for(int j = 0; j < mas.size(); j++) {
      if(lhsInside) {
        if(pathReversed(mas[j])) {
          dprintf("Holy fuck, this path is screwed up!\n");
          CHECK(0);
        } else {
          rrv.push_back(mas[j]);
        }
      } else {
        if(pathReversed(mas[j])) {
          if(gotReversedPath) {
            Coord rva = abs(getArea(rrv[rvpathID]));
            Coord masa = abs(getArea(mas[j]));
            dprintf("Battle! %f area versus %f area, which inside-out path will survive!", rva.toFloat(), masa.toFloat());
            if(rva < masa) {
              rrv[rvpathID] = mas[j];
            }
          } else {
            CHECK(!gotReversedPath);
            rvpathID = rrv.size();
            rrv.push_back(mas[j]);
            gotReversedPath = true;
          }
        } else {
          rvpt.push_back(mas[j]);
        }
      }
    }
  }
  if(!lhsInside) {
    CHECK(rvpathID != -1);
    for(int k = 0; k < rvpt.size(); k++) {
      if(inPath(getCentroid(rvpt[k]), rrv[rvpathID])) {
        rrv.push_back(rvpt[k]);
      }
    }
  }
  return rrv;

}
