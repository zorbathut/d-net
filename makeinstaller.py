
import commands
import sys

def generateInstaller(target, source, copyprefix, files, deployfiles):

  directories = {"data" : None}
  
  for item in [x.split('/', 1)[1] for x in files]:
    for steps in range(len(item.split('/')) - 1):
      directories["data/" + item.rsplit('/', steps + 1)[0]] = None
  
  print directories
  directores = [x for (x, y) in directories.iterkeys()]
  print directories
  
  sadlikgj

  install = ""
  uninstall = ""

  for line in directories.splitlines():
    install = install + 'CreateDirectory "$INSTDIR\\%s"\n' % line
    uninstall = 'RMDir "$INSTDIR\\%s"\n' % line + uninstall

  for line in files.splitlines():
    install = install + 'File "/oname=data\\%s" "%s\\%s"\n' % (line, sys.argv[1], line)
    uninstall = 'Delete "$INSTDIR\\data\\%s"\n' % line + uninstall

  install = install + 'File "/oname=settings" "settings.%s"\n' % sys.argv[1].split("_")[1]
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
    elif line == "$$$TYPE$$$":
      print '!define PRODUCT_TYPE "%s"' % sys.argv[1].split("_")[1]
    else:
      print line

