
SOURCES = main core game timer debug gfx collide gamemap util rng const args interface vecedit
CPPFLAGS = `sdl-config --cflags` -mno-cygwin -O2 #-pg
LINKFLAGS = `sdl-config --libs` -lglu32 -lopengl32 -lm -mno-cygwin -O2 #-pg

CPP = g++

all: d-net.exe

include $(SOURCES:=.d)

d-net.exe: $(SOURCES:=.o)
	$(CPP) -o $@ $(SOURCES:=.o) $(LINKFLAGS) 

clean:
	rm -rf *.o *.exe *.d
	
run: d-net.exe
	d-net.exe

vecedit: d-net.exe
	d-net.exe --vecedit
    
package: d-net.exe
	mkdir deploy
	cp d-net.exe deploy
	cp c:/cygwin/usr/local/bin/SDL.dll deploy
	cp -r data deploy
	rm -rf deploy/data/.svn
	strip deploy/d-net.exe
	cd deploy ; zip -9 -r `date +x:/d-net/dnet%G%m%d%H%M%S.zip` *
	rm -rf deploy

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<
	
%.d: %.cpp
	bash -ec '$(CPP) $(CPPFLAGS) -M $< | sed "s/$*.o/$*.o $@/g" > $@'
	
stats:
	@echo Graphics: `cat gfx.h gfx.cpp | wc -l` loc
	@echo Collisions: `cat collide.h collide.cpp | wc -l` loc
	@echo Game mechanics: `cat game.h game.cpp gamemap.h gamemap.cpp | wc -l` loc
	@echo UI: `cat interface.h interface.cpp | wc -l` loc
	@echo Framework: `cat core.h core.cpp main.h main.cpp | wc -l` loc
	@echo Util: `cat timer.h timer.cpp util.h util.cpp args.h args.cpp | wc -l` loc
	@echo Vector editor: `cat vecedit.h vecedit.cpp | wc -l` loc
	@echo Total code: `cat *.h *.cpp makefile | wc -l` loc
	@echo Datafiles: `cat data/* | wc -l` lines





