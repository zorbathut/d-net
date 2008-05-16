
from __future__ import with_statement

from SConstruct_config import Conf
from util import traverse, dispatcher
from makeinstaller import generateInstaller
import sys

# Globals
Decider('MD5-timestamp')
SetOption('implicit_cache', 1)

env, categories, flagtypes, oggpath, platform, makensis = Conf()

# List of buildables
buildables = [
  ["d-net", "GAME", Split("main core game timer debug gfx collide gamemap util rng args interface metagame itemdb itemdb_adjust itemdb_parse parse dvec2 input level coord ai inputsnag os float cfcommon coord_boolean player metagame_config shop shop_demo shop_info game_ai game_effects color metagame_tween cfc game_tank game_util game_projectile socket httpd recorder generators audio itemdb_httpd test regex shop_layout perfbar adler32 money stream stream_file itemdb_stream res_interface settings dumper audit stream_gz dumper_registry debug_911_on"), [], Split("resource")],
  ["vecedit", "EDITOR", Split("vecedit vecedit_main debug os util gfx float coord parse color dvec2 cfcommon input itemdb itemdb_adjust itemdb_parse args regex rng adler32 money perfbar timer stream stream_file itemdb_stream image dumper_registry debug_911_off")],
  ["reporter", "REPORTER", Split("reporter_main debug os util parse args debug_911_off os_ui")],
  ["merger", "CONSOLE_MERGER", Split("merger debug os util parse itemdb itemdb_adjust itemdb_parse args color dvec2 float coord cfcommon merger_weapon merger_bombardment merger_tanks merger_glory merger_util merger_upgrades merger_factions regex rng adler32 money stream stream_file itemdb_stream dumper_registry debug_911_off")],
  ["ods2csv", "CONSOLE_ODS2CSV", Split("ods2csv debug os util parse adler32 args debug_911_off"), Split("minizip/unzip minizip/ioapi")]
]

def addReleaseVersion(buildables, suffix):
  tversion = [buildables[0][0] + "-" + suffix] + buildables[0][1:]
  tversion[2] = tversion[2] + ["version_" + suffix]
  buildables += [tversion]

addReleaseVersion(buildables, "demo")
addReleaseVersion(buildables, "release")

buildables[0][2] += ["version_local"]

deploy_dlls = Split("SDL.dll libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll")

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
      built[(build[1], item)] = env.Object("build/%s.%s.o" % (item, abbreviation), "%s.cpp" % item, **params);
    objects += built[(build[1], item)]
  
  if len(build) > 3:
    for item in build[3]:
      if not (build[1], item) in built:
        built[(build[1], item)] = env.Object("build/%s.%s.o" % (item, abbreviation), "%s.c" % item, **params)
      objects += built[(build[1], item)]
      
  if len(build) > 4:
    for item in build[4]:
      if not (build[1], item) in built:
        built[(build[1], item)] = env.Command("build/%s.%s.res" % (item, abbreviation), "%s.rc" % item, "nice windres $SOURCE -O coff -o $TARGET")
      objects += built[(build[1], item)]
  
  programs[build[0]] = env.Program("build/" + build[0], objects, **params)[0]

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
  results = []
  
  for item in data_copy:
    results += env.Command(dest + "/" + item, "data_source/" + item, Copy("$TARGET", '$SOURCE'))
  
  for item in data_oggize:
    results += env.Command(dest + "/" + item.rsplit('.', 1)[0] + ".ogg", "data_source/" + item, "%s -q 6 -o $TARGET $SOURCE" % oggpath)
  
  for item in data_merge:
    identifier = item.split('.')[-3].split('/')[-1]
    destination = item.rsplit('.', 1)[0]
    if destination in extramergedeps:
      emgd = [dest + "/" + x for x in extramergedeps[item.rsplit('.', 1)[0]]]
    else:
      emgd = []
    results += env.Command(dest + "/" + item.rsplit('.', 1)[-2], [programs["merger"], csvs[identifier], "data_source/" + item] + emgd, "./${SOURCES[0]} ${SOURCES[1]} ${SOURCES[2]} $TARGET --fileroot=%s %s" % (dest, mergeflags))
  
  return results

data_dests = {}
data_dests["release"] = make_datadir("data_release")
data_dests["demo"] = make_datadir("data_demo", "--demo")

def make_shopcache(dest):
  return env.Command("data_" + dest + "/shopcache.dwh", [programs["d-net"]] + data_dests[dest], "./${SOURCES[0]} --generateCachedShops=0.99 --fileroot=data_%s" % dest)

shopcaches = {}
shopcaches["release"] = make_shopcache("release")
shopcaches["demo"] = make_shopcache("demo")


# deploy directory and associated
def commandstrip(env, source):
  return env.Command('deploy/%s' % str(source).split('/')[-1], source, "cp $SOURCE $TARGET && strip -s $TARGET")[0]

programs_stripped = {}
for key, value in programs.items():
  programs_stripped[key] = commandstrip(env, value)

deployfiles = []

for item in deploy_dlls:
  deployfiles += [commandstrip(env, "/usr/mingw/local/bin/%s" % item)]
deployfiles += [programs_stripped["reporter"]]
deployfiles += env.Command('deploy/license.txt', 'resources/license.txt', Copy("$TARGET", '$SOURCE'))


# installers
with open("version_data") as f:
  version = f.readline()

def create_installer(type, shopcaches):
  if shopcaches == []:
    quick = "-quick"
  else:
    quick = ""
  
  nsipath = 'build/installer_%s%s.nsi' % (type, quick)
  ident = '%s-%s' % (version, type)
  finalpath = 'build/dnet-%s-%s%s.exe' % (version, type, quick)
  mainexe = programs_stripped["d-net-" + type]
  
  env.Command(nsipath, ['installer.nsi.template', 'makeinstaller.py'] + data_dests[type] + deployfiles + shopcaches + [mainexe], dispatcher(generateInstaller, copyprefix=type, files=[str(x) for x in data_dests[type] + shopcaches], deployfiles=[str(x) for x in deployfiles], finaltarget=finalpath, mainexe=mainexe, version=ident)) # Technically it only depends on those files existing, not their actual contents.
  return env.Command(finalpath, [nsipath] + data_dests[type] + deployfiles + shopcaches + [mainexe], "%s - < ${SOURCES[0]}" % makensis)

Alias("packagedemoquick", create_installer("demo", []))
Alias("packagedemo", create_installer("demo", shopcaches["demo"]))
Alias("packagereleasequick", create_installer("release", []))
Alias("package", Alias("packagerelease", create_installer("release", shopcaches["release"])))


# version_*.cpp
def addVersionFile(type):
  env.Command('version_%s.cpp' % type, Split('version_data version_gen.py'), """( cat version_data ; echo -n "-%s" ) | ./version_gen.py > $TARGET""" % type)

for item in "local demo release".split():
  addVersionFile(item)


# cleanup
env.Clean("build", "build")
env.Clean("data_release", "data_release")
env.Clean("data_demo", "data_demo")
env.Clean("deploy", "deploy")


# bugfix
env.Dir("/usr/mingw/local/include/boost-1_33_1/boost/iterator")


# How we actually do stuff
def command(env, name, deps, line):
  env.AlwaysBuild(env.Alias(name, deps, line))
  
fulldata = env.Alias("d-net program and release data", data_dests["release"] + [programs["d-net"]])
if not env.GetOption('clean'):
  env.Default(fulldata) # if we clean, we want to clean everything

localflags = "--writetarget=dumps/dump --nofullscreen --noaudio --perfbar --httpd_port=616 --runTests"
stdrun = localflags + " --debugitems --startingPhase=8 --debugControllers=2 --factionMode=3 --nullControllers=11 --writeTarget= --auto_newgame --nocullShopTree --httpd_port=616 --checksumgamestate --noshopcache --nofastForwardOnNoCache"

command(env, "run", fulldata, "./%s %s" % (programs["d-net"], stdrun))
command(env, "runclean", fulldata, "./%s %s" % (programs["d-net"], localflags))

command(env, "ai", fulldata, "./%s %s --aiCount=16 --fastForwardTo=100000000 --factionMode=3 --nullcontrollers=19 --allowAisQuit --treatAiAsHuman --noshopcache" % (programs["d-net"], localflags))
command(env, "ailoop", fulldata, "while nice ./%s %s --aiCount=12 --fastForwardTo=100000000 --factionmode=3 --terminateAfter=3600 --startingPhase=0 --allowAisQuit --treatAiAsHuman --nullcontrollers=19 --noshopcache ; do echo Cycle. ; done" % (programs["d-net"], localflags))

aicslflags = localflags + " --fastForwardTo=100000000 --noshopcache --treatAiAsHuman --randomizeFrameRender=60"
command(env, "aicsl", fulldata, "while nice rm -f dumps/dump-*.dnd && nice ./%s %s --factionmode=3 --allowAisQuit --startingPhase=7 --aiCount=6 --terminateAfter=300 --nullcontrollers=5 --treataiashuman && sleep 2s && nice ./d-net.exe %s --readtarget=`ls dumps/dump-*.dnd` --writetarget= && sleep 2s ; do echo Cycle. ; done" % (programs["d-net"], aicslflags, aicslflags))

command(env, "vecedit", [programs["vecedit"]], "./%s" % (programs["vecedit"]))
