cmake_minimum_required (VERSION 2.8.11)
project (suidsh)
set (SUIDSH_VERSION_MAJOR "1" CACHE INTERNAL "Package major version")
set (SUIDSH_VERSION_MINOR "0" CACHE INTERNAL "Package minor version")
set (SUIDSH_VERSION_PATCH "3" CACHE INTERNAL "Package patch version")
set (CPACK_PACKAGE_VERSION_MAJOR "${SUIDSH_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${SUIDSH_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${SUIDSH_VERSION_PATCH}")
set (CPACK_GENERATOR TGZ)
set (CPACK_SOURCE_GENERATOR ${CPACK_GENERATOR})
set (CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set (CPACK_SOURCE_IGNORE_FILES "/build/" "/.git/" ".swp$" ".#" CACHE INTERNAL "CPACK will ignore these files")
include (CPack)

include (CheckIncludeFiles)

check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(stdarg.h HAVE_STDARG_H)
check_include_files(errno.h HAVE_ERRNO_H)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h ESCAPE_QUOTES)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

OPTION(DEBUG "Enable DEBUG output" OFF)
SET(CONFIG_LOCATION "/etc/suidsh.conf" CACHE STRING "Location of the configuration file")

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
add_custom_target(bindist COMMAND ${CMAKE_MAKE_PROGRAM} package_binary)
add_subdirectory(src)
