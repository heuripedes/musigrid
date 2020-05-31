
add_executable(operators operators.cpp)

set_target_properties(operators PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
target_link_libraries(operators PRIVATE musigrid_core musigrid_data gtest_main)
add_test(NAME operators COMMAND operators)
