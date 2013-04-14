set(Boost_ADDITIONAL_VERSIONS "1.49" "1.49.0" "1.50" "1.50.0" "1.51"
                              "1.51.0" "1.52" "1.52.0" "1.53" "1.53.0")
find_package( Boost )
if (Boost_FOUND)
  add_subdirectory(gtest-1.6.0)
  include_directories(${PROJECT_SOURCE_DIR}/src)
  include_directories(${PROJECT_SOURCE_DIR}/test)
  include_directories(${Boost_INCLUDE_DIRS})
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  add_executable(primary_construction_test 
    PrimaryConstructionTest.cpp 
    TestCommunicator.cpp)
  target_link_libraries(primary_construction_test gtest gtest_main)
  add_test(primary_construction_test primary_construction_test)
endif()

