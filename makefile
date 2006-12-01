
SOURCES = main core game timer debug gfx collide gamemap util rng args interface vecedit metagame itemdb parse dvec2 input level coord ai inputsnag os_win32 float cfcommon coord_boolean player itemdb_adjust metagame_config shop shop_demo shop_info game_ai game_effects color metagame_tween cfc game_tank game_util game_projectile socket httpd recorder generators audio
CPPFLAGS = `sdl-config --cflags` -DVECTOR_PARANOIA -I/usr/include/boost-1_33_1 -I/usr/ogg/include -Wall -Wno-sign-compare -Wno-uninitialized -g #-pg # I would love to get rid of -Wno-uninitialized, but it makes the standard library spit out warnings! :(
LINKFLAGS = `sdl-config --libs` -L/usr/ogg/lib -lglu32 -lopengl32 -lm -lws2_32 -lvorbisfile -lvorbis -logg -g #-pg
DATAFILES = $(shell find data | grep -v \.svn | grep -v shopcache.dwh)

CPP = g++

all: d-net.exe

include $(SOURCES:=.d)

d-net.exe: $(SOURCES:=.o)
	nice $(CPP) -o $@ $(SOURCES:=.o) $(LINKFLAGS)

d-net-dbg.exe: $(SOURCES:=.do)
	nice $(CPP) -o $@ $(SOURCES:=.do) $(LINKFLAGS) 

asm: $(SOURCES:=.S) makefile

clean:
	rm -rf *.o *.do *.exe *.d *.S

STDRUN = --nofullscreen --debugitems --startingCash=100000000 --debugControllers=2 --factionMode=0 --nullControllers=11 --writeTarget= --auto_newgame --nocullShopTree --httpd_port=616 --noshopcache

run: d-net.exe
	d-net.exe $(STDRUN)
  
basicrun: d-net.exe
	d-net.exe --nofullscreen --writeTarget= --httpd_port=616

cacherun: d-net.exe
	d-net.exe $(STDRUN) --shopcache

threerun: d-net.exe
	d-net.exe $(STDRUN) --debugControllers=3
  
debugrun: d-net-dbg.exe
	d-net-dbg.exe $(STDRUN)

debugbasicrun: d-net-dbg.exe
	d-net-dbg.exe --nofullscreen --writeTarget= --httpd_port=616

ai: d-net.exe
	d-net.exe --nofullscreen --aiCount=12 --fastForwardTo=100000000 --factionMode=0 --httpd_port=616 --noshopcache

ailoop: d-net.exe
	while ./d-net.exe --nofullscreen --aiCount=12 --fastForwardTo=100000000 --terminateAfter=600 --startingCash=100000000 --httpd_port=616 --noshopcache ; do echo Cycle. ; done

vecedit: d-net.exe
	d-net.exe --vecedit --nofullscreen
    
package: d-net.exe data/shopcache.dwh
	rm \\\\192.168.100.4\\zorba\\www\\d-net\\Dnet\ Latest\ Version.zip
	mkdir deploy
	cp d-net.exe deploy
	cp c:/cygwin/usr/local/bin/SDL.dll deploy
	cp /usr/ogg/bin/* deploy
	cp -r data deploy
	mkdir deploy/dumps
	cp dumps/readme.txt deploy/dumps
	rm -f deploy/data/coordfailure
	cd deploy ; rm -rf `find | grep .svn`
	strip deploy/d-net.exe
	cd deploy ; zip -9 -r \\\\192.168.100.4\\zorba\\www\\d-net\\Dnet\ Latest\ Version.zip *  # This is really too many backslashes.
	cp \\\\192.168.100.4\\zorba\\www\\d-net\\Dnet\ Latest\ Version.zip `date +\\\\\\\\192.168.100.4\\\\zorba\\\\www\\\\d-net\\\\dnet%G%m%d%H%M%S.zip` # This is really too many backslashes.
	rm -rf deploy

%.o: %.cpp
	nice $(CPP) $(CPPFLAGS) -O2 -c -o $@ $<
  
%.do: %.cpp
	nice $(CPP) $(CPPFLAGS) -c -o $@ $<

%.S: %.cpp
	nice $(CPP) $(CPPFLAGS) -c -g -Wa,-a,-ad $< > $@

%.d: %.cpp
	nice bash -ec '$(CPP) $(CPPFLAGS) -MM $< | sed "s!$*.o!$*.o $*.do $@!g" > $@'

data/shopcache.dwh: d-net.exe $(DATAFILES)
	d-net.exe --generateCachedShops=0.99

export: d-net.exe tools/generateWeaponGraph.py
	d-net.exe --generateWeaponStats
	/cygdrive/c/Python25/python.exe tools/generateWeaponGraph.py # this is not ideal

stats:
	@echo Graphics: `cat gfx.h gfx.cpp game_effects.h game_effects.cpp color.h color.cpp | wc -l` loc
  @echo Sound: `cat audio.h audio.cpp | wc -l` loc
	@echo Collisions: `cat collide.h collide.cpp | wc -l` loc
	@echo Game mechanics: `cat game.h game.cpp gamemap.h gamemap.cpp level.cpp level.h player.cpp player.h game_tank.cpp game_tank.h game_util.cpp game_util.h game_projectile.h game_projectile.cpp | wc -l` loc
	@echo Item hierarchy: `cat itemdb.h itemdb.cpp parse.h parse.cpp itemdb_adjust.cpp | wc -l` loc
	@echo UI: `cat interface.h interface.cpp metagame.h metagame.cpp metagame_config.cpp metagame_config.h shop.cpp shop.h shop_demo.cpp shop_demo.h shop_info.cpp shop_info.h game_ai.h game_ai.cpp metagame_tween.cpp metagame_tween.h | wc -l` loc
	@echo Framework: `cat core.h core.cpp main.cpp input.h input.cpp inputsnag.h inputsnag.cpp os.h os_gen.cpp os_win32.cpp debug.h debug.cpp recorder.h recorder.cpp generators.h generators.cpp | wc -l` loc
	@echo Util: `cat timer.h timer.cpp util.h util.cpp args.h args.cpp rng.h rng.cpp coord.h coord.cpp float.h float.cpp cfcommon.h cfcommon.cpp coord_boolean.cpp cfc.h cfc.cpp noncopyable.h smartptr.h | wc -l` loc
	@echo Networking: `cat httpd.h httpd.cpp socket.h socket.cpp | wc -l` loc
	@echo Vector system: `cat vecedit.h vecedit.cpp dvec2.h dvec2.cpp | wc -l` loc
	@echo AI: `cat ai.h ai.cpp | wc -l` loc
	@echo Total code: `cat *.h *.cpp makefile | wc -l` loc
	@echo Datafiles: `cd data; cat \`find -type f | grep -v .svn\` | wc -l` lines
