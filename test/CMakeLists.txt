set(Boost_ADDITIONAL_VERSIONS "1.49" "1.49.0" "1.50" "1.50.0" "1.51"
                              "1.51.0" "1.52" "1.52.0" "1.53" "1.53.0")
find_package( Boost )
if (Boost_FOUND)
  add_subdirectory(gtest-1.6.0)
  include_directories(${PROJECT_SOURCE_DIR}/src)
  include_directories(${Boost_INCLUDE_DIRS})
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  add_executable(unit_test UnitTest.cc)
  target_link_libraries(unit_test gtest gtest_main)
  add_test(unit_test unit_test)
endif()

