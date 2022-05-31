Import("env")

# access to global construction environment
# <SCons.Script.SConscript.SConsEnvironment>
# print(env.Dump())

def before_build_littlefs(source, target, env):
  env.Execute("yarn && yarn build --pioEnv=" + env["PIOENV"])

#
# Custom actions for specific files/objects
#
env.AddPreAction("$BUILD_DIR/littlefs.bin", before_build_littlefs)
