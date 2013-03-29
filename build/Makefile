-include Makefile.in

VPATH=../src

LIBSPOB_SRCFILES := \
	spob/AckTree.pb.cc \
	spob/ConstructTree.pb.cc \
	spob/Entry.pb.cc \
	spob/RecoverPropose.pb.cc \
	spob/StateMachine.cpp

LIBSPOB_HDRFILES := \
	Spob.hpp \
	spob/AckTree.pb.h \
	spob/ConstructTree.pb.h \
	spob/Entry.pb.h \
	spob/RecoverPropose.pb.h

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


MPICXX := mpicxx

CXXFLAGS := $(CXXFLAGS)

.PHONY: all clean

all: libspob.a testapp

libspob.a: $(LIBSPOB_HDRFILES) $(LIBSPOB_OBJFILES)
	ar cr $@ $(LIBSPOB_OBJFILES)

testapp: $(TEST_HDRFILES) $(TEST_OBJFILES)
	$(MPICXX) $(CXXFLAGS) $(TEST_OBJFILES) $(TEST_LIBS) -o $@

clean:
	-$(RM) $(wildcard $(LIBSPOB_OBJFILES) $(LIBSPOB_DEPFILES) \
	$(TEST_OBJFILES) $(TEST_DEPFILES) testapp libspob.a) \
	$(filter %.pb.cc, $(LIBSPOB_SRCFILES)) \
	$(filter %.pb.h, $(LIBSPOB_HDRFILES)) \
	$(filter %.pb.cc, $(TEST_SRCFILES)) \
	$(filter %.pb.h, $(TEST_HDRFILES))

-include $(LIBSPOB_DEPFILES)
-include $(TEST_DEPFILES)

%.pb.o: %.pb.cc Makefile
	mkdir -p $(dir $@)
	$(MPICXX) $(CXXFLAGS) -iquote. -MMD -MP -c $< -o $@

%.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(MPICXX) $(CXXFLAGS) -iquote$(VPATH) -iquote. -MMD -MP -c $< -o $@


%.pb.h %.pb.cc: %.proto Makefile
	mkdir -p $(dir $@)
	protoc --cpp_out=. -I$(VPATH) $<
