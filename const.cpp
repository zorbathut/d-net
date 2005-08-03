
#include "const.h"

int frameNumber;

// This should be in metagame.cpp. It's been moved out for compiletime reasons.
#include "metagame.h"

void Shop::recreateShopNetwork() {
    root.branches.clear();
    root.branches.push_back(ShopNode("weapons", 0, false, false));
    {
        ShopNode &weap = root.branches[0];
        weap.branches.push_back(ShopNode("chaingun", 0, false, false));
        {
            ShopNode &chg = weap.branches[0];
            chg.branches.push_back(ShopNode("chaingun", 0, true, true));
            chg.branches.push_back(ShopNode("chaingun II", 3, true, true));
            chg.branches.push_back(ShopNode("chaingun III", 6, true, true));
            chg.branches.push_back(ShopNode("chaingun IV", 10, true, true));
            chg.branches.push_back(ShopNode("chaingun V", 15, true, true));
        }
        weap.branches.push_back(ShopNode("missile", 0, false, false));
        {
            ShopNode &miss = weap.branches[1];
            miss.branches.push_back(ShopNode("missile", 3, true, true));
            miss.branches.push_back(ShopNode("missile II", 8, true, true));
            miss.branches.push_back(ShopNode("missile III", 15, true, true));
        }
        weap.branches.push_back(ShopNode("laser", 0, false, false));
        {
            ShopNode &lase = weap.branches[2];
            lase.branches.push_back(ShopNode("laser", 6, true, true));
            lase.branches.push_back(ShopNode("laser II", 15, true, true));
            lase.branches.push_back(ShopNode("laser III", 25, true, true));
        }
    }
    root.branches.push_back(ShopNode("chassis", 0, false, false));
    {
        ShopNode &chass = root.branches[1];
        chass.branches.push_back(ShopNode("hull boost", 100, true, true));
        chass.branches.push_back(ShopNode("speed boost", 100, true, true));
        chass.branches.push_back(ShopNode("turn boost", 100, true, true));
    }
    root.branches.push_back(ShopNode("done", 0, false, true));
    
    curloc.clear();
    curloc.push_back(0);
}

