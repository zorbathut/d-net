
SOURCES = main core game timer debug gfx collide gamemap util rng const
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

package: d-net.exe
	strip d-net.exe
	zip -9 -j `date +x:/d-net/dnet%G%m%d%H%M%S.zip` d-net.exe c:/cygwin/usr/local/bin/SDL.dll

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<
	
%.d: %.cpp
	bash -ec '$(CPP) $(CPPFLAGS) -M $< | sed "s/$*.o/$*.o $@/g" > $@'
	
stats:
	@echo Graphics: `cat gfx.h gfx.cpp | wc -l` loc
	@echo Collisions: `cat collide.h collide.cpp | wc -l` loc
	@echo Game mechanics: `cat game.h game.cpp gamemap.h gamemap.cpp | wc -l` loc
	@echo Total project: `cat *.h *.cpp makefile | wc -l` loc




