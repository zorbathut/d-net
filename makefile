
SOURCES = main core game timer debug gfx collide gamemap util
CFLAGS = `sdl-config --cflags` -mno-cygwin -O2 #-pg
LINKFLAGS = `sdl-config --libs` -lglu32 -lopengl32 -lm -mno-cygwin -O2 #-pg

all: d-net.exe

include $(SOURCES:=.d)

CC			= g++

d-net.exe: $(SOURCES:=.o)
	$(CC)  -o $@ $(SOURCES:=.o) $(LINKFLAGS) 

clean:
	rm -rf *.o *.exe *.d
	
run: d-net.exe
	d-net.exe

package: d-net.exe
	strip d-net.exe
	zip -9 -j `date +c:/inetpub/wwwroot/d-net/dnet%G%m%d%H%M%S.zip` d-net.exe c:/cygwin/usr/local/bin/SDL.dll

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
%.d: %.cpp
	bash -ec '$(CC) $(CFLAGS) -M $< | sed "s/$*.o/$*.o $@/g" > $@'



