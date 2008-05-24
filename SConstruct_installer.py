
import os
import commands

from SCons.Environment import Environment
from SCons.Util import Split

from util import dispatcher

from makeinstaller import generateInstaller

def MakeDeployables(env, commandstrip):
  deploy_dlls = Split("SDL.dll libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll")
  
  deployfiles = []
  for item in deploy_dlls:
    deployfiles += [commandstrip(env, "/usr/mingw/local/bin/%s" % item)]

  return []

def MakeInstaller(env, type, shopcaches, version, binaries, data, deployables, installers):
  if shopcaches == []:
    quick = "-quick"
  else:
    quick = ""
  
  nsipath = 'build/installer_%s%s.nsi' % (type, quick)
  ident = '%s-%s' % (version, type)
  finalpath = 'build/dnet-%s-%s%s.exe' % (version, type, quick)
  mainexe = binaries["d-net-" + type]
  
  nsirv = env.Command(nsipath, ['installer.nsi.template', 'makeinstaller.py'] + data[type] + deployables + shopcaches + [mainexe], dispatcher(generateInstaller, copyprefix=type, files=[str(x) for x in data[type] + shopcaches], deployfiles=[str(x) for x in deployables], finaltarget=finalpath, mainexe=mainexe, version=ident)) # Technically it only depends on those files existing, not their actual contents.
  return env.Command(finalpath, nsirv + data[type] + deployables + shopcaches + [mainexe], "%s - < ${SOURCES[0]}" % installers)

