
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

class DispatcherClass:
  def __init__(self, function, var, map):
    self.function = function
    self.var = var
    self.map = map
  
  def __call__(self, target, source, env):
    print self.var
    print self.map
    self.function(target, source, *self.var, **self.map)

def dispatcher(function, *var, **map):
  return DispatcherClass(function, var, map)
