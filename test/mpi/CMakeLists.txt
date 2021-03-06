find_package( Boost COMPONENTS mpi serialization program_options timer system chrono)
find_package(MPI)
find_package (Threads)
if (Boost_MPI_FOUND AND Threads_FOUND AND Boost_PROGRAM_OPTIONS_FOUND AND
    Boost_TIMER_FOUND AND Boost_SERIALIZATION_FOUND AND Boost_SYSTEM_FOUND AND
    Boost_CHRONO_FOUND AND MPI_FOUND)
   include_directories(${MPI_INCLUDE_PATH})
   add_executable(mpi-latency-test LatencyTest.cpp Communicator.cpp)
   add_executable(mpi-throughput-test ThroughputTest.cpp Communicator.cpp)
   target_link_libraries (mpi-latency-test spob ${Boost_MPI_LIBRARY}
      ${MPI_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} 
      ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} 
      ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
   target_link_libraries (mpi-throughput-test spob ${Boost_MPI_LIBRARY}
      ${MPI_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} 
      ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} 
      ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
   if (MPI_COMPILE_FLAGS)
     set_target_properties(mpi-latency-test PROPERTIES COMPILE_FLAGS
                                    "${MPI_COMPILE_FLAGS}")
     set_target_properties(mpi-throughput-test PROPERTIES COMPILE_FLAGS
                                    "${MPI_COMPILE_FLAGS}")
   endif()
   if (MPI_LINK_FLAGS)
     set_target_properties(mpi-latency-test PROPERTIES LINK_FLAGS
                                    "${MPI_LINK_FLAGS}")
     set_target_properties(mpi-throughput-test PROPERTIES LINK_FLAGS
                                    "${MPI_LINK_FLAGS}")
   endif()
   include_directories("${CMAKE_CURRENT_LIST_DIR}")
   # add_test(mpi-latency-test mpirun -np 5 mpi-latency-test 5)
endif ()
