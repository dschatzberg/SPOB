-include Makefile.in

VPATH=../src

LIBSPOB_SRCFILES := \
	spob/StateMachine.cpp

LIBSPOB_HDRFILES := \
	Spob.hpp

LIBSPOB_OBJFILES := $(LIBSPOB_SRCFILES:.cpp=.o)
LIBSPOB_DEPFILES := $(LIBSPOB_SRCFILES:.cpp=.d)

TEST_SRCFILES := \
	test/test.cpp

TEST_HDRFILES :=

TEST_OBJFILES := $(TEST_SRCFILES:.cpp=.o)
TEST_DEPFILES := $(TEST_SRCFILES:.cpp=.d)
TEST_LIBS := libspob.a

CXXFLAGS := $(CXXFLAGS) -iquote$(VPATH)

.PHONY: all clean

all: libspob.a testapp

libspob.a: $(LIBSPOB_HDRFILES) $(LIBSPOB_OBJFILES)
	ar cr $@ $(LIBSPOB_OBJFILES)

testapp: $(TEST_HDRFILES) $(TEST_OBJFILES) $(TEST_LIBS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJFILES) $(TEST_LIBS) -o $@

clean:
	-$(RM) $(wildcard $(LIBSPOB_OBJFILES) $(LIBSPOB_DEPFILES) \
	$(TEST_OBJFILES) $(TEST_DEPFILES) testapp libspob.a)

-include $(LIBSPOB_DEPFILES)
-include $(TEST_DEPFILES)

%.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
