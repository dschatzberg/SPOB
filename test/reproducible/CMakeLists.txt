find_package( Boost COMPONENTS context program_options)
check_cxx_compiler_flag(-std=c++0x HAS_CXX0X)
if ( HAS_CXX0X AND Boost_CONTEXT_FOUND AND Boost_PROGRAM_OPTIONS_FOUND
   AND (Boost_VERSION VERSION_GREATER "1053" OR Boost_VERSION
   VERSION_GREATER "1053")) 
   add_executable(reproducible-test test.cc Communicator.cc
      Coroutine.cc FailureDetector.cc Message.cc)
   set_source_files_properties(test.cc PROPERTIES COMPILE_FLAGS "-std=c++0x")
   set_source_files_properties(Communicator.cc PROPERTIES COMPILE_FLAGS "-std=c++0x")
   set_source_files_properties(Coroutine.cc PROPERTIES COMPILE_FLAGS "-std=c++0x")
   set_source_files_properties(FailureDetector.cc PROPERTIES COMPILE_FLAGS "-std=c++0x")
   set_source_files_properties(Message.cc PROPERTIES COMPILE_FLAGS "-std=c++0x")
   target_link_libraries (reproducible-test spob 
      ${Boost_CONTEXT_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${PROTOBUF_LIBRARY})
   include_directories("${CMAKE_CURRENT_LIST_DIR}")
   add_test(reproducible-test ./reproducible-test --np 20 --nm 100
      --p-prop .1 --p-fail .001 --seed 0)
endif()
