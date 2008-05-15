#!/bin/python

from __future__ import with_statement

import psycopg2
import os
import gzip
import StringIO

conn = psycopg2.connect("user=crashreader password=crashreader host=crashlog.cams.local dbname=crashlog")
print conn

curs = conn.cursor()
print curs

curs.execute("SELECT timestamp, exesize, file, line, version, dump FROM crashes")

rows = curs.fetchall()
print rows

try:
  os.mkdir("crashes")
except OSError:
  pass


values = {}
for item in rows:
  timestamp, exesize, file, line, version, dump = item
  prefix = "crashes/crash-%s-%s.%d" % (version, file, line)
  if not prefix in values:
    values[prefix] = 0
  #with open("%s-%d.txt.gz" % (prefix, values[prefix]), 'w') as f:
    #f.write(str(dump))
  with open("%s-%d.txt" % (prefix, values[prefix]), 'w') as f:
    f.write(gzip.GzipFile(None, 'rb', 9, StringIO.StringIO(dump)).read())
  values[prefix] = values[prefix] + 1
