
#include "metagame.h"

#include "ai.h"
#include "args.h"
#include "debug.h"
#include "game_tank.h"
#include "gfx.h"
#include "inputsnag.h"
#include "parse.h"
#include "player.h"

#include <fstream>

using namespace std;

DEFINE_int(factionMode, 0, "Faction mode to skip faction choice battle");
// Change to -1 to enable faction mode battle

bool Metagame::runTick(const vector<Controller> &keys) {
  if(mode == MGM_PLAYERCHOOSE) {
    CHECK(persistent.isPlayerChoose());
    if(persistent.tick(keys)) {
      mode = MGM_FACTIONTYPE;

      findLevels(persistent.players().size());

      faction_mode_players = persistent.players();
      game.initChoice(&faction_mode_players);
    }
  } else if(mode == MGM_FACTIONTYPE) {
    StackString stp("Factiontype");
    dprintf("factiontype tick");
    vector<Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    if(FLAGS_factionMode != -1 || game.runTick(persistent.genKeystates(keys), ppt)) {
      if(FLAGS_factionMode != -1)
        faction_mode = FLAGS_factionMode;
      else
        faction_mode = game.winningTeam();
      if(faction_mode == -1) {
        vector<int> teams = game.teamBreakdown();
        CHECK(teams.size() == 5);
        teams.erase(teams.begin() + 4);
        int smalteam = *min_element(teams.begin(), teams.end());
        vector<int> smalteams;
        for(int i = 0; i < teams.size(); i++)
          if(teams[i] == smalteam)
            smalteams.push_back(i);
        faction_mode = smalteams[int(frand() * smalteams.size())];
      }
      CHECK(faction_mode >= 0 && faction_mode < FACTION_LAST);

      mode = MGM_TWEEN;
      
      faction_mode_players.clear();
      // Their job is done. Accumulated profit will vanish along with them.
      // Thanks, players.
      // Thlayers.
    }
  } else if(mode == MGM_TWEEN) {
    if(persistent.tick(keys)) {
      mode = MGM_PLAY;

      findLevels(persistent.players().size());  // player count may have changed. TODO: make this suck less
      game.initStandard(&persistent.players(), levels[int(frand() * levels.size())], &win_history);
      if(win_history.size() != gameround)
        dprintf("%d, %d\n", win_history.size(), gameround);
      CHECK(win_history.size() == gameround);
    }
  } else if(mode == MGM_PLAY) {
    StackString stp("Play");
    vector<Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    if(game.runTick(persistent.genKeystates(keys), ppt)) {
      gameround++;
      if(gameround % roundsBetweenShop == 0) {
        mode = MGM_TWEEN;
        persistent.divvyCash(game.firepowerSpent);
      } else {
        // store the firepower, restart the game, and add firepower to it (kind of kludgy)
        float firepower = game.firepowerSpent;
        game.initStandard(&persistent.players(), levels[int(frand() * levels.size())], &win_history);
        game.firepowerSpent = firepower;
      }
    }
  } else {
    CHECK(0);
  }
  return false;
}
  
void Metagame::ai(const vector<Ai *> &ai) const {
  StackString stp("Metagame AI");
  if(mode == MGM_PLAYERCHOOSE || mode == MGM_TWEEN) {
    persistent.ai(ai);
  } else if(mode == MGM_FACTIONTYPE || mode == MGM_PLAY) {
    vector<Ai *> rai = persistent.distillAi(ai);
    vector<GameAi *> gai;
    for(int i = 0; i < rai.size(); i++)
      if(rai[i])
        gai.push_back(rai[i]->getGameAi());
      else
        gai.push_back(NULL);
    game.ai(gai);
  } else {
    CHECK(0);
  }
}

void Metagame::renderToScreen() const {
  if(mode == MGM_PLAYERCHOOSE) {
    persistent.render();
  } else if(mode == MGM_FACTIONTYPE) {
    StackString stp("Factiontype");
    vector<const Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    game.renderToScreen(ppt);
    if(!controls_users()) {    
      setColor(1.0, 1.0, 1.0);
      setZoom(Float4(0, 0, 133.333, 100));
      drawText(StringPrintf("faction setting round"), 2, Float2(5, 82));
    }
  } else if(mode == MGM_TWEEN) {
    persistent.render();
  } else if(mode == MGM_PLAY) {
    StackString stp("Play");
    vector<const Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    game.renderToScreen(ppt);
    if(!controls_users()) {    
      setColor(1.0, 1.0, 1.0);
      setZoom(Float4(0, 0, 133.333, 100));
      drawText(StringPrintf("round %d", gameround), 2, Float2(5, 82));
    }
  } else {
    CHECK(0);
  }
}

void Metagame::findLevels(int playercount) {
  levels.clear();
  if(!levels.size()) {
    ifstream ifs("data/levels/levellist.txt");
    string line;
    while(getLineStripped(ifs, &line)) {
      Level lev = loadLevel("data/levels/" + line);
      if(lev.playersValid.count(playercount))
        levels.push_back(lev);
    }
    dprintf("Got %d usable levels for %d\n", levels.size(), playercount);
    CHECK(levels.size());
  }
}

Metagame::Metagame(int playercount, int in_roundsBetweenShop) :
    persistent(playercount, in_roundsBetweenShop) {
  
  if(FLAGS_factionMode != -1) {
    mode = MGM_TWEEN;
    persistent.startAtNormalShop();
  } else {
    mode = MGM_PLAYERCHOOSE;
  }
  
  faction_mode = FLAGS_factionMode;
  roundsBetweenShop = in_roundsBetweenShop;
  gameround = 0;
  CHECK(roundsBetweenShop >= 1);
}
