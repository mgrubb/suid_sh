cmake_minimum_required (VERSION 2.8.11)
project (suidsh)

#if(DEBUG)
#add_definitions(-DDEBUG)
#endif(DEBUG)

#add_definitions(-DCONFIG_LOCATION="${CONFIG_LOCATION}")
add_executable(suidsh suidsh.c)

install(TARGETS suidsh
  RUNTIME DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID SETGID)

