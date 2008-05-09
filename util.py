
import dircache

def traverse(path, prefix=""):
  if path[-1] != '/':
    path = path + '/'
  
  list = dircache.listdir(path)
  dircache.annotate(path, list)
  olist = []
  for item in list:
    if item == '.svn/':
      continue
    if item[-1] == '/':
      olist += traverse(path + item, prefix + item)
    else:
      olist += [prefix + item]
  return olist
