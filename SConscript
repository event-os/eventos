src = Glob('main_unittest.c')

paths = ['.', 'meowc']

defines = ['main_function']
ccflags = []

env = Environment()
env.Append(CPPDEFINES = defines)
env.Append(CCCOMSTR = "CC $SOURCES")
env.Append(CPPPATH=paths)

obj = env.Object(src)
 
Return('obj')