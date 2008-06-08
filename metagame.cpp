
#include "metagame.h"
#include "adler32_util.h"
#include "ai.h"
#include "audit.h"
#include "dumper_registry.h"
#include "game_projectile.h"
#include "game_tank.h"
#include "gfx.h"
#include "parse.h"
#include "perfbar.h"

#include <fstream>
using namespace std;



DEFINE_string(singlelevel, "", "The name of a level to load and use exclusively");
REGISTER_string(singlelevel);

DECLARE_bool(renderframenumber);


void Metagame::instant_action_init(const ControlConsts &ck, int primary_id) {
  persistent.instant_action_init(ck, primary_id);
  instant_action_keys = true;
}

bool Metagame::runTick(const vector<Controller> &keys, bool confused, const InputSnag &is) {
  if(mode == MGM_PLAYERCHOOSE) {
    CHECK(persistent.isPlayerChoose());
    if(persistent.tick(keys, is)) {
      mode = MGM_FACTIONTYPE;

      findLevels(persistent.players().size());

      faction_mode_players = persistent.players();
      game.initChoice(&faction_mode_players, &rng);
    }
  } else if(mode == MGM_FACTIONTYPE) {
    StackString stp("Factiontype");
    vector<Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    
    // This is the "do init faction-related stuff" routine. If the faction_mode isn't -1 then we go straight into intting. Otherwise, we run the Faction Game, and only init once it's done.
    if(faction_mode != -1 || game.runTick(persistent.genKeystates(keys), confused, ppt, &rng)) {
      if(faction_mode == -1)  // We must have been doing the faction game, so pull the winning team from that.
        faction_mode = game.winningTeam();
      
      if(faction_mode == -1) {  // Not only were we doing the faction game, but there *was no winner*. Penalize people for sucking >:(
        vector<int> teams = game.teamBreakdown();
        CHECK(teams.size() == 5);
        teams.erase(teams.begin() + 4);
        int smalteam = *min_element(teams.begin(), teams.end());
        vector<int> smalteams;
        for(int i = 0; i < teams.size(); i++)
          if(teams[i] == smalteam)
            smalteams.push_back(i);
        faction_mode = smalteams[int(rng.frand() * smalteams.size())];
      }
      CHECK(faction_mode >= 0 && faction_mode < FACTION_LAST);

      mode = MGM_TWEEN;
      instant_action_keys = false;
      
      faction_mode_players.clear();
      // Their job is done. Accumulated profit will vanish along with them.
      // Thanks, players.
      // Thlayers.
      
      persistent.setFactionMode(faction_mode);
    }
  } else if(mode == MGM_TWEEN) {
    PersistentData::PDRTR rv = persistent.tick(keys, is);
    if(rv == PersistentData::PDRTR_PLAY) {
      mode = MGM_PLAY;
      
      findLevels(persistent.players().size());  // player count may have changed. TODO: make this suck less
      game.initStandard(&persistent.players(), chooseLevel(), &rng, instant_action_keys);
      if(win_history.size() != gameround)
        dprintf("%d, %d\n", win_history.size(), gameround);
      CHECK(win_history.size() == gameround);
    } else if(rv == PersistentData::PDRTR_EXIT) {
      return true;
    }
  } else if(mode == MGM_PLAY) {
    StackString stp("Play");
    vector<Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    if(game.runTick(persistent.genKeystates(keys), confused, ppt, &rng)) {
      if(game.winningTeam() == -1)
        win_history.push_back(NULL);
      else
        win_history.push_back(ppt[game.winningTeam()]->getFaction());
      gameround++;
      CHECK(gameround == win_history.size());
      if(gameround % roundsBetweenShop == 0) {
        mode = MGM_TWEEN;
        instant_action_keys = false;
        persistent.divvyCash();
      } else {
        game.initStandard(&persistent.players(), chooseLevel(), &rng, instant_action_keys);
      }
    }
  } else {
    CHECK(0);
  }
  return false;
}

void Metagame::endgame() {
  persistent.endgame(gameround % roundsBetweenShop, mode == MGM_PLAY);
  mode = MGM_TWEEN;
}

void Metagame::checksum(Adler32 *adl) const {
  StackString sstr("WE CHECKSUM META");
  audit(*adl);
  adler(adl, mode);
  adler(adl, faction_mode);
  adler(adl, gameround);
  adler(adl, roundsBetweenShop);
  adler(adl, last_level);
  adler(adl, rng);
  audit(*adl);
  adler(adl, win_history);
  audit(*adl);
  adler(adl, faction_mode_players);
  audit(*adl);
  persistent.checksum(adl);
  audit(*adl);
  game.checksum(adl);
  audit(*adl);
};
  
void Metagame::ai(const vector<Ai *> &ai, const vector<bool> &isHuman) const {
  StackString stp("Metagame AI");
  if(mode == MGM_PLAYERCHOOSE || mode == MGM_TWEEN) {
    persistent.ai(ai, isHuman);
  } else if(mode == MGM_FACTIONTYPE || mode == MGM_PLAY) {
    vector<Ai *> rai = persistent.distillAi(ai);
    vector<GameAi *> gai;
    for(int i = 0; i < rai.size(); i++)
      if(rai[i])
        gai.push_back(rai[i]->getGameAi());
      else
        gai.push_back(NULL);
    game.ai(gai);
    
    for(int i = 0; i < ai.size(); i++)
      if(ai[i] && !count(rai.begin(), rai.end(), ai[i]))
        ai[i]->updateIdle();
  } else {
    CHECK(0);
  }
}

bool Metagame::isWaitingOnAi() const {
  if(mode == MGM_PLAYERCHOOSE || mode == MGM_TWEEN)
    return persistent.isWaitingOnAi();
  return false;
}

void Metagame::renderToScreen(const InputSnag &is) const {
  PerfStack pst(PBC::rendermeta);
  
  if(mode == MGM_PLAYERCHOOSE) {
    persistent.render(is);
  } else if(mode == MGM_FACTIONTYPE) {
    StackString stp("Factiontype");
    vector<const Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    game.renderToScreen(ppt, GameMetacontext());
    if(!is.users() && FLAGS_renderframenumber) {    
      setColor(1.0, 1.0, 1.0);
      setZoomVertical(0, 0, 100);
      drawText(StringPrintf("faction setting round"), 2, Float2(5, 82));
    }
  } else if(mode == MGM_TWEEN) {
    persistent.render(is);
  } else if(mode == MGM_PLAY) {
    StackString stp("Play");
    vector<const Player *> ppt;
    {
      for(int i = 0; i < persistent.players().size(); i++)
        ppt.push_back(&persistent.players()[i]);
    }
    game.renderToScreen(ppt, GameMetacontext(win_history, roundsBetweenShop));
    if(!is.users() && FLAGS_renderframenumber) {    
      setColor(1.0, 1.0, 1.0);
      setZoomVertical(0, 0, 100);
      drawText(StringPrintf("round %d", gameround), 2, Float2(5, 82));
    }
  } else {
    CHECK(0);
  }
}

void Metagame::findLevels(int playercount) {
  levels.clear();
  if(FLAGS_singlelevel.size()) {
    Level lev = loadLevel(FLAGS_singlelevel);
    levels.push_back(lev);
    dprintf("Used single level %s\n", FLAGS_singlelevel.c_str());
  } else {
    ifstream ifs((FLAGS_fileroot + "levels/levellist.txt").c_str());
    CHECK(ifs);
    string line;
    while(getLineStripped(ifs, &line)) {
      Level lev = loadLevel(FLAGS_fileroot + "levels/" + line);
      if(lev.playersValid.count(playercount))
        levels.push_back(lev);
    }
    dprintf("Got %d usable levels for %d\n", levels.size(), playercount);
    CHECK(levels.size());
  }
}

Level Metagame::chooseLevel() {
  while(1) {
    int cs = int(rng.frand() * levels.size());
    if(cs != last_level || levels.size() == 1) {
      last_level = cs;
      return levels[cs];
    }
  }
}

Metagame::Metagame(const vector<bool> &humans, Money startingcash, Coord multiple, int faction, int in_roundsBetweenShop, int rounds_until_end, RngSeed seed, const InputSnag &isnag) :
    persistent(humans, startingcash, multiple, in_roundsBetweenShop, rounds_until_end, isnag), rng(seed) {
  CHECK(multiple > 1);
  
  faction_mode = faction;
  if(faction_mode != -1) {
    mode = MGM_TWEEN;
    persistent.setFactionMode(faction_mode);
    persistent.startAtNormalShop();
  } else {
    mode = MGM_PLAYERCHOOSE;
  }
  
  roundsBetweenShop = in_roundsBetweenShop;
  gameround = 0;
  CHECK(roundsBetweenShop >= 1);
  
  last_level = -1;
  
  instant_action_keys = false;
}
