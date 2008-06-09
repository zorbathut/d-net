#!/usr/bin/python

import sys

print """

#include "version.h"

extern const string dnet_version = "%s";

""" % sys.stdin.readline()
