
#include "metagame.h"

#include "gfx.h"

bool Metagame::tweenTick(const vector< Controller > &keys) {
  vector<Keystates> ki = genKeystates(keys, pms);
  if(currentShop == -1) {
    StackString stp("Results");
    // this is a bit hacky - SHOP mode when currentShop is -1 is the "show results" screen
    for(int i = 0; i < ki.size(); i++) {
      CHECK(SIMUL_WEAPONS == 2);
      if(ki[i].accept.push || ki[i].fire[0].push || ki[i].fire[1].push)
        checked[i] = true;
    }
    if(count(checked.begin(), checked.end(), false) == 0) {
      for(int i = 0; i < playerdata.size(); i++)
        playerdata[i].addCash(lrCash[i]);
      currentShop = 0;
      shop.init(&playerdata[0], true);
    }
  } else if(shop.runTick(ki[currentShop])) {
    StackString stp("Shop");
    // and here's our actual shop - the tickrunning happens in the conditional, this is just what happens if it's time to change shops
    currentShop++;
    if(currentShop != playerdata.size()) {
      shop.init(&playerdata[currentShop], true);
    } else {
      mode = MGM_PLAY;
      game.initStandard(&playerdata, levels[int(frand() * levels.size())], &win_history);
      CHECK(win_history.size() == gameround);
    }
  }
  return false;
}

void Metagame::tweenRender() const {
  if(currentShop == -1) {
    StackString stp("Results");
    setZoom(Float4(0, 0, 800, 600));
    setColor(1.0, 1.0, 1.0);
    drawText("Damage", 30, 20, 20);
    drawText("Kills", 30, 20, 80);
    drawText("Wins", 30, 20, 140);
    drawText("Base", 30, 20, 200);
    drawText("Totals", 30, 20, 320);
    drawMultibar(lrCategory[0], Float4(200, 20, 700, 60));
    drawMultibar(lrCategory[1], Float4(200, 80, 700, 120));
    drawMultibar(lrCategory[2], Float4(200, 140, 700, 180));
    drawMultibar(lrCategory[3], Float4(200, 200, 700, 240));
    drawMultibar(lrPlayer, Float4(200, 320, 700, 360));
    setColor(1.0, 1.0, 1.0);
    drawJustifiedText("Waiting for", 30, 400, 400, TEXT_CENTER, TEXT_MIN);
    int notdone = count(checked.begin(), checked.end(), false);
    CHECK(notdone);
    int cpos = 0;
    float increment = 800.0 / notdone;
    for(int i = 0; i < checked.size(); i++) {
      if(!checked[i]) {
        setColor(playerdata[i].getFaction()->color);
        drawDvec2(playerdata[i].getFaction()->icon, boxAround(Float2((cpos + 0.5) * increment, float(440 + 580) / 2), min(increment * 0.95f, float(580 - 440)) / 2), 50, 1);
        cpos++;
      }
    }
  } else {
    StackString stp("Shop");
    
    const float divider_pos = 90;
    
    setZoom(Float4(0, 0, 133.333, 100));
    setColor(1.0, 1.0, 1.0);
    drawLine(Float4(0, divider_pos, 140, divider_pos), 0.1);
    
    GfxWindow gfxw(Float4(0, 0, 133.333, divider_pos), 1.0);
    
    setZoom(Float4(0, 0, getAspect(), 1.0));
    
    drawLine(Float4(0, 0.5, getAspect(), 0.5), 0.001);
    drawLine(Float4(getAspect() / 2, 0, getAspect() / 2, 1), 0.001);
    
    {
      GfxWindow gfxw2(Float4(0, 0, getAspect() / 2, 0.5), 1.0);
      shop.renderToScreen();
    }
    {
      GfxWindow gfxw2(Float4(getAspect() / 2, 0, getAspect(), 0.5), 1.0);
      shop.renderToScreen();
    }
    {
      GfxWindow gfxw2(Float4(0, 0.5, getAspect() / 2, 1), 1.0);
      shop.renderToScreen();
    }
    {
      GfxWindow gfxw2(Float4(getAspect() / 2, 0.5, getAspect(), 1), 1.0);
      shop.renderToScreen();
    }
  }
}
