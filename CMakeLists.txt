cmake_minimum_required(VERSION 3.17)
project(musigrid)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(PkgConfig)
find_package(SDL2 CONFIG REQUIRED)
include(CheckCXXCompilerFlag)

pkg_check_modules(SFML sfml-all)
add_executable(musigrid
  main.cpp
  machine.hpp
  machine.cpp)


if (MSVC)
else()
  check_cxx_compiler_flag(-Wduplicated-cond WDUPLICATED_COND)
  check_cxx_compiler_flag(-Wduplicated-branches WDUPLICATED_BRANCHES)
  check_cxx_compiler_flag(WLOGICAL_OP  -Wlogical-op)
  check_cxx_compiler_flag(-Wnull-dereference WNULL_DEREFERENCE)

  target_compile_options(musigrid PRIVATE -Wall
    $<$<BOOL:WDUPLICATED_COND>:-Wduplicated-cond>
    $<$<BOOL:WDUPLICATED_BRANCHES>:-Wduplicated-branches>
    $<$<BOOL:WLOGICAL_OP>:-Wlogical-op>
    $<$<BOOL:WNULL_DEREFERENCE>:-Wnull-dereference>
  )
endif()

add_subdirectory(libtcod/src)

set_target_properties(musigrid PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
target_link_libraries(musigrid PRIVATE  LIBTCOD::LIBTCOD SDL2::SDL2 ${SFML_LIBRARIES} ${CMAKE_DL_LIBS})
target_include_directories(musigrid PRIVATE  ${SFML_INCLUDE_DIRS})

set(SANITIZER address)
if (NOT SANITIZER STREQUAL "")
  target_link_libraries(musigrid PRIVATE -fsanitize=${SANITIZER})
  target_compile_options(musigrid PRIVATE -fsanitize=${SANITIZER})
endif()