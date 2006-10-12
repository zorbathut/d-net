from pylab import *
import re
for line in open("tools/weapondump.dat"):
  splits = line.rsplit(",")
  name = splits[0]
  store = None
  key = []
  value = []
  for item in splits[1:]:
    if(store == None):
      store = item
    else:
      key = key + [float(store)]
      value = value + [float(item)]
      store = None
  if(store != None):
    print "ERROR"
    fail
  plot(key, value, label = name)
legend()
loglog()
axis(xmin=1, ymin=1)
xlabel('DPS')
ylabel('CPS')
show()
