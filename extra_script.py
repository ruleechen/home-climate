Import("env")

# access to global construction environment
print(env)

def before_build_littlefs(source, target, env):
  env.Execute("yarn && yarn build")

#
# Custom actions for specific files/objects
#
env.AddPreAction("$BUILD_DIR/littlefs.bin", before_build_littlefs)
