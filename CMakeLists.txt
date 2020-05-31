cmake_minimum_required(VERSION 3.12)
project(musigrid VERSION 0.1 LANGUAGES CXX ASM)

enable_testing()
add_subdirectory(googletest)


set(SANITIZER "address")
# add_subdirectory(tests)
add_subdirectory(data)
add_subdirectory(core)


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

  if (MSVC)
    if (NOT SANITIZER STREQUAL "")
      target_link_libraries(musigrid_sdl PRIVATE /fsanitize=${SANITIZER})
      target_compile_options(musigrid_sdl PRIVATE /fsanitize=${SANITIZER})
    endif()
  else()
    target_compile_options(musigrid_sdl PRIVATE -Wall -Wpedantic)

    if (NOT SANITIZER STREQUAL "")
      target_compile_options(musigrid_sdl PRIVATE -fno-omit-frame-pointer -fno-common)
      target_link_libraries(musigrid_sdl PRIVATE -fno-omit-frame-pointer -fno-common)

      target_link_libraries(musigrid_sdl PUBLIC -fsanitize=${SANITIZER})
      target_compile_options(musigrid_sdl PUBLIC -fsanitize=${SANITIZER})
    endif()
  endif()
endif()
