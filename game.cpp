
#include "game.h"

#include "args.h"
#include "debug.h"
#include "game_ai.h"
#include "game_tank.h"
#include "gfx.h"
#include "player.h"

using namespace std;

DEFINE_bool(verboseCollisions, false, "Verbose collisions");
DEFINE_bool(debugGraphics, false, "Enable various debug graphics");
DEFINE_bool(debugGraphicsCollisions, false, "Enable HUD for collision stats");

// returns center and width/height
pair<Float2, Float2> getMapZoom(const Coord4 &mapbounds) {
  Float4 bounds = mapbounds.toFloat();
  pair<Float2, Float2> rv;
  rv.first.x = (bounds.sx + bounds.ex) / 2;
  rv.first.y = (bounds.sy + bounds.ey) / 2;
  rv.second.x = bounds.ex - bounds.sx;
  rv.second.y = bounds.ey - bounds.sy;
  rv.second.x *= 1.1;
  rv.second.y *= 1.1;
  return rv;
}

void goCloser(float *cur, const float *now, const float dist) {
  if(*cur < *now) {
    *cur = min(*cur + dist, *now);
  } else {
    *cur = max(*cur - dist, *now);
  }
}

void doInterp(float *curcenter, const float *nowcenter, float *curzoom, const float *nowzoom, float *curspeed) {
  if(*curcenter != *nowcenter || *curzoom != *nowzoom) {
    *curspeed = min(*curspeed + 0.0000002, 0.001);
    goCloser(curcenter, nowcenter, *nowzoom * *curspeed);
    goCloser(curzoom, nowzoom, *nowzoom * *curspeed);
  } else {
    *curspeed = max(*curspeed - 0.000002, 0.);
  }
}

bool Game::runTick(const vector<Keystates> &rkeys, const vector<Player *> &players, Rng *rng) {
  StackString sst("Frame runtick");
  
  CHECK(rkeys.size() == players.size());
  CHECK(players.size() == tanks.size());

  GameImpactContext gic(&tanks, &gfxeffects, &gamemap, rng);
  
  if(!ffwd && FLAGS_verboseCollisions)
    dprintf("Ticking\n");
  
  frameNm++;
  
  vector< Keystates > keys = rkeys;
  if(frameNm < frameNmToStart && freezeUntilStart) {
    for(int i = 0; i < keys.size(); i++) {
      if(keys[i].accept.push || keys[i].fire[0].push)
        gfxeffects.push_back(GfxPing(tanks[i].pos.toFloat(), zoom_size.y, zoom_size.y / 50, 0.5, tanks[i].getColor()));
      keys[i].nullMove();
      for(int j = 0; j < SIMUL_WEAPONS; j++)
        keys[i].fire[j] = Button();
    }
  }
  
  if(gamemode == GMODE_DEMO) {
    // EVERYONE IS INVINCIBLE
    for(int i = 0; i < tanks.size(); i++) {
      tanks[i].megaboostHealth();
    }
  }
  
  // first we deal with moving the tanks around
  // we shuffle the player order randomly, then move tanks in that order
  // if the player can't move where they want to, they simply don't move
  // I am assuming there will be no tanks that can move further than their entire length per frame.
  
  // Second, we feed the player start/end positions into the old collision system, along with all the projectiles
  // And then we do our collision system solely on projectile/* intersections.
  
  // I think this works.
  
  const Coord4 gmbc = gamemap.getCollisionBounds();
  const Coord4 gmbr = gamemap.getRenderBounds();
  vector<int> teamids;
  {
    for(int i = 0; i < tanks.size(); i++)
      teamids.push_back(tanks[i].team);
  }
  
  {
    StackString sst("Player movement collider");
    
    collider.cleanup(COM_PLAYER, gmbc, teamids);
    
    {
      StackString sst("Adding walls");
    
      gamemap.updateCollide(&collider);
    }
    
    for(int j = 0; j < tanks.size(); j++) {
      if(!tanks[j].isLive())
        continue;
      StackString sst(StringPrintf("Adding player %d, status live %d", j, tanks[j].isLive()));
      //CHECK(inPath(tanks[j].pos, gamemap.getCollide()[0]));
      if(!isInside(gmbc, tanks[j].pos)) {
        StackString sst("Critical error, running tests");
        dprintf("%s vs %s\n", tanks[j].pos.rawstr().c_str(), gmbc.rawstr().c_str());
        CHECK(0);
      }
      tanks[j].addCollision(&collider, keys[j], j);
    }
    
    /*    // CPU-intensive :(
    for(int j = 0; j < tanks.size(); j++)
      CHECK(!collider.checkSimpleCollision(CGR_TANK, j, tanks[j].getCurrentCollide()));
    */
    
    vector<int> playerorder;
    {
      vector<int> tanksleft;
      for(int i = 0; i < tanks.size(); i++)
        if(tanks[i].isLive())
          tanksleft.push_back(i);
      // TODO: turn this into random-shuffle using my deterministic seed
      while(tanksleft.size()) {
        int pt = int(rng->frand() * tanksleft.size());
        CHECK(pt >= 0 && pt < tanksleft.size());
        playerorder.push_back(tanksleft[pt]);
        tanksleft.erase(tanksleft.begin() + pt);
      }
    }
    
    for(int i = 0; i < playerorder.size(); i++) {
      
      CHECK(count(playerorder.begin(), playerorder.end(), playerorder[i]) == 1);
      CHECK(playerorder[i] >= 0 && playerorder[i] < tanks.size());
      
      vector<Coord4> newpos = tanks[playerorder[i]].getNextCollide(keys[playerorder[i]]);
      {
        Tank test = tanks[playerorder[i]];
        test.tick(keys[playerorder[i]]);
        CHECK(newpos == test.getCurrentCollide());
      }
      
      if(collider.checkSimpleCollision(CGR_TANK, playerorder[i], newpos)) {
        tanks[playerorder[i]].inertia = make_pair(0.f, 0.f);  // wham!
        keys[playerorder[i]].nullMove();
      } else {
        StackString sst(StringPrintf("Moving player %d, status live %d", playerorder[i], tanks[playerorder[i]].isLive()));
        //CHECK(inPath(tanks[playerorder[i]].getNextPosition(keys[playerorder[i]], tanks[playerorder[i]].pos, tanks[playerorder[i]].d).first, gamemap.getCollide()[0]));
        CHECK(isInside(gmbc, tanks[playerorder[i]].getNextPosition(keys[playerorder[i]]).first));
        collider.dumpGroup(CollideId(CGR_TANK, playerorder[i], 0));
        for(int j = 0; j < newpos.size(); j++)
          collider.addToken(CollideId(CGR_TANK, playerorder[i], 0), newpos[j], Coord4(0, 0, 0, 0));
      }

    }
    
  }
  
  {
    StackString sst("Main collider");
    
    collider.cleanup(COM_PROJECTILE, gmbc, teamids);
    
    gamemap.updateCollide(&collider);
    
    for(int j = 0; j < tanks.size(); j++)
      tanks[j].addCollision(&collider, keys[j], j);
  
    for(int j = 0; j < projectiles.size(); j++)
      projectiles[j].updateCollisions(&collider, j);
    
    collider.processMotion();
    
    while(collider.next()) {
      //dprintf("Collision!\n");
      //dprintf("%d,%d,%d vs %d,%d,%d\n", collider.getCollision().lhs.category, collider.getCollision().lhs.bucket, collider.getCollision().lhs.item, collider.getCollision().rhs.category, collider.getCollision().rhs.bucket, collider.getCollision().rhs.item);
      CollideId lhs = collider.getCollision().lhs;
      CollideId rhs = collider.getCollision().rhs;
      if(lhs.category == CGR_STATPROJECTILE)
        lhs.category = CGR_PROJECTILE;
      if(rhs.category == CGR_STATPROJECTILE)
        rhs.category = CGR_PROJECTILE;
      if(rhs < lhs) swap(lhs, rhs);
      if(lhs.category == CGR_TANK && rhs.category == CGR_TANK) {
        // tank-tank collision, should never happen
        CHECK(0);
      } else if(lhs.category == CGR_TANK && rhs.category == CGR_PROJECTILE) {
        // tank-projectile collision - kill projectile, do damage
        if(projectiles[rhs.bucket].find(rhs.item).isConsumed())
          continue;
        projectiles[rhs.bucket].find(rhs.item).detonate(collider.getCollision().pos, &tanks[lhs.bucket], GamePlayerContext(&tanks[rhs.bucket], &projectiles[rhs.bucket], gic), true);
      } else if(lhs.category == CGR_TANK && rhs.category == CGR_WALL) {
        // tank-wall collision, should never happen
        CHECK(0);
      } else if(lhs.category == CGR_PROJECTILE && rhs.category == CGR_PROJECTILE) {
        // projectile-projectile collision - kill both projectiles
        // also do radius damage, and do it fairly dammit
        if(projectiles[lhs.bucket].find(lhs.item).isConsumed() || projectiles[rhs.bucket].find(rhs.item).isConsumed())
          continue;
        
        bool lft = rng->frand() < 0.5;
        
        if(projectiles[lhs.bucket].find(lhs.item).toughness() > projectiles[rhs.bucket].find(rhs.item).toughness())
          swap(lhs, rhs);
        
        // LHS now has less toughness, so it's guaranteed to go boom
        bool rhsdestroyed = rng->frand() < (projectiles[lhs.bucket].find(lhs.item).toughness() / projectiles[rhs.bucket].find(rhs.item).toughness());
        
        if(lft)
          projectiles[lhs.bucket].find(lhs.item).detonate(collider.getCollision().pos, NULL, GamePlayerContext(&tanks[rhs.bucket], &projectiles[lhs.bucket], gic), true);
        
        if(rhsdestroyed)
          projectiles[rhs.bucket].find(rhs.item).detonate(collider.getCollision().pos, NULL, GamePlayerContext(&tanks[rhs.bucket], &projectiles[rhs.bucket], gic), true);
        
        if(!lft)
          projectiles[lhs.bucket].find(lhs.item).detonate(collider.getCollision().pos, NULL, GamePlayerContext(&tanks[rhs.bucket], &projectiles[lhs.bucket], gic), true);
        
      } else if(lhs.category == CGR_PROJECTILE && rhs.category == CGR_WALL) {
        // projectile-wall collision - kill projectile
        if(projectiles[lhs.bucket].find(lhs.item).isConsumed())
          continue;
        projectiles[lhs.bucket].find(lhs.item).detonate(collider.getCollision().pos, NULL, GamePlayerContext(&tanks[rhs.bucket], &projectiles[lhs.bucket], gic), true);
      } else if(lhs.category == CGR_WALL && rhs.category == CGR_WALL) {
        // wall-wall collision, wtf?
        CHECK(0);
      } else {
        // nothing meaningful, should totally never happen, what the hell is going on here, who are you, and why are you in my apartment
        dprintf("%d, %d\n", lhs.category, rhs.category);
        CHECK(0);
      }
    }
    
    collider.finishProcess();
  }

  for(int j = 0; j < projectiles.size(); j++) {
    projectiles[j].tick(&gfxeffects, &collider, j, gic);
  }
  
  for(int j = 0; j < bombards.size(); j++) {
    CHECK(bombards[j].state >= 0 && bombards[j].state < BombardmentState::BS_LAST);
    bombards[j].framesSinceLastSwitch++;
    if(bombards[j].state == BombardmentState::BS_OFF) {
      if(!tanks[j].isLive()) {
        // if the player is dead and the bombard isn't initialized
        bombards[j].pos = tanks[j].pos;
        bombards[j].state = BombardmentState::BS_SPAWNING;
        if(gamemode == GMODE_DEMO)
          bombards[j].timer = 0;
        else
          bombards[j].timer = 60 * 6;
      }
    } else if(bombards[j].state == BombardmentState::BS_SPAWNING) {
      bombards[j].timer--;
      if(bombards[j].timer <= 0)
        bombards[j].state = BombardmentState::BS_ACTIVE;
    } else if(bombards[j].state == BombardmentState::BS_ACTIVE) {
      Float2 deaded = deadzone(keys[j].udlrax, DEADZONE_CENTER, 0.2);
      if(len(deaded) > 1)
        deaded /= len(deaded);
      deaded.y *= -1;
      bombards[j].pos += Coord2(deaded);
      bombards[j].pos.x = max(bombards[j].pos.x, gmbr.sx);
      bombards[j].pos.y = max(bombards[j].pos.y, gmbr.sy);
      bombards[j].pos.x = min(bombards[j].pos.x, gmbr.ex);
      bombards[j].pos.y = min(bombards[j].pos.y, gmbr.ey);
      CHECK(SIMUL_WEAPONS == 2);
      if(keys[j].fire[0].down || keys[j].fire[1].down) {
        bombards[j].state = BombardmentState::BS_FIRING;
        bombards[j].timer = round(players[j]->getBombardment((int)bombardment_tier).lockdelay() * FPS);
      }
    } else if(bombards[j].state == BombardmentState::BS_FIRING) {
      bombards[j].timer--;
      if(bombards[j].timer <= 0) {
        detonateWarhead(players[j]->getBombardment((int)bombardment_tier).warhead(), bombards[j].pos, Coord2(0, 0), NULL, GamePlayerContext(&tanks[j], &projectiles[j], gic), 1.0, false, true);
        bombards[j].state = BombardmentState::BS_COOLDOWN;
        bombards[j].timer = round(players[j]->getBombardment((int)bombardment_tier).unlockdelay() * FPS);
      }
    } else if(bombards[j].state == BombardmentState::BS_COOLDOWN) {
      bombards[j].timer--;
      if(bombards[j].timer <= 0)
        bombards[j].state = BombardmentState::BS_ACTIVE;
    } else {
      CHECK(0);
    }
  }  
  
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].tick(keys[i]);

  for(int i = 0; i < tanks.size(); i++) {
    StackString sst(StringPrintf("Player weaponry %d", i));
    
    // Keep bombardment spamming from occuring
    if(!tanks[i].isLive() && bombards[i].framesSinceLastSwitch < FPS)
      continue;
    
    // Switch weapons
    for(int j = 0; j < SIMUL_WEAPONS; j++) {
      if(keys[i].change[j].push) {
        StackString sst(StringPrintf("Switching"));
        players[i]->cycleWeapon(j);
        addTankStatusText(i, players[i]->getWeapon(j).name(), 1);
        bombards[i].framesSinceLastSwitch = 0;
      }
    }
    
    // Attempt to actually fire - deals with all weapons that the tank has equipped.
    if(tanks[i].isLive() && teams[tanks[i].team].weapons_enabled && frameNm >= frameNmToStart && frameNmToStart != -1) {
      vector<pair<string, float> > status;
      tanks[i].tryToFire(keys[i].fire, players[i], &projectiles[i], i, gic, &status, &firepowerSpent);
      for(int j = 0; j < status.size(); j++)
        addTankStatusText(i, status[j].first, status[j].second);
    }
  }

  {
    vector<smart_ptr<GfxEffects> > neffects;
    for(int i = 0; i < gfxeffects.size(); i++) {
      gfxeffects[i]->tick();
      if(!gfxeffects[i]->dead())
        neffects.push_back(gfxeffects[i]);
    }
    swap(neffects, gfxeffects);
  }

  for(int i = 0; i < tanks.size(); i++) {
    tanks[i].genEffects(gic, &projectiles[i], players[i]);
    if(tanks[i].isLive()) {
      int inzone = -1;
      for(int j = 0; j < zones.size(); j++)
        if(inPath(tanks[i].pos, zones[j].first))
          inzone = j;
      if(tanks[i].zone_current != inzone) {
        tanks[i].zone_current = inzone;
        tanks[i].zone_frames = 0;
      }
      tanks[i].zone_frames++;
    }
  }
  
  // This is a bit ugly - this only happens in choice mode
  if(zones.size() == 4) {
    CHECK(teams.size() == 5);
    for(int i = 0; i < tanks.size(); i++) {
      if(tanks[i].zone_current != -1 && (tanks[i].zone_frames - 1) % 60 == 0 && tanks[i].team == 4) {
        int secleft = (181 - tanks[i].zone_frames) / 60;
        if(secleft) {
          addTankStatusText(i, StringPrintf("%d second%s to join team", secleft, (secleft == 1 ? "" : "s")), 1.5);
        } else {
          addTankStatusText(i, StringPrintf("Team joined"), 2);
        }
      }
      if(tanks[i].zone_current != -1 && tanks[i].zone_frames > 180 && tanks[i].team == 4) {
        tanks[i].team = tanks[i].zone_current;
        if(frameNmToStart == -1)
          frameNmToStart = frameNm + 60 * 6;
      }
    }
  }
  
  #if 0 // This hideous hack produces pretty yet deadly fireworks
  {   
    static Tank boomy;
    static Player boomyplay;
    static FactionState boomyfact;
    boomy.spawnShards = true;
    boomy.player = &boomyplay;
    boomyplay.glory = defaultGlory();
    float border = 40;
    boomy.pos.x = Coord(rng->frand()) * (gmbr.x_span() - border * 2) + gmbr.sx + border;
    boomy.pos.y = Coord(rng->frand()) * (gmbr.y_span() - border * 2) + gmbr.sy + border;
    boomy.d = 0;
    boomyplay.faction = &boomyfact;
    boomyfact.color = Color(1.0, 1.0, 1.0);
    boomy.genEffects(&gfxeffects, &projectiles[0]);
  }
  #endif
  
  {
    set<int> liveteams;
    for(int i = 0; i < tanks.size(); i++) {
      if(tanks[i].isLive())
        liveteams.insert(tanks[i].team);
    }
    if(liveteams.size() <= 1) {
      if(zones.size() != 4 || liveteams.size() != 1 || liveteams.count(4) == 0)   // nasty hackery for choice mode
        framesSinceOneLeft++;
    }
  }

  {
    pair<Float2, Float2> z = getMapZoom(gamemap.getRenderBounds());
    
    doInterp(&zoom_center.x, &z.first.x, &zoom_size.x, &z.second.x, &zoom_speed.x);
    doInterp(&zoom_center.y, &z.first.y, &zoom_size.y, &z.second.y, &zoom_speed.y);
  }
  
  if(gamemode == GMODE_STANDARD)
    bombardment_tier += getBombardmentIncreasePerSec() / FPS;

  if(framesSinceOneLeft / FPS >= 3 && gamemode != GMODE_TEST && gamemode != GMODE_DEMO && gamemode != GMODE_CENTERED_DEMO) {
    if(zones.size() == 0) {
      int winplayer = -1;
      for(int i = 0; i < tanks.size(); i++) {
        if(tanks[i].isLive()) {
          players[i]->addWin();
          CHECK(winplayer == -1);
          winplayer = i;
        }
        tanks[i].addAccumulatedScores(players[i]);
      }
    } else if(zones.size() == 4) {
    } else {
      CHECK(0);
    }
    return true;
  } else {
    return false;
  }

};

void Game::ai(const vector<GameAi *> &ais) const {
  CHECK(ais.size() == tanks.size());
  for(int i = 0; i < ais.size(); i++) {
    if(ais[i]) {
      if(tanks[i].isLive()) {
        ais[i]->updateGame(tanks, i);
      } else {
        ais[i]->updateBombardment(tanks, bombards[i].pos);
      }
    }
  }
}

void drawCirclePieces(const Coord2 &cloc, float solidity, float rad) {
  Float2 loc = cloc.toFloat();
  for(int i = 0; i < 3; i++) {
    float core = PI / 2 * 3 + (PI * 2 / 3 * i);
    float span = PI * 2 / 6 + PI * 2 / 6 * solidity;
    vector<Float2> path;
    for(int k = 0; k <= 10; k++)
      path.push_back(makeAngle(core - span / 2 + span / 10 * k) * rad + loc);
    drawLinePath(path, 0.3);
  }
}

void drawCrosses(const Coord2 &cloc, float rad) {
  Float2 loc = cloc.toFloat();
  for(int i = 0; i < 3; i++) {
    float core = PI / 2 * 3 + (PI * 2 / 3 * i);
    drawLine(makeAngle(core) * rad * 1.25 + loc, makeAngle(core) * rad * 0.75 + loc, 0.3);
  }
}

void Game::renderToScreen(const vector<const Player *> &players, GameMetacontext gmc) const {
  
  {
    // Set up zooming for the gfx window, if necessary
    setZoom(Float4(0, 0, getAspect(), 1));
    bool hasStatus = false;
    if(gamemode == GMODE_STANDARD || gamemode == GMODE_CHOICE)
      hasStatus = true;
    
    smart_ptr<GfxWindow> gfxw;
    
    // Possibly make the GFX window, and also make the actual zooming
    if(gamemode != GMODE_CENTERED_DEMO) {
      gfxw.reset(new GfxWindow(Float4(0, hasStatus?0.1:0, getAspect(), 1), 1.0));
      
      setZoomAround(Coord4(zoom_center.x - zoom_size.x / 2, zoom_center.y - zoom_size.y / 2, zoom_center.x + zoom_size.x / 2, zoom_center.y + zoom_size.y / 2));
    } else {
      CHECK(tanks[0].isLive());
      setZoomVertical(tanks[0].pos.x.toFloat() - centereddemo_zoom / 2, tanks[0].pos.y.toFloat() - centereddemo_zoom / 2, tanks[0].pos.y.toFloat() + centereddemo_zoom / 2);
    }
    
    // In most modes, clear the background
    if(gamemode == GMODE_DEMO || gamemode == GMODE_CENTERED_DEMO || gamemode == GMODE_TEST) {
      drawSolid(clear);
    }
    
    // In centered-demo mode, draw the grid
    if(gamemode == GMODE_CENTERED_DEMO) {
      setColor(C::gray(0.5));
      drawGrid(10, 0.1);
    }
    
    // Tanks
    for(int i = 0; i < tanks.size(); i++) {
      tanks[i].render(teams);
      // Debug graphics :D
      collider.renderAround(tanks[i].pos);
    }
    
    // Projectiles, graphics effects, and bombardments
    {
      // mines need these to know how far away they are from something
      vector<Coord2> tankposes;
      for(int i = 0; i < tanks.size(); i++)
        if(tanks[i].isLive())
          tankposes.push_back(tanks[i].pos);
      
      for(int i = 0; i < projectiles.size(); i++)
        projectiles[i].render(tankposes);
    }
    
    for(int i = 0; i < gfxeffects.size(); i++)
      gfxeffects[i]->render();
    
    for(int i = 0; i < bombards.size(); i++) {
      if(bombards[i].state == BombardmentState::BS_OFF) {
      } else if(bombards[i].state == BombardmentState::BS_SPAWNING) {
      } else if(bombards[i].state == BombardmentState::BS_ACTIVE) {
        setColor(tanks[i].getColor() * 0.8);
        drawCirclePieces(bombards[i].pos, 0.3, 4);
        drawCrosses(bombards[i].pos, 4);
      } else if(bombards[i].state == BombardmentState::BS_FIRING) {
        setColor(tanks[i].getColor() * 0.5);
        drawCirclePieces(bombards[i].pos, 0.3, 4);
        drawCrosses(bombards[i].pos, 4);
        setColor(Color(1.0, 1.0, 1.0));
        float ps = (float)bombards[i].timer / (players[i]->getBombardment((int)bombardment_tier).lockdelay() * FPS);
        drawCirclePieces(bombards[i].pos, 1 - ps, 4 * ps);
      } else if(bombards[i].state == BombardmentState::BS_COOLDOWN) {
        setColor(tanks[i].getColor() * 0.5);
        drawCirclePieces(bombards[i].pos, 0.3, 4);
        drawCrosses(bombards[i].pos, 4);
        float ps = (float)bombards[i].timer / (players[i]->getBombardment((int)bombardment_tier).unlockdelay() * FPS);
        drawCirclePieces(bombards[i].pos, ps, 4 * (1 - ps));
      } else {
        CHECK(0);
      }
    }
    
    // Game map and collider, if we're drawing one
    gamemap.render();
    collider.render();
    
    // This is where we draw the zones
    for(int i = 0; i < zones.size(); i++) {
      setColor(zones[i].second * 0.3);
      drawLineLoop(zones[i].first, 1.0);
    }
    
    // Here's the text for choice mode
    if(gamemode == GMODE_CHOICE) {
      vector<vector<string> > zonenames;
      {
        vector<string> foo;
        
        foo.push_back("No Bonuses");
        foo.push_back("No Penalties");
        zonenames.push_back(foo);
        foo.clear();
        
        foo.push_back("Small Bonuses");
        foo.push_back("No Penalties");
        zonenames.push_back(foo);
        foo.clear();
        
        foo.push_back("Medium Bonuses");
        foo.push_back("Small Penalties");
        zonenames.push_back(foo);
        foo.clear();
        
        foo.push_back("Large Bonuses");
        foo.push_back("Medium Penalties");
        zonenames.push_back(foo);
        foo.clear();
      }
        
      for(int i = 0; i < zones.size(); i++) {
        setColor(zones[i].second);
        Float2 pos = getCentroid(zones[i].first).toFloat();
        pos.x *= 2;
        pos.y *= 1.4;
        drawJustifiedMultiText(zonenames[i], 10, pos, TEXT_CENTER, TEXT_CENTER);
      }
    }
    
    // Here is the DPS numbers
    if(gamemode == GMODE_DEMO) {
      setColor(1.0, 1.0, 1.0);
      for(int i = 0; i < tanks.size(); i++) {
        if(demomode_playermodes[i] == DEMOPLAYER_DPS) {
          if(tanks[i].getDPS() > 0) {
            drawJustifiedText(StringPrintf("%.2f DPS", tanks[i].getDPS()), demomode_boxradi / 15, tanks[i].pos.toFloat() - demo_hudpos, TEXT_MAX, TEXT_MAX);
          }
        } else if(demomode_playermodes[i] == DEMOPLAYER_DPC) {
          if(demomode_hits) {
            drawJustifiedText(StringPrintf("%.2f DPH", tanks[i].getDPC(demomode_hits)), demomode_boxradi / 15, tanks[i].pos.toFloat() - demo_hudpos, TEXT_MAX, TEXT_MAX);
          }
        } else if(demomode_playermodes[i] == DEMOPLAYER_DPH) {
          if(tanks[i].getDPH() > 0) {
            drawJustifiedText(StringPrintf("%.2f DPH", tanks[i].getDPH()), demomode_boxradi / 15, tanks[i].pos.toFloat() - demo_hudpos, TEXT_MAX, TEXT_MAX);
          }
        } else if(demomode_playermodes[i] == DEMOPLAYER_BOMBSIGHT) {
        } else if(demomode_playermodes[i] == DEMOPLAYER_QUIET) {
        } else {
          CHECK(0);
        }
      }
    }
  }
  
  // Here's everything outside gamespace
  if(gamemode != GMODE_TEST && gamemode != GMODE_DEMO && gamemode != GMODE_CENTERED_DEMO) {
    setZoom(Float4(0, 0, 133.333, 100));
    
    // Player health
    {
      setColor(1.0, 1.0, 1.0);
      drawLine(Float4(0, 10, (400./3.), 10), 0.1);
      for(int i = 0; i < tanks.size(); i++) {
        setColor(1.0, 1.0, 1.0);
        float loffset = (400./3.) / tanks.size() * i;
        float roffset = (400./3.) / tanks.size() * (i + 1);
        if(i)
          drawLine(Float4(loffset, 0, loffset, 10), 0.1);
        
        setColor(players[i]->getFaction()->color);
        
        if(tanks[i].isLive()) {
          float barl = loffset + 1;
          float bare = (roffset - 1) - (loffset + 1);
          bare /= players[i]->getTank().maxHealth();
          bare *= tanks[i].getHealth();
          drawShadedRect(Float4(barl, 2, barl + bare, 7), 0.1, 2);
        }
          
        string ammotext[SIMUL_WEAPONS];
        for(int j = 0; j < SIMUL_WEAPONS; j++) {
          if(players[i]->shotsLeft(j) == UNLIMITED_AMMO) {
            ammotext[j] = "Inf";
          } else {
            ammotext[j] = StringPrintf("%d", players[i]->shotsLeft(j));
          }
        }
        
        CHECK(SIMUL_WEAPONS == 2);
        drawJustifiedText(ammotext[0], 1.5, Float2(loffset + 1, 7.75), TEXT_MIN, TEXT_MIN);
        drawJustifiedText(ammotext[1], 1.5, Float2(roffset - 1, 7.75), TEXT_MAX, TEXT_MIN);
      }
    }
    
    // The giant overlay text for countdowns
    if(frameNmToStart == -1) {
      setColor(C::gray(1.0));
      drawJustifiedText("Choose team", 8, Float2(133.3, 100) / 2, TEXT_CENTER, TEXT_CENTER);
    } else if(frameNm < frameNmToStart) {
      setColor(C::gray(1.0));
      int fleft = frameNmToStart - frameNm;
      int s;
      if(frameNm % 60 < 5) {
        s = 15;
      } else if(frameNm % 30 < 5) {
        s = 12;
      } else {
        s = 8;
      }
      drawJustifiedText(StringPrintf("Ready %d.%02d", fleft / 60, fleft % 60), s, Float2(133.3, 100) / 2, TEXT_CENTER, TEXT_CENTER);
    } else if(frameNm < frameNmToStart + 60) {
      setColor(C::gray((240.0 - frameNm) / 60));
      drawJustifiedText("GO", 40, Float2(133.3, 100) / 2, TEXT_CENTER, TEXT_CENTER);
    }
    
    // Bombardment level text
    if(bombardment_tier != 0) {
      setColor(C::gray(1.0));
      drawText(StringPrintf("Bombardment level %d, %.0fs until next level", (int)floor(bombardment_tier) + 1, getTimeUntilBombardmentUpgrade()), 2, Float2(2, 96));
    }
    
    setZoom(Float4(0, 0, 1.33333, 1));
    
    // Our win ticker
    if(gmc.hasMetacontext()) {
      const float iconwidth = 0.02;
      const float iconborder = 0.001;
      const float comboborder=0.0015;
      const float lineborder = iconborder * 2;
      const float lineextra = 0.005;
      
      Float2 spos(0.01, 0.11);
      
      for(int i = 0; i < gmc.getWins().size(); i += 6) {
        map<const IDBFaction *, int> fc;
        int smax = 0;
        for(int j = 0; j < i; j++) {
          fc[gmc.getWins()[j]]++;
          smax = max(smax, fc[gmc.getWins()[j]]);
        }
        fc.erase(NULL);
        
        int winrup = gmc.getWins().size() / 6 * 6 + 6;
        
        float width = 0.02;
        if(fc.size())
          width += iconwidth * fc.size() + lineborder * 2;
        width += iconwidth * (winrup - i) + lineborder * 2 * ((winrup - i) / 6);
        
        if(width >= 1.33)
          continue;
        
        float hei = min(iconwidth, iconwidth * 6 / smax);
        
        if(i) {
          for(map<const IDBFaction *, int>::iterator itr = fc.begin(); itr != fc.end(); itr++) {
            setColor(itr->first->color);
            for(int j = 0; j < itr->second; j++)
              drawDvec2(itr->first->icon, Float4(spos.x + comboborder, spos.y + comboborder + hei * j, spos.x + iconwidth - comboborder, spos.y + iconwidth - comboborder + hei * j), 10, 0.0002);
            float linehei = spos.y + hei * (itr->second - 1) + iconwidth;
            drawLine(spos.x + iconborder + iconborder, linehei, spos.x + iconwidth - iconborder - iconborder, linehei, 0.0002);
            spos.x += iconwidth;
          }
          spos.x += lineborder;
          drawLine(spos.x, spos.y - lineextra, spos.x, spos.y + iconwidth + lineextra, 0.0002);
          spos.x += lineborder;
        }
            
        for(int j = i; j < gmc.getWins().size(); j++) {
          if(gmc.getWins()[j]) {
            setColor(gmc.getWins()[j]->color);
            drawDvec2(gmc.getWins()[j]->icon, Float4(spos.x + iconborder, spos.y + iconborder, spos.x + iconwidth - iconborder, spos.y + iconwidth - iconborder), 10, 0.0002);
          } else {
            setColor(Color(0.5, 0.5, 0.5));
            drawLine(Float4(spos.x + iconwidth - iconborder * 2, spos.y + iconborder * 2, spos.x + iconborder * 2, spos.y + iconwidth - iconborder * 2), 0.0002);
          }
          spos.x += iconwidth;
          if(j % gmc.getRoundsPerShop() == gmc.getRoundsPerShop() - 1) {
            setColor(Color(1.0, 1.0, 1.0));
            spos.x += lineborder;
            drawLine(spos.x, spos.y - lineextra, spos.x, spos.y + iconwidth + lineextra, 0.0002);
            spos.x += lineborder;
          }
        }
        
        break;
      }
    }
    
    // Collision HUD
    if(FLAGS_debugGraphicsCollisions || FLAGS_debugGraphics) {
      setColor(Color(1.0, 1.0, 1.0));
      drawText(StringPrintf("Collision tokens added: %d", collider.consumeAddedTokens()), 0.02, Float2(0.01, 0.14));
    }

  }
  
};

int Game::winningTeam() const {
  int winteam = -1;
  for(int i = 0; i < tanks.size(); i++) {
    if(tanks[i].isLive()) {
      CHECK(winteam == -1 || winteam == tanks[i].team);
      winteam = tanks[i].team;
    }
  }
  return winteam;
}

vector<int> Game::teamBreakdown() const {
  vector<int> teamsize(teams.size());
  for(int i = 0; i < tanks.size(); i++)
    teamsize[tanks[i].team]++;
  return teamsize;
}

int Game::frameCount() const {
  return frameNm;
}

Coord2 Game::queryPlayerLocation(int id) const {
  return tanks[id].pos;
}

void Game::kill(int id) {
  CHECK(gamemode == GMODE_DEMO);
  tanks[id].takeDamage(1000000000);
}

void Game::respawnPlayer(int id, Coord2 pos, float facing) {
  CHECK(gamemode == GMODE_DEMO);
  tanks[id].respawn(pos, facing, id);
  
  bombards[id].state = BombardmentState::BS_OFF;
  CHECK(tanks[id].isLive());
}

void Game::addStatCycle() {
  bool addahit = false;
  if(demomode_hits)
    addahit = true;
  else
    for(int i = 0; i < tanks.size(); i++)
      if(demomode_playermodes[i] == DEMOPLAYER_DPC && tanks[i].hasTakenDamage())
        addahit = true;
  
  if(addahit) {
    for(int i = 0; i < tanks.size(); i++) {
      if(demomode_playermodes[i] == DEMOPLAYER_DPC) {
        tanks[i].addCycle();
      }
    }
    demomode_hits++;
  }
}

float Game::getBombardmentIncreasePerSec() const {
  int bombardy = 0;
  for(int i = 0; i < bombards.size(); i++)
    if(bombards[i].state != BombardmentState::BS_OFF && bombards[i].state != BombardmentState::BS_SPAWNING)
      bombardy++;
  if(!bombardy)
    return 0;
  return 1 / (15 / ((float)bombardy / tanks.size()));
}
  
float Game::getTimeUntilBombardmentUpgrade() const {
  return (floor(bombardment_tier + 1) - bombardment_tier) / getBombardmentIncreasePerSec();
}

Game::Game() : collider(0, 0) {
  gamemode = GMODE_LAST;
}

void Game::initCommon(const vector<Player*> &in_playerdata, const vector<Color> &in_colors, const vector<vector<Coord2> > &lev, bool smashable) {
  CHECK(gamemode >= 0 && gamemode < GMODE_LAST);
  CHECK(in_playerdata.size() == in_colors.size());
  
  tanks.clear();
  bombards.clear();
  zones.clear();
  teams.clear();
  projectiles.clear();
  gfxeffects.clear();
  
  tanks.resize(in_playerdata.size());
  bombards.resize(in_playerdata.size());
  
  gamemap = Gamemap(lev, smashable);
  
  for(int i = 0; i < tanks.size(); i++) {
    CHECK(in_playerdata[i]);
    tanks[i].init(in_playerdata[i]->getTank(), in_colors[i]);
  }

  frameNm = 0;
  framesSinceOneLeft = 0;
  firepowerSpent = 0;
  
  projectiles.resize(in_playerdata.size());
  
  pair<Float2, Float2> z = getMapZoom(gamemap.getRenderBounds());
  zoom_center = z.first;
  zoom_size = z.second;
  
  zoom_speed = Float2(0, 0);

  collider = Collider(tanks.size(), Coord(20));
  
  teams.resize(tanks.size());
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].team = i;
  
  bombardment_tier = 0;
  
  frameNmToStart = -1000;
  freezeUntilStart = false;
  
  demomode_hits = 0;
}

void Game::initRandomTankPlacement(const map<int, vector<pair<Coord2, float> > > &player_starts, Rng *rng) {
  CHECK(player_starts.count(tanks.size()));
  vector<pair<Coord2, float> > pstart = player_starts.find(tanks.size())->second;
  for(int i = 0; i < tanks.size(); i++) {
    int loc = int(rng->frand() * pstart.size());
    CHECK(loc >= 0 && loc < pstart.size());
    tanks[i].pos = Coord2(pstart[loc].first);
    tanks[i].d = pstart[loc].second;
    pstart.erase(pstart.begin() + loc);
  }
}

vector<Color> createBasicColors(const vector<Player*> &in_playerdata) {
  vector<Color> rv;
  for(int i = 0; i < in_playerdata.size(); i++)
    rv.push_back(in_playerdata[i]->getFaction()->color);
  return rv;
}

void Game::initStandard(vector<Player> *in_playerdata, const Level &lev, Rng *rng) {
  gamemode = GMODE_STANDARD;
  
  vector<Player*> playerdata;
  for(int i = 0; i < in_playerdata->size(); i++)
    playerdata.push_back(&(*in_playerdata)[i]);
  initCommon(playerdata, createBasicColors(playerdata), lev.paths, true);
  initRandomTankPlacement(lev.playerStarts, rng);
  
  frameNmToStart = 180;
  freezeUntilStart = true;
};

void Game::initChoice(vector<Player> *in_playerdata, Rng *rng) {
  gamemode = GMODE_CHOICE;
  
  Level lev = loadLevel("data/levels_special/choice_4.dv2");
  lev.playerStarts.clear();
  vector<Player*> playerdata;
  for(int i = 0; i < in_playerdata->size(); i++) {
    float ang = PI * 2 * i / in_playerdata->size();
    lev.playerStarts[in_playerdata->size()].push_back(make_pair(makeAngle(Coord(ang)) * 20, ang));
    playerdata.push_back(&(*in_playerdata)[i]);
  }
  
  initCommon(playerdata, createBasicColors(playerdata), lev.paths, true);
  initRandomTankPlacement(lev.playerStarts, rng);
  
  teams.resize(5);
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].team = 4;
  
  vector<vector<Coord2> > paths;
  {
    const float dist = 160;
    vector<Coord2> cut;
    cut.push_back(Coord2(0, dist));
    cut.push_back(Coord2(-dist, 0));
    cut.push_back(Coord2(0, -dist));
    cut.push_back(Coord2(dist, 0));
    vector<Coord2> lpi = lev.paths[0];
    reverse(lpi.begin(), lpi.end());
    paths = getDifference(lpi, cut);
  }
  
  Color zonecol[4] = {Color(0.6, 0.6, 0.6), Color(0.3, 0.3, 1.0), Color(0, 1.0, 0), Color(1.0, 0, 0)};
  CHECK(paths.size() == ARRAY_SIZE(zonecol));
  for(int i = 0; i < paths.size(); i++) {
    zones.push_back(make_pair(paths[i], zonecol[i]));
    teams[i].color = zonecol[i];
    teams[i].swap_colors = true;
  }
  
  teams[4].weapons_enabled = false;
  
  frameNmToStart = -1;
  freezeUntilStart = false;
}

void Game::initTest(Player *in_playerdata, const Float4 &bounds) {
  gamemode = GMODE_TEST;
  
  vector<vector<Coord2> > level;
  {
    vector<Coord2> path;
    path.push_back(Coord2(bounds.sx, bounds.sy));
    path.push_back(Coord2(bounds.sx, bounds.ey));
    path.push_back(Coord2(bounds.ex, bounds.ey));
    path.push_back(Coord2(bounds.ex, bounds.sy));
    level.push_back(path);
  }
  
  vector<Player*> playerdata;
  playerdata.push_back(in_playerdata);
  initCommon(playerdata, createBasicColors(playerdata), level, false);
  CHECK(tanks.size() == 1);
  tanks[0].pos = Coord2(bounds.midpoint());
  tanks[0].d = PI / 2 * 3;
  
  clear = bounds;
}

void Game::initDemo(vector<Player> *in_playerdata, float boxradi, const float *xps, const float *yps, const float *facing, const int *modes, bool blockades, Float2 hudpos) {
  gamemode = GMODE_DEMO;
  demo_hudpos = hudpos;
  
  vector<vector<Coord2> > level;
  {
    vector<Coord2> path;
    path.push_back(Coord2(-boxradi, -boxradi));
    path.push_back(Coord2(-boxradi, boxradi));
    path.push_back(Coord2(boxradi, boxradi));
    path.push_back(Coord2(boxradi, -boxradi));
    level.push_back(path);
  }
  
  if(blockades) {
    Coord inside = 1;
    Coord outside = Coord(boxradi) - inside;
    vector<Coord2> blockade;
    blockade.push_back(Coord2(-inside, outside));
    blockade.push_back(Coord2(-inside, inside));
    blockade.push_back(Coord2(-outside, inside));
    blockade.push_back(Coord2(-outside, -inside));
    blockade.push_back(Coord2(-inside, -inside));
    blockade.push_back(Coord2(-inside, -outside));
    blockade.push_back(Coord2(inside, -outside));
    blockade.push_back(Coord2(inside, -inside));
    blockade.push_back(Coord2(outside, -inside));
    blockade.push_back(Coord2(outside, inside));
    blockade.push_back(Coord2(inside, inside));
    blockade.push_back(Coord2(inside, outside));
    level.push_back(blockade);
  }
  
  vector<Player*> playerdata;
  for(int i = 0; i < in_playerdata->size(); i++)
    playerdata.push_back(&(*in_playerdata)[i]);
  
  vector<Color> colors;
  CHECK(playerdata.size() <= factionList().size());
  for(int i = 0; i < playerdata.size(); i++)
    colors.push_back(factionList()[i].color);
  
  initCommon(playerdata, colors, level, false);
  
  for(int i = 0; i < tanks.size(); i++) {
    tanks[i].pos = Coord2(xps[i], yps[i]);
    if(facing)
      tanks[i].d = facing[i];
  }
  
  demomode_playermodes.clear();
  for(int i = 0; i < tanks.size(); i++) {
    CHECK(modes[i] >= 0 && modes[i] < DEMOPLAYER_LAST);
    demomode_playermodes.push_back(modes[i]);
    if(modes[i] == DEMOPLAYER_BOMBSIGHT)
      tanks[i].setDead();
  }
  
  demomode_boxradi = boxradi;
  
  clear = Float4(-boxradi, -boxradi, boxradi, boxradi);
}

void Game::initCenteredDemo(Player *in_playerdata, float zoom) {
  gamemode = GMODE_CENTERED_DEMO;
  
  centereddemo_zoom = zoom;
  
  vector<vector<Coord2> > level;
  {
    const float size = 1000;
    vector<Coord2> path;
    path.push_back(Coord2(-size, -size));
    path.push_back(Coord2(-size, size));
    path.push_back(Coord2(size, size));
    path.push_back(Coord2(size, -size));
    level.push_back(path);
    
    clear = Float4(-size, -size, size, size);
  }
  
  vector<Player*> playerdata;
  playerdata.push_back(in_playerdata);
  initCommon(playerdata, createBasicColors(playerdata), level, false);
  CHECK(tanks.size() == 1);
  tanks[0].pos = Coord2(0, 0);
  tanks[0].d = PI / 2 * 3;
  
  collider = Collider(tanks.size(), Coord(1000));
}

void Game::addTankStatusText(int tankid, const string &text, float duration) {
  Float2 pos;
  if(tanks[tankid].isLive()) {
    pos = tanks[tankid].pos.toFloat();
  } else {
    pos = bombards[tankid].pos.toFloat();
  }
  gfxeffects.push_back(GfxText(pos + Float2(4, -4), Float2(0, -6), zoom_size.y / 80, text, duration, Color(1.0, 1.0, 1.0)));
}

bool GamePackage::runTick(const vector<Keystates> &keys, Rng *rng) {
  vector<Player*> ppt;
  for(int i = 0; i < players.size(); i++)
    ppt.push_back(&players[i]);
  return game.runTick(keys, ppt, rng);
}

void GamePackage::renderToScreen() const {
  vector<const Player*> ppt;
  for(int i = 0; i < players.size(); i++)
    ppt.push_back(&players[i]);
  game.renderToScreen(ppt, GameMetacontext());
}

const vector<const IDBFaction *> &GameMetacontext::getWins() const {
  CHECK(wins);
  return *wins;
};
int GameMetacontext::getRoundsPerShop() const {
  CHECK(roundsPerShop != -1);
  return roundsPerShop;
};

bool GameMetacontext::hasMetacontext() const {
  CHECK(!wins == (roundsPerShop == -1));
  return !!wins;
}

GameMetacontext::GameMetacontext() :
    wins(NULL), roundsPerShop(-1) { };
GameMetacontext::GameMetacontext(const vector<const IDBFaction *> &wins, int roundsPerShop) :
    wins(&wins), roundsPerShop(roundsPerShop) { };
