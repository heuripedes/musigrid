
add_executable(operators operators.cpp)

set(SANITIZER "")
if (NOT SANITIZER STREQUAL "")
  target_link_libraries(operators PRIVATE -fsanitize=${SANITIZER})
  target_compile_options(operators PRIVATE -fsanitize=${SANITIZER})
endif()

set_target_properties(operators PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
target_link_libraries(operators PRIVATE musigrid-core gtest_main)
add_test(NAME operators COMMAND operators)
