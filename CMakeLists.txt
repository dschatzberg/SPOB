cmake_minimum_required (VERSION 2.8.7)
project (Spob)

set (Spob_VERSION_MAJOR 0)
set (Spob_VERSION_MINOR 1)

set (CMAKE_CXX_FLAGS_DEBUG "")
set (CMAKE_CXX_FLAGS_RELEASE "")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
set (CMAKE_CXX_FLAGS_MINSIZEREL "")
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wall HAS_WALL)
if (HAS_WALL)
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
check_cxx_compiler_flag(-Werror HAS_WERROR)
if (HAS_WERROR)
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()
check_cxx_compiler_flag(-O0 HAS_O0)
if (HAS_O0)
   set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
endif()
check_cxx_compiler_flag(-g3 HAS_G3)
if (HAS_G3)
   set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")
   set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g3")
endif()
check_cxx_compiler_flag(-pg HAS_PG)
if (HAS_PG)
   set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
   set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -pg")
endif()
check_cxx_compiler_flag(-O2 HAS_O2)
if (HAS_O2)
   set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2")
endif()
check_cxx_compiler_flag(-O3 HAS_O3)
if (HAS_O3)
   set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()
check_cxx_compiler_flag(-Os HAS_OS)
if (HAS_OS)
   set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
endif()
check_cxx_compiler_flag(-fprofile-arcs HAS_FPROFILE_ARCS)
if (HAS_FPROFILE_ARCS)
   set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs")
   set (CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fprofile-arcs") 
endif()
check_cxx_compiler_flag(-ftest-coverage HAS_FTEST_COVERAGE)
if (HAS_FTEST_COVERAGE)
   set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ftest-coverage")
   set (CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ftest-coverage")
endif()


set(Boost_ADDITIONAL_VERSIONS "1.49" "1.49.0" "1.50" "1.50.0" "1.51"
                              "1.51.0" "1.52" "1.52.0" "1.53" "1.53.0")
find_package( Boost REQUIRED )
include_directories(${Boost_INCLUDE_DIRS})

find_package( Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

configure_file (
  "${PROJECT_SOURCE_DIR}/src/SpobConfig.h.in"
  "${PROJECT_BINARY_DIR}/SpobConfig.h"
  )

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS 
                                 src/Ack.proto
                                 src/AckRecover.proto
                                 src/AckTree.proto 
                                 src/Commit.proto 
                                 src/ConstructTree.proto 
                                 src/Entry.proto
                                 src/NakTree.proto 
                                 src/Propose.proto 
                                 src/Reconnect.proto 
                                 src/ReconnectResponse.proto
                                 src/RecoverCommit.proto 
                                 src/RecoverPropose.proto 
                                 src/RecoverReconnect.proto)

add_library(spob src/StateMachine.cc ${PROTO_SRCS} ${PROTO_HDRS})

include(CTest)

add_subdirectory(test)