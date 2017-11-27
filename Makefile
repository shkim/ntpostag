# Simple makefile to build on Unix-like OSes (see memo.txt)
# Windows users should use Visual Studio 2017

OUTDIR = out
SRCDIR = ntpostag
TESTDIR = tests

CPP_FILES = character paragraph conjugate sentence \
	token worddic makedic util restapi

TEST_FILES = $(addprefix test_, main conjugate split_sentence syllable)

COMMON_OBJ_FILES = $(addprefix $(OUTDIR)/, $(addsuffix .o, $(CPP_FILES)))
MAIN_OBJ_FILES = $(addprefix $(OUTDIR)/, $(addsuffix .o, main))
PYMOD_OBJ_FILES = $(addprefix $(OUTDIR)/, $(addsuffix .o, bpython))
TEST_OBJ_FILES += $(addprefix $(OUTDIR)/, $(addsuffix .o, $(TEST_FILES)))

CXXFLAGS = -std=c++14 -Wno-multichar
LDFLAGS = -Wall
LDLIBS = -lboost_system -lboost_filesystem -lboost_chrono -lssl -lcrypto
# -lcpprest 

OS := $(shell uname)
ifeq ($(OS),Darwin)
	BOOST_ROOT=/usr/local/opt/boost@1.59
	OPENSSL_ROOT=/usr/local/opt/openssl
	CPPREST_ROOT=casablanca
	CPPREST_BINDIR=$(CPPREST_ROOT)/build.debug/Binaries
	PYTHON_HOME=/usr/local/Cellar/python3/3.6.3/Frameworks/Python.framework/Versions/3.6
	PYTHON_VERSION=3.6m

	CXXFLAGS += -I$(BOOST_ROOT)/include -I$(OPENSSL_ROOT)/include \
		-I$(CPPREST_ROOT)/Release/include \
		-I$(PYTHON_HOME)/include/python$(PYTHON_VERSION)
	LDFLAGS += -L$(BOOST_ROOT)/lib -L$(OPENSSL_ROOT)/lib \
		-L$(CPPREST_BINDIR) -rpath $(CPPREST_BINDIR)
	LDLIBS += -lboost_thread-mt
else ifeq ($(OS),Linux)
	BOOST_PYTHON_SUFFIX=-py35
	PYTHON_VERSION=3.5m
	LDLIBS += -pthread -lm 
endif


ifeq ($(BUILD_TEST),1)
	ifeq ($(OS),Darwin)
		TEST_CXXFLAGS += -Iextern/gtest/src/googletest/googletest/include
		LDFLAGS += -Lextern/gtest/src/googletest-build/googlemock
	else ifeq ($(OS),Linux)
		TEST_CXXFLAGS += -Iout/extern/gtest/src/googletest/googletest/include
		LDFLAGS += -Lout/extern/gtest/src/googletest-build/googlemock
	endif
TEST_CXXFLAGS += -D_UNITTEST -Intpostag 
LDLIBS += -lgmock
endif

ifeq ($(BUILD_PYMOD),1)
	ifeq ($(OS),Darwin)
		LDFLAGS += -L/usr/local/opt/boost-python@1.59/lib -L$(PYTHON_HOME)/lib
		LDLIBS += -lboost_python-mt -dynamiclib
	else ifeq ($(OS),Linux)
		CXXFLAGS += -fPIC
		LDFLAGS += -L/usr/lib/x86_64-linux-gnu -shared -Wl,--export-dynamic 
		LDLIBS += -lboost_python$(BOOST_PYTHON_SUFFIX) 
	endif
LDLIBS += -lpython$(PYTHON_VERSION) 
endif


TARGET = $(OUTDIR)/ntpostag
TEST_TARGET = $(OUTDIR)/test_ntpostag
PYMOD_TARGET = $(OUTDIR)/ntpostag.so


$(TARGET): $(OUTDIR) $(COMMON_OBJ_FILES) $(MAIN_OBJ_FILES)
	$(CXX) -o $@ $(LDFLAGS) $(COMMON_OBJ_FILES) $(MAIN_OBJ_FILES) $(LDLIBS)

$(TEST_TARGET):	$(OUTDIR) $(COMMON_OBJ_FILES) $(TEST_OBJ_FILES)
	$(CXX) -o $@ $(LDFLAGS) $(COMMON_OBJ_FILES) $(TEST_OBJ_FILES) $(LDLIBS)

$(PYMOD_TARGET): $(OUTDIR) $(COMMON_OBJ_FILES) $(PYMOD_OBJ_FILES)
	$(CXX) -o $@ $(LDFLAGS) $(COMMON_OBJ_FILES) $(PYMOD_OBJ_FILES) $(LDLIBS) 


$(OUTDIR)/%.o:	$(SRCDIR)/%.cpp
	$(CXX) -o $@ -c $(CXXFLAGS) $<

$(OUTDIR)/bpython.o:	$(SRCDIR)/bpython.cpp
	$(CXX) -o $@ -c $(CXXFLAGS) -I/usr/include/python$(PYTHON_VERSION) $<

$(OUTDIR)/%.o:	$(TESTDIR)/%.cpp
	$(CXX) -o $@ -c $(CXXFLAGS) $(TEST_CXXFLAGS) $<


test:
	BUILD_TEST=1 make $(TEST_TARGET)


pymod:
	BUILD_PYMOD=1 make $(PYMOD_TARGET)


$(OUTDIR):
	mkdir -p $@

clean:
	rm -f $(OUTDIR)/*.o $(TARGET) $(TEST_TARGET) $(PYMOD_TARGET)

