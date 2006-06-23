
SOURCES = main core game timer debug gfx collide gamemap util rng args interface vecedit metagame itemdb parse dvec2 input level coord ai inputsnag os_win32 float cfcommon coord_boolean player itemdb_adjust metagame_config shop shop_info
CPPFLAGS = `sdl-config --cflags` -mno-cygwin -DVECTOR_PARANOIA -Wall -Wno-sign-compare -Wno-uninitialized -O2 #-g -pg
LINKFLAGS = `sdl-config --libs` -lglu32 -lopengl32 -lm -mno-cygwin -O2 #-g -pg

CPP = g++

all: d-net.exe

include $(SOURCES:=.d)

d-net.exe: $(SOURCES:=.o) makefile
	$(CPP) -o $@ $(SOURCES:=.o) $(LINKFLAGS) 

asm: $(SOURCES:=.S) makefile

clean:
	rm -rf *.o *.exe *.d *.S

testrun: d-net.exe
	d-net.exe --nofullscreen --debugitems --startingCash=100000 --debugControllers=2 --factionMode=0

run: d-net.exe
	d-net.exe --nofullscreen

ai: d-net.exe
	d-net.exe --nofullscreen --aiCount=12 --fastForwardTo=100000000 --factionMode=0

ailoop: d-net.exe
	while ./d-net.exe --nofullscreen --aiCount=12 --fastForwardTo=100000000 --terminateAfter=600 --startingCash=100000000 ; do echo Cycle. ; done

vecedit: d-net.exe
	d-net.exe --vecedit --nofullscreen
    
package: d-net.exe
	rm \\\\192.168.100.2\\www-data\\d-net\\Dnet\ Latest\ Version.zip
	mkdir deploy
	cp d-net.exe deploy
	cp c:/cygwin/usr/local/bin/SDL.dll deploy
	cp -r data deploy
	mkdir deploy/dumps
	cp dumps/readme.txt deploy/dumps
	rm -f deploy/data/coordfailure
	cd deploy ; rm -rf `find | grep .svn`
	strip deploy/d-net.exe
	cd deploy ; zip -9 -r \\\\192.168.100.2\\www-data\\d-net\\Dnet\ Latest\ Version.zip *  # This is really too many backslashes.
	cp \\\\192.168.100.2\\www-data\\d-net\\Dnet\ Latest\ Version.zip `date +\\\\\\\\192.168.100.2\\\\www-data\\\\d-net\\\\dnet%G%m%d%H%M%S.zip` # This is really too many backslashes.
	rm -rf deploy

%.o: %.cpp makefile
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.S: %.cpp makefile
	$(CPP) $(CPPFLAGS) -c -g -Wa,-a,-ad $< > $@

%.d: %.cpp makefile
	bash -ec '$(CPP) $(CPPFLAGS) -MM $< | sed "s!$*.o!$*.o $@!g" > $@'
	
stats:
	@echo Graphics: `cat gfx.h gfx.cpp | wc -l` loc
	@echo Collisions: `cat collide.h collide.cpp | wc -l` loc
	@echo Game mechanics: `cat game.h game.cpp gamemap.h gamemap.cpp level.cpp level.h player.cpp player.h | wc -l` loc
	@echo Item hierarchy: `cat itemdb.h itemdb.cpp parse.h parse.cpp itemdb_adjust.cpp | wc -l` loc
	@echo UI: `cat interface.h interface.cpp metagame.h metagame.cpp metagame_config.cpp metagame_config.h shop.cpp shop.h shop_info.cpp shop_info.h | wc -l` loc
	@echo Framework: `cat core.h core.cpp main.h main.cpp input.h input.cpp inputsnag.h inputsnag.cpp os.h os_gen.cpp os_win32.cpp | wc -l` loc
	@echo Util: `cat timer.h timer.cpp util.h util.cpp args.h args.cpp rng.h rng.cpp coord.h coord.cpp float.h float.cpp cfcommon.h cfcommon.cpp coord_boolean.cpp | wc -l` loc
	@echo Vector system: `cat vecedit.h vecedit.cpp dvec2.h dvec2.cpp | wc -l` loc
	@echo AI: `cat ai.h ai.cpp | wc -l` loc
	@echo Total code: `cat *.h *.cpp makefile | wc -l` loc
	@echo Datafiles: `cd data; cat \`find -type f | grep -v .svn\` | wc -l` lines
