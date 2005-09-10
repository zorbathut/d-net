
SOURCES = main core game timer debug gfx collide gamemap util rng const args interface vecedit metagame itemdb parse dvec2 input level
CPPFLAGS = `sdl-config --cflags` -mno-cygwin -O2 -DVECTOR_PARANOIA -Wall -Wno-sign-compare -Wno-uninitialized#-pg
LINKFLAGS = `sdl-config --libs` -lglu32 -lopengl32 -lm -mno-cygwin -O2 #-pg

CPP = g++

all: d-net.exe

include $(SOURCES:=.d)

d-net.exe: $(SOURCES:=.o)
	$(CPP) -o $@ $(SOURCES:=.o) $(LINKFLAGS) 

asm: $(SOURCES:=.S)

clean:
	rm -rf *.o *.exe *.d *.S
	
run: d-net.exe
	d-net.exe --nofullscreen

vecedit: d-net.exe
	d-net.exe --vecedit --nofullscreen
    
package: d-net.exe
	mkdir deploy
	cp d-net.exe deploy
	cp c:/cygwin/usr/local/bin/SDL.dll deploy
	cp -r data deploy
	cd deploy ; rm -rf `find | grep .svn`
	strip deploy/d-net.exe
	cd deploy ; zip -9 -r `date +x:/d-net/dnet%G%m%d%H%M%S.zip` *
	rm -rf deploy

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.S: %.cpp
	$(CPP) $(CPPFLAGS) -c -g -Wa,-a,-ad $< > $@

%.d: %.cpp
	bash -ec '$(CPP) $(CPPFLAGS) -MM $< | sed "s/$*.o/$*.o $@/g" > $@'
	
stats:
	@echo Graphics: `cat gfx.h gfx.cpp | wc -l` loc
	@echo Collisions: `cat collide.h collide.cpp | wc -l` loc
	@echo Game mechanics: `cat game.h game.cpp gamemap.h gamemap.cpp level.cpp level.h | wc -l` loc
	@echo Item hierarchy: `cat itemdb.h itemdb.cpp parse.h parse.cpp | wc -l` loc
	@echo UI: `cat interface.h interface.cpp metagame.h metagame.cpp | wc -l` loc
	@echo Framework: `cat core.h core.cpp main.h main.cpp input.h input.cpp | wc -l` loc
	@echo Util: `cat timer.h timer.cpp util.h util.cpp args.h args.cpp | wc -l` loc
	@echo Vector system: `cat vecedit.h vecedit.cpp dvec2.h dvec2.cpp | wc -l` loc
	@echo Total code: `cat *.h *.cpp makefile | wc -l` loc
	@echo Datafiles: `cd data; cat \`find -type f | grep -v .svn\` | wc -l` lines
