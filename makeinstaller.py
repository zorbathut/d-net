
from __future__ import with_statement

import commands
import sys

def generateInstaller(target, source, copyprefix, files, deployfiles, finaltarget, mainexe, version):

  directories = {"data" : None}
  
  for item in [x.split('/', 1)[1] for x in files]:
    for steps in range(len(item.split('/')) - 1):
      directories["data/" + item.rsplit('/', steps + 1)[0]] = None
  
  directories = [x.replace('/', '\\') for x in directories.iterkeys()]
  files = [x.replace('/', '\\') for x in files]
  deployfiles = [x.replace('/', '\\') for x in deployfiles]
  
  mainexe = str(mainexe).replace('/', '\\')
  
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

