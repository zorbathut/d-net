
from SConstruct_config import Conf

# Globals
Decider('MD5-timestamp')
SetOption('implicit_cache', 1)

env = Conf()

# List of buildables
# Game
# Editor
# Reporter
# Merger
# ods2csv

# Data accumulation
# Base copy data
# Base oggized data

# Released data
# Demoed data
# Deployed data

if 0:
  for item in ["GAME", "EDITOR", "REPORTER", "CONSOLE"]:
    for flag in ["CCFLAGS", "CPPFLAGS", "CXXFLAGS", "LINKFLAGS", "LIBS", "CPPPATH", "LIBPATH", "CPPDEFINES"]:
      cube = flag + "_" + item
      print cube, "is", env.subst("$" + cube)


# deploy directory and associated
def commandstrip(env, source):
  env.Command('deploy/%s' % source.split('/')[-1], source, "cp $SOURCE $TARGET && strip -s $TARGET")

for item in "SDL.dll libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll".split(" "):
  commandstrip(env, "/usr/mingw/local/bin/%s" % item)
commandstrip(env, "d-net.exe")

# version.cpp
env.Command('version.cpp', 'version_data version_gen.py'.split(" "), "./version_gen.py < version_data > $TARGET")
