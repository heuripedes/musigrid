cmake_minimum_required(VERSION 3.12)
project(musigrid VERSION 0.1 LANGUAGES CXX ASM)


set(SANITIZER "address")

enable_testing()
add_subdirectory(googletest)
add_subdirectory(tests)

add_subdirectory(data)
add_subdirectory(core)

if (MUSIGRID_LIBRETRO)
  message(STATUS "Will compile musigrid_libretro")

  if (BUILD_SHARED_LIBS)
    message(WARNING "musigrid_core will use position independent code (potentially slower)")
    set_property(TARGET musigrid_core PROPERTY POSITION_INDEPENDENT_CODE 1)
  endif()

  add_library(musigrid_libretro
    libretro/libretro.cpp
    libretro/libretro.h
  )

  target_compile_options(musigrid_libretro PRIVATE -Wall -Wpedantic)
  target_link_libraries(musigrid_libretro PRIVATE musigrid_core)
endif()

if(MUSIGRID_SDL)
  message(STATUS "Will compile musigrid_sdl")
  find_package(SDL2 CONFIG REQUIRED)

  add_executable(musigrid_sdl
    sdl/main.cpp
  )

  set_target_properties(musigrid_sdl PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
  target_link_libraries(musigrid_sdl PRIVATE
    musigrid_core
    SDL2::SDL2
    ${CMAKE_DL_LIBS}
  )

endif()
