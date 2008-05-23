
import os
import commands

from SCons.Environment import Environment
from SCons.Util import Split

from makeinstaller import generateInstaller

def MakeInstaller(env, type, shopcaches):
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

