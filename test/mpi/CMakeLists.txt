find_package( Boost COMPONENTS mpi serialization)
find_package(MPI)
if ( Boost_MPI_FOUND AND Boost_SERIALIZATION_FOUND AND MPI_FOUND)
   include_directories(${MPI_INCLUDE_PATH})
   add_executable(mpi-test test.cc)
   target_link_libraries (mpi-test spob ${Boost_MPI_LIBRARY}
      ${MPI_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY} ${PROTOBUF_LIBRARY})
   if (MPI_COMPILE_FLAGS)
     set_target_properties(mpi-test PROPERTIES COMPILE_FLAGS
                                    "${MPI_COMPILE_FLAGS}")
   endif()
   if (MPI_LINK_FLAGS)
     set_target_properties(mpi-test PROPERTIES LINK_FLAGS
                                    "${MPI_LINK_FLAGS}")
   endif()
   add_test(mpi-test mpirun -np 5 mpi-test 5)
endif ()
