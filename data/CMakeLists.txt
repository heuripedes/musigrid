
add_library(musigrid_data
  data.s
)

file(GLOB DATA_FILES ${CMAKE_SOURCE_DIR}/data/*.png)

set_source_files_properties(data.s PROPERTIES
  COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/data"
  OBJECT_DEPENDS ${DATA_FILES}
)

unset(DATA_FILES)
