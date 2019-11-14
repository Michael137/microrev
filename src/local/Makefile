include config.mk

SRC_DIR := $(ROOT_DIR)/src
INCL_DIR := $(ROOT_DIR)/include
BIN_DIR := $(ROOT_DIR)/bin
LIB_DIR := $(ROOT_DIR)/lib

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SOURCES := $(wildcard $(LIB_DIR)/*.cpp)
OBJS := $(patsubst $(LIB_DIR)/%.cpp,$(LIB_DIR)/%.o,$(LIB_SOURCES))
EXES := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.out,$(SOURCES))

LDFLAGS := -lpthread
CXXFLAGS += -g -std=c++17 -I$(INCL_DIR)

all: setup $(OBJS) $(EXES)

ifeq "$(UNAME)" "FreeBSD"
	@if [ -z "$(shell kldstat | grep hwpmc)" ]; then	\
		sudo kldload hwpmc;								\
	fi

	LDFLAGS += -lpmc
endif

.PHONY:
setup:
	mkdir -p $(SRC_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(INCL_DIR)
	mkdir -p $(LIB_DIR)

.PHONY:
clean:
	rm -f $(ROOT_DIR)/*.out
	rm -f $(LIB_DIR)/*.o
	rm -rf $(BIN_DIR)

$(LIB_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN_DIR)/%.out: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(OBJS)

.PHONY:
run: $(EXES)
	./$(BIN_DIR)/$(EXE).out
