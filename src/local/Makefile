include config.mk

SRC_DIR := $(ROOT_DIR)/src
TEST_DIR := $(ROOT_DIR)/test
INCL_DIR := $(ROOT_DIR)/include
BIN_DIR := $(ROOT_DIR)/bin
TEST_BIN_DIR := $(ROOT_DIR)/bin/test
LIB_DIR := $(ROOT_DIR)/lib

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
SOURCES += $(wildcard $(TEST_DIR)/*.cpp)
LIB_SOURCES := $(wildcard $(LIB_DIR)/*.cpp)
OBJS := $(patsubst $(LIB_DIR)/%.cpp,$(LIB_DIR)/%.o,$(LIB_SOURCES))
EXES := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.out,$(SOURCES))
TEST_EXES := $(patsubst $(TEST_DIR)/%.cpp,$(TEST_BIN_DIR)/%.out,$(SOURCES))

#DEPS := $(SOURCES:.c=.d)
#-include $(DEPS)
DEPFLAGS := -MD

ifeq "$(PC_TYPE)" "WITH_PAPI_LL"
  LDFLAGS := -Xlinker --no-as-needed
else
  LDFLAGS :=
endif
CXXFLAGS += -g -std=$(CXX_VERSION) -I$(INCL_DIR) -D$(PC_TYPE) -pthread

all: setup $(OBJS) $(EXES) $(TEST_EXES)

ifeq "$(UNAME)" "FreeBSD"
  ifeq "$(PC_TYPE)" "WITH_PMC"
	@if [ -z "$(shell kldstat | grep hwpmc)" ]; then	\
		sudo kldload hwpmc;								\
	fi

    LDFLAGS += -lpmc
  endif
else
  ifdef WITH_PFM
	export LD_LIBRARY_PATH=$(PFM_LIB_DIR)
    LDFLAGS += -lpfm
  else
    LDFLAGS += -L$(PAPI_DIR)/lib -lpapi
    CXXFLAGS += -I$(PAPI_DIR)/include

	# https://superuser.com/questions/980632/run-perf-without-root-rights
	# if PAPI is complaining about no available hwcntrs then run following command:
	#     sudo sysctl -w kernel.perf_event_paranoid=1
  endif
endif

.PHONY:
setup:
	mkdir -p $(SRC_DIR)
	mkdir -p $(TEST_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(TEST_BIN_DIR)
	mkdir -p $(INCL_DIR)
	mkdir -p $(LIB_DIR)

.PHONY:
clean:
	rm -f $(ROOT_DIR)/*.out
	rm -f $(LIB_DIR)/*.o
	rm -f $(ROOT_DIR)/*.d
	rm -f $(LIB_DIR)/*.d
	rm -f $(SRC_DIR)/*.d
	rm -rf $(BIN_DIR)
	rm -rf $(TEST_BIN_DIR)

$(LIB_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN_DIR)/%.out: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(OBJS)

$(TEST_BIN_DIR)/%.out: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(OBJS)

.PHONY: test
test: $(TEST_BIN_DIR)/*
	@for file in $^; do 	\
		sudo ./$${file} ;	\
	done

.PHONY:
run: $(EXES)
	./$(BIN_DIR)/$(EXE).out
