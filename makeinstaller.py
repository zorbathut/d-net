#!/bin/python

import commands
import sys

directories = "data\n" + commands.getoutput("""cd %s ; find -type d -print | grep / | sed 's/.\//data\\\\/' | sed 's,/,\\\\,g'""" % sys.argv[1])
files = commands.getoutput("""cd %s ; find -type f -print | grep / | sed "s/.\///" | sed 's,/,\\\\,g'""" % sys.argv[1])

dfiles = commands.getoutput("""cd deploy ; find -type f -print | grep / | sed "s/.\///" | sed 's,/,\\\\,g'""")

install = ""
uninstall = ""

for line in directories.splitlines():
  install = install + 'CreateDirectory "$INSTDIR\\%s"\n' % line
  uninstall = 'RMDir "$INSTDIR\\%s"\n' % line + uninstall

for line in files.splitlines():
  install = install + 'File "/oname=data\\%s" "%s\\%s"\n' % (line, sys.argv[1], line)
  uninstall = 'Delete "$INSTDIR\\data\\%s"\n' % line + uninstall

install = install + 'File "/oname=settings" "settings.release"\n';
uninstall = 'Delete "$INSTDIR\\settings"\n' + uninstall;

for line in dfiles.splitlines():
  install = install + 'File "/oname=%s" "deploy\\%s"\n' % (line, line)
  uninstall = 'Delete "$INSTDIR\\%s"\n' % line + uninstall

for line in sys.stdin.readlines():
  line = line.strip()
  if line == "$$$INSTALL$$$":
    print install
  elif line == "$$$UNINSTALL$$$":
    print uninstall
  elif line == "$$$VERSION$$$":
    print '!define PRODUCT_VERSION "%s"' % open(sys.argv[2]).readline()
  else:
    print line

