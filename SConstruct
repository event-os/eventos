
defines = ['MAIN_DEF']
ccflags = []

env = Environment()
env.Append(CPPDEFINES = defines)
env.Append(CCCOMSTR = "CC $SOURCES")
env.Append(LINKCOMSTR = "LINK $TARGET")

# The unit test example --------------------------------------------------------
objs = SConscript('examples/unittest/SConscript', variant_dir = 'build/examples/unittest', duplicate = 0)
objs += SConscript('eventos/SConscript', variant_dir = 'build/eventos', duplicate = 0)
objs += SConscript('3rd/mdebug/SConscript', variant_dir = 'build/3rd/mdebug', duplicate = 0)

env.Program(target = 'eventos', source = objs)

# The unit test example --------------------------------------------------------
objs = SConscript('examples/test/SConscript', variant_dir = 'build/examples/test', duplicate = 0)

env.Program(target = 'hello', source = objs)
