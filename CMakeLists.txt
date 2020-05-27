cmake_minimum_required(VERSION 3.17)
project(musigrid)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(PkgConfig)
find_package(SDL2 CONFIG REQUIRED)

pkg_check_modules(SFML sfml-all)
add_executable(musigrid
  main.cpp)

add_subdirectory(libtcod/src)

set_target_properties(musigrid PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
target_link_libraries(musigrid PRIVATE  LIBTCOD::LIBTCOD SDL2::SDL2 ${SFML_LIBRARIES})
target_include_directories(musigrid PRIVATE  ${SFML_INCLUDE_DIRS})
