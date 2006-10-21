from pylab import *
import re
colors = "bgrcmyk"
pos = 0
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
      key = key + [float(item)]
      value = value + [float(store)]
      store = None
  if(store != None):
    print "ERROR"
    fail
  plot(key, value, colors[pos] + "-", label = name)
  plot(key, value, colors[pos] + "o", label = "")
  pos = (pos + 1) % len(colors)
legend(loc='lower right')
loglog()
xlabel('CPS')
ylabel('DPS')
show()
