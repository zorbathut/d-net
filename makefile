
SOURCES = main core game timer debug
CFLAGS = `sdl-config --cflags` -mno-cygwin
LINKFLAGS = `sdl-config --libs` -lglu32 -lopengl32 -lm -mno-cygwin

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
	bash -ec '$(CC) $(CFLAGS) -MM $< | sed "s/$*.o/$*.o $@/g" > $@'



