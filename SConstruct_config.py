
import os
import commands

from SCons.Environment import Environment
from SCons.Util import Split

def Conf():

  # Set up our environment
  env = Environment(LINKFLAGS = Split("-g -O2 -Wl,--as-needed -Wl,-\\("), CXXFLAGS=Split("-Wall -Wno-sign-compare -Wno-uninitialized -g -O2"), CPPDEFINES=["DPRINTF_MARKUP"], CXX="nice ./ewrap $TARGET g++")
  
  categories = Split("GAME EDITOR REPORTER CONSOLE CONSOLE_MERGER CONSOLE_ODS2CSV")
  flagtypes = Split("CCFLAGS CPPFLAGS CXXFLAGS LINKFLAGS LIBS CPPPATH LIBPATH CPPDEFINES")
  
  for flag in flagtypes:
    env.Append(**{flag : []})
    for item in categories:
      env[flag + "_" + item] = []
  
  # If we're cleaning, we don't need all this.
  if not env.GetOption('clean'):
    
    # Here's our custom tests
    def CheckFile(context, paths, file):
      context.Message("Checking for %s ... " % file)
      for path in paths:
        testpath = path + "/" + file;
        if os.path.exists(testpath):
          context.Result(testpath)
          return '"%s"' % testpath
      context.Result(0)
      return 0

    def Execute(context, command):
      context.Message("Caching return value of %s ... " % command)
      rv = commands.getoutput(command)
      context.Result(rv)
      return rv

    # Now let's actually configure it
    conf = env.Configure(custom_tests = {'CheckFile' : CheckFile, 'Execute' : Execute})

    if 1: # Cygwin
      # Set up our environment defaults
      env.Append(CPPPATH = Split("/usr/mingw/local/include"), LIBPATH = Split("/usr/mingw/local/lib"), CCFLAGS=Split("-mno-cygwin"), CPPFLAGS=Split("-mno-cygwin"), CXXFLAGS=Split("-mno-cygwin"), LINKFLAGS=Split("-mno-cygwin"))
      
      # Boost flags
      boostlibs=["boost_regex-mgw-mt", "boost_filesystem-mgw-mt"]
      boostpath=["/usr/mingw/local/include/boost-1_33_1"]
      
      # VECTOR_PARANOIA
      env.Append(CPPDEFINES=["VECTOR_PARANOIA"])
      
      if not conf.CheckCXXHeader('windows.h', '<>'):
        Exit(1)
      
      if not conf.CheckLibWithHeader("ws2_32", "windows.h", "c++", "WSACleanup();", autoadd=0):
        Exit(1)
      env.Append(LIBS_GAME="ws2_32")
      env.Append(LIBS_REPORTER="ws2_32")
      
      if not conf.CheckLibWithHeader("opengl32", "GL/gl.h", "c++", "glLoadIdentity();", autoadd=0):
        Exit(1)
      env.Append(LIBS_GAME="opengl32")
      env.Append(LIBS_EDITOR="opengl32")
      
      if not conf.CheckLib("mingw32", autoadd=0):
        Exit(1)
      env.Append(LIBS="mingw32")

    # SDL
    sdlpath = conf.CheckFile(["/usr/mingw/local/bin", "/usr/local/bin", "/usr/bin"], "sdl-config")
    if not sdlpath:
      Exit(1)
    env.Append(LINKFLAGS_GAME=Split(conf.Execute(sdlpath + " --libs")), CPPFLAGS_GAME=Split(conf.Execute(sdlpath + " --cflags")))
    env.Append(CPPDEFINES_EDITOR=["NOSDL"])

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
    env.Append(LIBS_GAME="z")
    env.Append(LIBS_EDITOR="z")
    env.Append(LIBS_REPORTER="z")
    env.Append(LIBS_CONSOLE_ODS2CSV="z")

    # libpng
    if not conf.CheckLib("png", "png_create_read_struct", autoadd=0):
      Exit(1)
    env.Append(LIBS_EDITOR="png")

    # curl
    # curl depends on zlib and ws2_32 on Windows
    templibs = env['LIBS']
    env.Append(LIBS=["z", "ws2_32"])
    if not conf.CheckLib("curl", "curl_easy_init", autoadd=0):
      Exit(1)
    env['LIBS'] = templibs
    env.Append(LIBS_REPORTER="curl")
    env.Append(CPPDEFINES_REPORTER="CURL_STATICLIB")

    # xerces
    if not conf.CheckLib("xerces-c", autoadd=0):
      Exit(1)
    env.Append(LIBS_CONSOLE_ODS2CSV="xerces-c")

    # Check for libogg
    if not conf.CheckLib("ogg", "ogg_stream_init", autoadd=0):
      Exit(1)
    env.Append(LIBS_GAME="ogg")
    
    # Check for libvorbis
    if not conf.CheckLib("vorbis", "vorbis_info_init", autoadd=0):
      Exit(1)
    env.Append(LIBS_GAME="vorbis")
    
    # Check for libvorbisfile
    if not conf.CheckLibWithHeader("vorbisfile", "vorbis/vorbisfile.h", "c++", 'ov_fopen("hi", NULL);', autoadd=0):
      Exit(1)
    env.Append(LIBS_GAME="vorbisfile")

    # Check for wx
    wxpath = conf.CheckFile(["/usr/mingw/local/bin", "/usr/local/bin", "/usr/bin"], "wx-config")
    if not wxpath:
      Exit(1)
    env.Append(LINKFLAGS_EDITOR=Split(conf.Execute(wxpath + " --libs --gl-libs")), CPPFLAGS_EDITOR=Split(conf.Execute(wxpath + " --cxxflags")))

    # Check for oggenc
    oggpath = conf.CheckFile(["/cygdrive/c/windows/util"], "oggenc")
    if not oggpath:
      Exit(1)
      
    # Check for makensis
    makensis = conf.CheckFile(["/cygdrive/c/Program Files (x86)/NSIS"], "makensis")
    if not makensis:
      Exit(1)
    
    env.Append(CXXFLAGS_REPORTER="-O0", LINKFLAGS_REPORTER="-O0")

    env = conf.Finish()
    
  else:
    
    oggpath = ""
    makensis = ""
  
  return env, categories, flagtypes, oggpath, makensis
