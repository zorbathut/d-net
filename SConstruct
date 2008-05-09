
from SConstruct_config import Conf
from util import traverse
from makeinstaller import generateInstaller

# Globals
Decider('MD5-timestamp')
SetOption('implicit_cache', 1)

env, categories, flagtypes, oggpath = Conf()

# List of buildables
buildables = [
  ("d-net", "GAME", Split("main core game timer debug gfx collide gamemap util rng args interface metagame itemdb itemdb_adjust itemdb_parse parse dvec2 input level coord ai inputsnag os float cfcommon coord_boolean player metagame_config shop shop_demo shop_info game_ai game_effects color metagame_tween cfc game_tank game_util game_projectile socket httpd recorder generators audio itemdb_httpd test regex shop_layout perfbar adler32 money stream stream_file itemdb_stream res_interface settings dumper audit stream_gz dumper_registry version")),
  ("vecedit", "EDITOR", Split("vecedit vecedit_main debug os util gfx float coord parse color dvec2 cfcommon input itemdb itemdb_adjust itemdb_parse args regex rng adler32 money perfbar timer stream stream_file itemdb_stream image dumper_registry")),
  ("reporter", "REPORTER", Split("reporter_main debug os util parse")),
  ("merger", "CONSOLE_MERGER", Split("merger debug os util parse itemdb itemdb_adjust itemdb_parse args color dvec2 float coord cfcommon merger_weapon merger_bombardment merger_tanks merger_glory merger_util merger_upgrades merger_factions regex rng adler32 money stream stream_file itemdb_stream dumper_registry")),
  ("ods2csv", "CONSOLE_ODS2CSV", Split("ods2csv debug os util parse adler32"), Split("minizip/unzip minizip/ioapi"))
]

deploy_dlls = Split("SDL.dll libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll")
deploy = deploy_dlls + ["d-net.exe", "license.txt"]

if 0:
  for item in categories:
    for flag in flagtypes:
      cube = flag + "_" + item
      print cube, "is", env.subst("$" + cube)

to_build = []
built = {}
programs = {}

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
  
  programs[build[0]] = env.Program(build[0], objects, **params)

# data copying and merging
data_source = traverse("data_source")

data_vecedit = [x for x in data_source if x.split('/')[0] == "vecedit"]
data_oggize = [x for x in data_source if x.split('.')[-1] == "wav"]
data_merge = [x for x in data_source if x.split('.')[-1] == "unmerged"]
data_copy = [x for x in data_source if not (x in data_vecedit or x in data_oggize or x in data_merge)]

csvs = {}

extramergedeps = {"base/tank.dwh" : ["base/weapon_sparker.dwh"], "base/factions.dwh" : [x for x in data_copy if x.rsplit('/', 1)[0] == "base/faction_icons"]}

for item in data_merge:
  identifier = item.split('.')[-3].split('/')[-1]
  csvs[identifier] = env.Command("build/notes_%s.csv" % identifier, [programs["ods2csv"], "notes.ods"], "./${SOURCES[0]} $TARGET notes.ods %s" % identifier)

def make_datadir(dest, mergeflags = ""):
  for item in data_copy:
    env.Command(dest + "/" + item, "data_source/" + item, Copy("$TARGET", '$SOURCE'))
  
  for item in data_oggize:
    env.Command(dest + "/" + item.rsplit('.', 1)[0] + ".ogg", "data_source/" + item, "%s -q 6 -o $TARGET $SOURCE" % oggpath)
  
  for item in data_merge:
    identifier = item.split('.')[-3].split('/')[-1]
    destination = item.rsplit('.', 1)[0]
    if destination in extramergedeps:
      emgd = [dest + "/" + x for x in extramergedeps[item.rsplit('.', 1)[0]]]
    else:
      emgd = []
    env.Command(dest + "/" + item.rsplit('.', 1)[-2], [programs["merger"], csvs[identifier], "data_source/" + item] + emgd, "./${SOURCES[0]} ${SOURCES[1]} ${SOURCES[2]} $TARGET --fileroot=%s %s" % (dest, mergeflags))

make_datadir("data_release")
make_datadir("data_demo", "--demo")

# deploy directory and associated
def commandstrip(env, source):
  env.Command('deploy/%s' % source.split('/')[-1], source, "cp $SOURCE $TARGET && strip -s $TARGET")

for item in deploy_dlls:
  commandstrip(env, "/usr/mingw/local/bin/%s" % item)
commandstrip(env, "d-net.exe")
env.Command('deploy/license.txt', 'resources/license.txt', Copy("$TARGET", '$SOURCE'))

# installers

#env.Command('build/installer_demo.nsi', 'version_data', 

# version.cpp
env.Command('version.cpp', Split('version_data version_gen.py'), "./version_gen.py < version_data > $TARGET")
