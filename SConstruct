import os
import sys

# 添加路径到环境变量
sys.path.append("tools")

from tools import test_copy

test_copy.execute()

defines = ['MAIN_DEF']
ccflags = []

env = Environment()
env.Append(CPPDEFINES = defines)
env.Append(CCCOMSTR = "CC $SOURCES")
env.Append(LINKCOMSTR = "LINK $TARGET")

# The unit test example --------------------------------------------------------
objs = SConscript('test/SConscript', variant_dir = 'build/test', duplicate = 0)
objs += SConscript('eventos/SConscript', variant_dir = 'build/eventos', duplicate = 0)
objs += SConscript('3rd/unity/SConscript', variant_dir = 'build/3rd/unity', duplicate = 0)

env.Program(target = 'eos', source = objs)

# The posix example ------------------------------------------------------------
objs = SConscript('examples/posix/SConscript', variant_dir = 'build/examples/posix', duplicate = 0)
objs += SConscript('eventos/SConscript', variant_dir = 'build/eventos', duplicate = 0)

env.Program(target = 'posix', source = objs)