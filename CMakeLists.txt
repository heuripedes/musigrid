cmake_minimum_required(VERSION 3.17)
project(musigrid CXX ASM)

enable_testing()
add_subdirectory(googletest)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(PkgConfig)
find_package(SDL2 CONFIG REQUIRED)
include(CheckCXXCompilerFlag)

pkg_check_modules(SFML sfml-all)

set(SANITIZER "address")
add_subdirectory(tests)

add_library(musigrid-core
  machine.hpp
  machine.cpp
  terminal.cpp
  terminal.hpp
  data/data.s

  tsf.h
  stb_image.h
)

file(GLOB DATA_FILES ${CMAKE_SOURCE_DIR}/data/*.png)
set_source_files_properties(data/data.s PROPERTIES
  COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/data"
  OBJECT_DEPENDS ${DATA_FILES}
)
unset(DATA_FILES)

if (MSVC)
else()
  check_cxx_compiler_flag(-Wduplicated-cond WDUPLICATED_COND)
  check_cxx_compiler_flag(-Wduplicated-branches WDUPLICATED_BRANCHES)
  check_cxx_compiler_flag(WLOGICAL_OP  -Wlogical-op)
  check_cxx_compiler_flag(-Wnull-dereference WNULL_DEREFERENCE)

  target_compile_options(musigrid-core PRIVATE -Wall
    $<$<BOOL:WDUPLICATED_COND>:-Wduplicated-cond>
    $<$<BOOL:WDUPLICATED_BRANCHES>:-Wduplicated-branches>
    $<$<BOOL:WLOGICAL_OP>:-Wlogical-op>
    $<$<BOOL:WNULL_DEREFERENCE>:-Wnull-dereference>
  )
endif()

# add_subdirectory(libtcod/src)

set_target_properties(musigrid-core PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
target_link_libraries(musigrid-core PRIVATE
  )
# target_link_options(musigrid-core PUBLIC
#   -Wl,--format=binary -Wl,data/font_10x16.bmp -Wl,--format=default)


add_executable(musigrid
  main.cpp
)

set_target_properties(musigrid PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)

# target_link_libraries(musigrid PRIVATE musigrid-core  LIBTCOD::LIBTCOD SDL2::SDL2 ${SFML_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(musigrid PRIVATE musigrid-core SDL2::SDL2 ${CMAKE_DL_LIBS}
)
target_include_directories(musigrid PRIVATE  ${SFML_INCLUDE_DIRS})

if (NOT SANITIZER STREQUAL "")
  target_link_libraries(musigrid PRIVATE -fsanitize=${SANITIZER})
  target_compile_options(musigrid PRIVATE -fsanitize=${SANITIZER})

  target_link_libraries(musigrid-core PRIVATE -fsanitize=${SANITIZER})
  target_compile_options(musigrid-core PRIVATE -fsanitize=${SANITIZER})
endif()