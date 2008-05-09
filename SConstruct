
from SConstruct_config import Conf

# Globals
Decider('MD5-timestamp')
SetOption('implicit_cache', 1)

env, categories, flagtypes = Conf()

# List of buildables
buildables = [
  ("d-net", "GAME", Split("main core game timer debug gfx collide gamemap util rng args interface metagame itemdb itemdb_adjust itemdb_parse parse dvec2 input level coord ai inputsnag os float cfcommon coord_boolean player metagame_config shop shop_demo shop_info game_ai game_effects color metagame_tween cfc game_tank game_util game_projectile socket httpd recorder generators audio itemdb_httpd test regex shop_layout perfbar adler32 money stream stream_file itemdb_stream res_interface settings dumper audit stream_gz dumper_registry version")),
  ("vecedit", "EDITOR", Split("vecedit vecedit_main debug os util gfx float coord parse color dvec2 cfcommon input itemdb itemdb_adjust itemdb_parse args regex rng adler32 money perfbar timer stream stream_file itemdb_stream image dumper_registry")),
  ("reporter", "REPORTER", Split("reporter_main debug os util parse")),
  ("merger", "CONSOLE_MERGER", Split("merger debug os util parse itemdb itemdb_adjust itemdb_parse args color dvec2 float coord cfcommon merger_weapon merger_bombardment merger_tanks merger_glory merger_util merger_upgrades merger_factions regex rng adler32 money stream stream_file itemdb_stream dumper_registry")),
  ("ods2csv", "CONSOLE_ODS2CSV", Split("ods2csv debug os util parse adler32"), Split("minizip/unzip minizip/ioapi"))
]

# Data accumulation
# Base copy data
# Base oggized data

# Released data
# Demoed data

deploy_dlls = Split("SDL.dll libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll")
deploy = deploy_dlls + ["d-net.exe", "license.txt"]

if 0:
  for item in categories:
    for flag in flagtypes:
      cube = flag + "_" + item
      print cube, "is", env.subst("$" + cube)

to_build = []
built = {}

for build in buildables:
  params = {}
  for param in flagtypes:
    buildstring = param + "_" + build[1];
    params[param] = []
    for i in range(buildstring.count("_"), -1, -1):
      params[param] += env[buildstring.rsplit("_", i)[0]]
  
  abbreviation = "".join(x.lower()[0] for x in build[1].split("_"))
  
  objects = []
  
  for item in build[2]:
    if not (build[1], item) in built:
      built[(build[1], item)] = env.Object(source="%s.cpp" % item, target="build/%s.%s.o" % (item, abbreviation), **params);
    objects += built[(build[1], item)]
  
  if len(build) > 3:
    for item in build[3]:
      if not (build[1], item) in built:
        built[(build[1], item)] = env.Object(source="%s.c" % item, target="build/%s.%s.o" % (item, abbreviation), **params)
      objects += built[(build[1], item)]
  
  env.Program(build[0], objects, **params)

# deploy directory and associated
def commandstrip(env, source):
  env.Command('deploy/%s' % source.split('/')[-1], source, "cp $SOURCE $TARGET && strip -s $TARGET")

for item in deploy_dlls:
  commandstrip(env, "/usr/mingw/local/bin/%s" % item)
commandstrip(env, "d-net.exe")
env.Command('deploy/license.txt', 'resources/license.txt', Copy("$TARGET", '$SOURCE'))

# version.cpp
env.Command('version.cpp', Split('version_data version_gen.py'), "./version_gen.py < version_data > $TARGET")
