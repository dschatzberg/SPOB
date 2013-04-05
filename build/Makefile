-include Makefile.in

VPATH=../src

LIBSPOB_SRCFILES := \
	spob/Ack.pb.cc \
	spob/AckRecover.pb.cc \
	spob/AckTree.pb.cc \
	spob/Commit.pb.cc \
	spob/ConstructTree.pb.cc \
	spob/Entry.pb.cc \
	spob/NakTree.pb.cc \
	spob/Propose.pb.cc \
	spob/RecoverCommit.pb.cc \
	spob/RecoverPropose.pb.cc \
	spob/RecoverReconnect.pb.cc \
	spob/StateMachine.cpp

LIBSPOB_HDRFILES := \
	Spob.hpp \
	spob/Ack.pb.h \
	spob/AckRecover.pb.h \
	spob/AckTree.pb.h \
	spob/Commit.pb.h \
	spob/ConstructTree.pb.h \
	spob/Entry.pb.h \
	spob/NakTree.pb.h \
	spob/Propose.pb.h \
	spob/RecoverCommit.pb.h \
	spob/RecoverPropose.pb.h \
	spob/RecoverReconnect.pb.h

LIBSPOB_OBJFILES := $(patsubst %.cc, %.o, \
	$(patsubst %.cpp, %.o, $(LIBSPOB_SRCFILES)))
LIBSPOB_DEPFILES := $(patsubst %.cc, %.d, \
	$(patsubst %.cpp, %.d, $(LIBSPOB_SRCFILES)))

TEST_SRCFILES := \
	test/test.cpp

TEST_HDRFILES :=

TEST_OBJFILES := $(patsubst %.cc, %.o, \
	$(patsubst %.cpp, %.o, $(TEST_SRCFILES))) libspob.a
TEST_DEPFILES := $(patsubst %.cc, %.d, \
	$(patsubst %.cpp, %.d, $(TEST_SRCFILES)))

TEST_LIBS := -lboost_mpi -lboost_serialization -lprotobuf

REPRODUCIBLE_TEST_SRCFILES := \
	test/reproducible/Communicator.cpp \
	test/reproducible/Coroutine.cpp \
	test/reproducible/FailureDetector.cpp \
	test/reproducible/Message.cpp \
	test/reproducible/test.cpp

REPRODUCIBLE_TEST_HDRFILES := \
	test/reproducible/Communicator.hpp \
	test/reproducible/Coroutine.hpp \
	test/reproducible/FailuredDetector.hpp \
	test/reproducible/Message.hpp

REPRODUCIBLE_TEST_OBJFILES := $(patsubst %.cc, %.o, \
	$(patsubst %.cpp, %.o, $(REPRODUCIBLE_TEST_SRCFILES))) libspob.a
REPRODUCIBLE_TEST_DEPFILES := $(patsubst %.cc, %.d, \
	$(patsubst %.cpp, %.d, $(REPRODUCIBLE_TEST_SRCFILES)))

REPRODUCIBLE_TEST_LIBS := -lboost_context -lprotobuf

MPICXX := mpicxx

.PHONY: all clean

all: libspob.a testapp

libspob.a: $(LIBSPOB_HDRFILES) $(LIBSPOB_OBJFILES)
	ar cr $@ $(LIBSPOB_OBJFILES)

testapp: $(TEST_HDRFILES) $(TEST_OBJFILES)
	$(MPICXX) $(CXXFLAGS) $(TEST_OBJFILES) $(TEST_LIBS) -o $@

reproducible_test: $(REPRODUCIBLE_TEST_HDFFILES) $(REPRODUCIBLE_TEST_OBJFILES)
	$(CXX) $(CXXFLAGS) $(REPRODUCIBLE_TEST_OBJFILES) \
	$(REPRODUCIBLE_TEST_LIBS) -o $@

clean:
	-$(RM) $(wildcard $(LIBSPOB_OBJFILES) $(LIBSPOB_DEPFILES) \
	$(TEST_OBJFILES) $(TEST_DEPFILES) testapp libspob.a) \
	$(filter %.pb.cc, $(LIBSPOB_SRCFILES)) \
	$(filter %.pb.h, $(LIBSPOB_HDRFILES)) \
	$(filter %.pb.cc, $(TEST_SRCFILES)) \
	$(filter %.pb.h, $(TEST_HDRFILES))

-include $(LIBSPOB_DEPFILES)
-include $(TEST_DEPFILES)
-include $(REPRODUCIBLE_TEST_DEPFILES)

test/test.o: test/test.cpp Makefile
	mkdir -p $(dir $@)
	$(MPICXX) $(CXXFLAGS) -iquote$(VPATH) -iquote. -MMD -MP -c $< -o $@

%.pb.o: %.pb.cc Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -iquote. -MMD -MP -c $< -o $@

%.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -iquote$(VPATH) -iquote. -MMD -MP -c $< -o $@


%.pb.h %.pb.cc: %.proto Makefile
	mkdir -p $(dir $@)
	protoc --cpp_out=. -I$(VPATH) $<
