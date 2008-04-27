#!/bin/python

import sys

print """

#include "version.h"

const string dnet_version = "%s";

""" % sys.stdin.readline()
