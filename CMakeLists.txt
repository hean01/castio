#
#  This file is part of cast.io
#
#  Copyright 2014 Henrik Andersson <henrik.4e@gmail.com>
#
#  cast.io is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  cast.io is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with cast.io.  If not, see <http://www.gnu.org/licenses/>.
#
#

cmake_minimum_required(VERSION 2.6)

include(CheckIncludeFiles)
include(GNUInstallDirs)

project(castio C)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix")

include_directories("${CMAKE_BINARY_DIR}/")
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/src")

configure_file("src/config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(CASTIO_DATA_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME})

# Setup compiler
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long -pedantic -fPIC")
endif()

# Find required libraries
find_package(PkgConfig)

pkg_check_modules(LIBARCHIVE libarchive)
if (LIBARCHIVE_FOUND)
  link_directories(${LIBARCHIVE_LIBRARY_DIRS})
  include_directories(${LIBARCHIVE_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LIBARCHIVE_LIBRARIES})
else (LIBARCHIVE_FOUND)
  message(FATAL_ERROR "libarchive is required.")
endif (LIBARCHIVE_FOUND)

pkg_check_modules(LIBJS libjs)
if (LIBJS_FOUND)
  link_directories(${LIBJS_LIBRARY_DIRS})
  include_directories(${LIBJS_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LIBJS_LIBRARIES})
else (LIBJS_FOUND)
  message(FATAL_ERROR "JS is required, install js-devel.")
endif (LIBJS_FOUND)

pkg_check_modules(GLIB glib-2.0)
if (GLIB_FOUND)
  link_directories(${GLIB_LIBRARY_DIRS})
  include_directories(${GLIB_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${GLIB_LIBRARIES})
else (GLIB_FOUND)
  message(FATAL_ERROR "glib >= 2.0 is required, install glib-devel.")
endif (GLIB_FOUND)

pkg_check_modules(JSON json-glib-1.0)
if (JSON_FOUND)
  link_directories(${JSON_LIBRARY_DIRS})
  include_directories(${JSON_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${JSON_LIBRARIES})
else (JSON_FOUND)
  message(FATAL_ERROR "json-glib >= 1.0 is required, install json-glib-devel.")
endif (JSON_FOUND)

pkg_check_modules(SOUP libsoup-2.4)
if (SOUP_FOUND)
  link_directories(${SOUP_LIBRARY_DIRS})
  include_directories(${SOUP_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${SOUP_LIBRARIES})
else (SOUP_FOUND)
  message(FATAL_ERROR "libsoup >= 2.4 is required, install libsoup-devel.")
endif (SOUP_FOUND)

set(CASTIO_SOURCES
  src/providers/movie_library.c
  src/provider.c
  src/service.c
  src/settings.c
  src/main.c
)

#
# Build cast.io binary
#
add_executable(castio ${CASTIO_SOURCES})
target_link_libraries(castio ${LIBS})
install(TARGETS castio
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})

#
# Setup make dist target to create a tarball
#
include (InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/stats/;/tests/;/build/;/.git/;.gitignore;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_IGNORE_FILES ".*~;/#;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
include (CPack)
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
