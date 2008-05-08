
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

