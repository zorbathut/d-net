
import os
import subprocess

from SCons.Environment import Environment

def Conf():

  # Set up our environment
  env = Environment(LINKFLAGS = "-g -O2 -Wl,--as-needed -Wl,-\\(".split(" "), CXXFLAGS="-Wall -Wno-sign-compare -Wno-uninitialized -g -O2".split(" "), CPPDEFINES="DPRINTF_MARKUP".split(" "))

  # Here's our custom tests
  def CheckFile(context, paths, file):
    context.Message("Checking for %s ... " % file)
    for path in paths:
      testpath = path + "/" + file;
      if os.path.exists(testpath):
        context.Result(testpath)
        return testpath
    context.Result(0)
    return 0

  def Execute(context, command):
    context.Message("Caching return value of %s ... " % command)
    run = subprocess.Popen(command.split(" "), stdout=subprocess.PIPE)
    rv = run.communicate()[0].strip()
    context.Result(rv)
    return rv

  # Now let's actually configure it
  conf = env.Configure(custom_tests = {'CheckFile' : CheckFile, 'Execute' : Execute})

  if 1: # Cygwin
    # Set up our environment defaults
    env.Append(CPPPATH = "/usr/mingw/local/include".split(" "), LIBPATH = "/usr/mingw/local/lib".split(" "), CCFLAGS="-mno-cygwin".split(" "), CPPFLAGS="-mno-cygwin".split(" "), CXXFLAGS="-mno-cygwin".split(" "), LINKFLAGS="-mno-cygwin".split(" "))
    
    # Boost flags
    boostlibs=["boost_regex-mgw-mt", "boost_filesystem-mgw-mt"]
    boostpath="/usr/mingw/local/include/boost-1_33_1"
    
    # VECTOR_PARANOIA
    env.Append(CPPDEFINES="VECTOR_PARANOIA")
    
    if not conf.CheckCXXHeader('windows.h', '<>'):
      Exit(1)
    
    if not conf.CheckLibWithHeader("ws2_32", "windows.h", "c++", "WSACleanup();", autoadd=0):
      Exit(1)
    env.Append(LIBS_GAME=["ws2_32"])
    
    if not conf.CheckLibWithHeader("opengl32", "GL/gl.h", "c++", "glLoadIdentity();", autoadd=0):
      Exit(1)
    env.Append(LIBS_GAME=["opengl32"])
    
    if not conf.CheckLib("mingw32", autoadd=0):
      Exit(1)
    env.Append(LIBS="mingw32")

  # SDL
  sdlpath = conf.CheckFile(["/usr/mingw/local/bin", "/usr/local/bin", "/usr/bin"], "sdl-config")
  if not sdlpath:
    Exit(1)
  env.Append(LINKFLAGS_GAME=conf.Execute(sdlpath + " --libs"), CPPFLAGS_GAME=conf.Execute(sdlpath + " --cflags"))

  # Boost
  env.Append(LIBS=boostlibs, CPPPATH=boostpath)
  if not conf.CheckCXXHeader('boost/noncopyable.hpp', '<>'):
    Exit(1)
  for lib in boostlibs:
    if not conf.CheckLib(lib, autoadd=0):
      Exit(1)

  # libm
  if not conf.CheckLib("m", autoadd=0):
    Exit(1)
  env.Append(LIBS="m")

  # zlib
  if not conf.CheckLib("z", "compress", autoadd=0):
    Exit(1)

  # libpng
  if not conf.CheckLib("png", "png_create_read_struct", autoadd=0):
    Exit(1)

  # curl
  # curl depends on zlib and ws2_32 on Windows
  templibs = env['LIBS']
  env.Append(LIBS=["z", "ws2_32"])
  env.Append(CPPDEFINES="CURL_STATICLIB")
  if not conf.CheckLib("curl", "curl_easy_init", autoadd=0):
    Exit(1)
  env['LIBS'] = templibs

  # xerces
  if not conf.CheckLib("xerces-c", autoadd=0):
    Exit(1)

  # Check for libogg
  if not conf.CheckLib("ogg", "ogg_stream_init", autoadd=0):
    Exit(1)
    
  # Check for libvorbis
  if not conf.CheckLib("vorbis", "vorbis_info_init", autoadd=0):
    Exit(1)
    
  # Check for libvorbisfile
  if not conf.CheckLibWithHeader("vorbisfile", "vorbis/vorbisfile.h", "c++", 'ov_fopen("hi", NULL);', autoadd=0):
    Exit(1)


  # Check for wx
  wxpath = conf.CheckFile(["/usr/mingw/local/bin", "/usr/local/bin", "/usr/bin"], "wx-config")
  if not wxpath:
    Exit(1)

  env = conf.Finish()
  
  return env
