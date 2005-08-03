
#include "itemdb.h"

#include <fstream>

#include "parse.h"
#include "util.h"

class HierarchyNode {
public:
    vector<HierarchyNode> branches;

    string name;

    enum {HNT_CATEGORY, HNT_WEAPON, HNT_DONE, HNT_LAST};
    int type;

    enum {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_LAST};
    int displaymode;

    bool buyable;
    int cost;
    
    int quantity;
    
    int cat_restrictiontype;
    
    void checkConsistency() const;
    
    HierarchyNode();
    
};

void HierarchyNode::checkConsistency() const {
    dprintf("Consistency scan entering %s\n", name.c_str());
    // all nodes need a name
    CHECK(name.size());
    
    // check that type is within bounds
    CHECK(type >= 0 && type < HNT_LAST);
    
    // check that displaymode is within bounds
    CHECK(displaymode >= 0 && displaymode < HNDM_LAST);
    
    // categories don't have a cost and aren't buyable
    if(type == HNT_CATEGORY) {
        CHECK(displaymode != HNDM_COST);
        CHECK(!buyable);
    }
    
    // weapons all have costs and are buyable
    if(type == HNT_WEAPON) {
        CHECK(displaymode == HNDM_COST);
        CHECK(buyable);
    }
    
    // the "done" token has no cost or other display but is "buyable"
    if(type == HNT_DONE) {
        CHECK(displaymode == HNDM_BLANK);
        CHECK(buyable);
        CHECK(name == "done");
        CHECK(cost == 0);
    }
    
    // if it's buyable, it has a cost
    if(buyable) {
        CHECK(cost >= 0);
    } else {
        CHECK(cost == -1);
    }
    
    // all things that are buyable have quantities
    if(buyable)
        CHECK(quantity > 0);
    
    // it may have no restriction or a valid restriction
    CHECK(cat_restrictiontype == -1 || cat_restrictiontype >= 0 && cat_restrictiontype < HNT_LAST);
    
    // if it's not a category, it shouldn't have branches
    CHECK(type == HNT_CATEGORY || branches.size() == 0);
    
    // last, check the consistency of everything recursively
    for(int i = 0; i < branches.size(); i++) {
        if(cat_restrictiontype != -1) {
            CHECK(branches[i].type == cat_restrictiontype || branches[i].type == HNT_CATEGORY);
            CHECK(branches[i].cat_restrictiontype == cat_restrictiontype);
        }
        branches[i].checkConsistency();
    }
    dprintf("Consistency scan leaving %s\n", name.c_str());
}

HierarchyNode::HierarchyNode() {
    type = HNT_LAST;
    displaymode = HNDM_LAST;
    buyable = false;
    cost = -1;
    quantity = -1;
    cat_restrictiontype = -1;
}

HierarchyNode root;

HierarchyNode *findNamedNode(const string &in, int postcut) {
    vector<string> toks = tokenize(in, ".");
    CHECK(toks.size());
    CHECK(toks.size() > postcut);
    toks.erase(toks.end() - postcut, toks.end());
    CHECK(toks[0] == "ROOT");
    HierarchyNode *current = &root;
    for(int i = 1; i < toks.size(); i++) {
        int fc = 0;
        int fi = -1;
        for(int k = 0; k < current->branches.size(); k++) {
            if(toks[i] == current->branches[k].name) {
                fc++;
                fi = k;
            }
        }
        CHECK(fc == 1);
        CHECK(fi != -1);
        current = &current->branches[fi];
    }
    return current;
}

void parseItemFile(const string &fname) {
    ifstream tfil(fname.c_str());
    CHECK(tfil);
    kvData chunk;
    while(getkvData(tfil, chunk)) {
        dprintf("%s\n", chunk.debugOutput().c_str());
        if(chunk.category == "hierarchy") {
            HierarchyNode *mountpoint = findNamedNode(chunk.kv["name"], 1);
            HierarchyNode tnode;
            tnode.name = tokenize(chunk.consume("name"), ".").back();
            dprintf("name: %s\n", tnode.name.c_str());
            tnode.type = HierarchyNode::HNT_CATEGORY;
            if(chunk.kv.count("pack")) {
                tnode.displaymode = HierarchyNode::HNDM_PACK;
                tnode.quantity = atoi(chunk.consume("pack").c_str());
                CHECK(mountpoint->quantity == -1);
            } else {
                tnode.displaymode = HierarchyNode::HNDM_BLANK;
                tnode.quantity = mountpoint->quantity;
            }
            if(chunk.kv.count("type")) {
                if(chunk.kv["type"] == "weapon") {
                    tnode.cat_restrictiontype = HierarchyNode::HNT_WEAPON;
                } else {
                    CHECK(0);
                }
                chunk.consume("type");
            }
            if(tnode.cat_restrictiontype == -1) {
                tnode.cat_restrictiontype = mountpoint->cat_restrictiontype;
            }
            CHECK(mountpoint->cat_restrictiontype == -1 || tnode.cat_restrictiontype == mountpoint->cat_restrictiontype);
            mountpoint->branches.push_back(tnode);
        } else if(chunk.category == "weapon") {
        } else if(chunk.category == "projectile") {
        } else {
            CHECK(0);
        }
    }
}

void initItemdb() {
    {
        CHECK(root.name == "");
        root.name = "ROOT";
        root.type = HierarchyNode::HNT_CATEGORY;
        root.displaymode = HierarchyNode::HNDM_BLANK;
    }
    
    string basepath = "data/base/";
    ifstream manifest((basepath + "manifest").c_str());
    string line;
    while(getLineStripped(manifest, line)) {
        dprintf("%s\n", line.c_str());
        parseItemFile(basepath + line);
    }
    
    {
        // add our hardcoded "done" token
        HierarchyNode tnode;
        tnode.name = "done";
        tnode.type = HierarchyNode::HNT_DONE;
        tnode.displaymode = HierarchyNode::HNDM_BLANK;
        tnode.buyable = true;
        tnode.cost = 0;
        tnode.quantity = 1;
        root.branches.push_back(tnode);
    }
    
    dprintf("done loading, consistency check\n");
    root.checkConsistency();
}
