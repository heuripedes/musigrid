
add_library(musigrid_core OBJECT
  machine.hpp
  machine.cpp
  terminal.cpp
  terminal.hpp
  tsf.h
  stb_image.h
)

set_target_properties(musigrid_core PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS OFF
)

target_link_libraries(musigrid_core PUBLIC musigrid_data)

if (MSVC)
  if (NOT SANITIZER STREQUAL "")
    target_link_libraries(musigrid_core PRIVATE /fsanitize=${SANITIZER})
    target_compile_options(musigrid_core PRIVATE /fsanitize=${SANITIZER})
  endif()
else()
  target_compile_options(musigrid_core PRIVATE -Wall -Wpedantic)

  if (NOT SANITIZER STREQUAL "")
    target_compile_options(musigrid_core PRIVATE -fno-omit-frame-pointer -fno-common)
    target_link_libraries(musigrid_core PRIVATE -fno-omit-frame-pointer -fno-common)

    target_link_libraries(musigrid_core PUBLIC -fsanitize=${SANITIZER})
    target_compile_options(musigrid_core PUBLIC -fsanitize=${SANITIZER})
  endif()
endif()
