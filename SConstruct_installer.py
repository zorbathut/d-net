
from __future__ import with_statement

import os
import commands
import sys

from SCons.Environment import Environment
from SCons.Util import Split

from util import dispatcher

def Installers(platform):
  
  if platform == "win":
    def MakeDeployables(env, commandstrip):
      deploy_dlls = Split("SDL.dll libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll")
      
      deployfiles = []
      for item in deploy_dlls:
        deployfiles += [commandstrip(env, "/usr/mingw/local/bin/%s" % item)]

      return []

    def generateInstaller(target, source, copyprefix, files, deployfiles, finaltarget, mainexe, vecedit, version):

      directories = {"data" : None}
      
      for item in [x.split('/', 1)[1] for x in files]:
        for steps in range(len(item.split('/')) - 1):
          directories["data/" + item.rsplit('/', steps + 1)[0]] = None
      
      directories = [x.replace('/', '\\') for x in directories.iterkeys()]
      files = [x.replace('/', '\\') for x in files]
      deployfiles = [x.replace('/', '\\') for x in deployfiles]
      
      mainexe = str(mainexe).replace('/', '\\')
      vecedit = str(vecedit).replace('/', '\\')
      
      install = ""
      uninstall = ""

      for line in directories:
        install = install + 'CreateDirectory "$INSTDIR\\%s"\n' % line
        uninstall = 'RMDir "$INSTDIR\\%s"\n' % line + uninstall

      for line in files:
        install = install + 'File "/oname=data\\%s" "%s"\n' % (line.split('\\', 1)[1], line)
        uninstall = 'Delete "$INSTDIR\\data\\%s"\n' % line.split('\\', 1)[1] + uninstall

      install = install + 'File "/oname=settings" "settings.%s"\n' % copyprefix
      uninstall = 'Delete "$INSTDIR\\settings"\n' + uninstall;

      for line in deployfiles:
        install = install + 'File "/oname=%s" "%s"\n' % (line.split('\\', 1)[1], line)
        uninstall = 'Delete "$INSTDIR\\%s"\n' % line.split('\\', 1)[1] + uninstall
      
      install = install + 'File "/oname=d-net.exe" "%s"\n' % mainexe
      uninstall = 'Delete "$INSTDIR\\d-net.exe"\n' + uninstall
      
      install = install + 'File "/oname=vecedit.exe" "%s"\n' % vecedit
      uninstall = 'Delete "$INSTDIR\\vecedit.exe"\n' + uninstall

      with open(str(source[0])) as inp:
        with open(str(target[0]), "w") as otp:
          for line in inp.readlines():
            line = line.strip()
            if line == "$$$INSTALL$$$":
              print >> otp, install
            elif line == "$$$UNINSTALL$$$":
              print >> otp, uninstall
            elif line == "$$$VERSION$$$":
              print >> otp, '!define PRODUCT_VERSION "%s"' % version
            elif line == "$$$TYPE$$$":
              print >> otp, '!define PRODUCT_TYPE "%s"' % copyprefix
            elif line == "$$$OUTFILE$$$":
              print >> otp, 'OutFile "%s"' % finaltarget
            else:
              print >> otp, line

    def MakeInstaller(env, type, shopcaches, version, binaries, data, deployables, installers, suffix):
      nsipath = 'build/installer_%s.nsi' % (suffix)
      ident = '%s-%s' % (version, suffix)
      finalpath = 'build/dnet-%s-%s.exe' % (version, suffix)
      mainexe = binaries["d-net-" + type]
      vecedit = binaries["vecedit-" + type]
      
      deps = data[type] + deployables + shopcaches + [mainexe] + [vecedit]
      
      nsirv = env.Command(nsipath, ['installer.nsi.template', 'SConstruct_installer.py'] + deps, dispatcher(generateInstaller, copyprefix=type, files=[str(x) for x in data[type] + shopcaches], deployfiles=[str(x) for x in deployables], finaltarget=finalpath, mainexe=mainexe, vecedit=vecedit, version=ident)) # Technically it only depends on those files existing, not their actual contents.
      return env.Command(finalpath, nsirv + deps, "%s - < ${SOURCES[0]}" % installers)
    
    return MakeDeployables, MakeInstaller
  elif platform == "linux":
    def MakeDeployables(env, commandstrip):
      return []

    def MakeInstaller(env, type, shopcaches, version, binaries, data, deployables, installers, suffix):
      nsipath = 'build/installer_%s.nsi' % (suffix)
      ident = '%s-%s' % (version, suffix)
      finalpath = 'build/dnet-%s-%s.exe' % (version, suffix)
      mainexe = binaries["d-net-" + type]
      
      nsirv = env.Command(nsipath, ['installer.nsi.template', 'SConstruct_installer.py'] + data[type] + deployables + shopcaches + [mainexe], dispatcher(generateInstaller, copyprefix=type, files=[str(x) for x in data[type] + shopcaches], deployfiles=[str(x) for x in deployables], finaltarget=finalpath, mainexe=mainexe, version=ident)) # Technically it only depends on those files existing, not their actual contents.
      return env.Command(finalpath, nsirv + data[type] + deployables + shopcaches + [mainexe], "%s - < ${SOURCES[0]}" % installers)
    
    return MakeDeployables, MakeInstaller
  else:
    lol

