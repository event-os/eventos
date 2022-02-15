
defines = ['MAIN_DEF']
ccflags = []

env = Environment()
env.Append(CPPDEFINES=defines)
env.Append(CCCOMSTR="CC $SOURCES")
env.Append(CPPPATH=['app', 'meowc'])
env.Append(LINKCOMSTR="LINK $TARGET")

objs = SConscript('SConscript', variant_dir = 'build', duplicate = 0)
objs += SConscript('meowc/SConscript', variant_dir = 'build/eventos', duplicate = 0)
objs += SConscript('app/SConscript', variant_dir = 'build/app', duplicate = 0)
objs += SConscript('mdedug/SConscript', variant_dir = 'build/mdebug', duplicate = 0)
 
env.Program(target = 'eventos', source = objs)