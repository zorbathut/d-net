
SOURCES = main core game timer debug gfx
CFLAGS = `sdl-config --cflags` -mno-cygwin -O2
LINKFLAGS = `sdl-config --libs` -lglu32 -lopengl32 -lm -mno-cygwin -O2

all: d-net.exe

include $(SOURCES:=.d)

CC			= g++

d-net.exe: $(SOURCES:=.o)
	$(CC)  -o $@ $(SOURCES:=.o) $(LINKFLAGS) 

clean:
	rm -rf *.o *.exe *.d
	
run: d-net.exe
	d-net.exe

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
%.d: %.cpp
	bash -ec '$(CC) $(CFLAGS) -M $< | sed "s/$*.o/$*.o $@/g" > $@'



