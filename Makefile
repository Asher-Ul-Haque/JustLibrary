# - - - Build Configuration - - -
# Options : release, debug
MODE          ?= debug
# Options : both, static, dynamic
LIB_TYPE      ?= both

# - - - Directory Structure - - -
INCLUDE_DIR   := include
SRC_DIR       := src
TEST_DIR      := tests
EXAMPLES_DIR  := examples
BIN_DIR       := bin/$(MODE)
BUILD_DIR     := .build/$(MODE)

# - - - Toolchain & Flags - - -
CC            := clang
AR            := ar
ARFLAGS       := rcs
COMMON_FLAGS  := -std=c11 -Wall -Wextra -Werror -Wpedantic -fPIC -fvisibility=hidden
DEBUG_FLAGS   := -O0 -g -fno-omit-frame-pointer
RELEASE_FLAGS := -O3

CPPFLAGS      := -I$(INCLUDE_DIR) -MMD -MP
LDFLAGS       := -L$(BIN_DIR) -Wl,-rpath,'$$ORIGIN/..' -Wl,-rpath,$(abspath $(BIN_DIR)) -ljust

ifeq ($(MODE),debug)
  CFLAGS      := $(COMMON_FLAGS) $(DEBUG_FLAGS)
  CPPFLAGS    += -DDEBUG
  $(info Build mode : DEBUG)
  $(info Flags      : $(CFLAGS))
else ifeq ($(MODE),release)
  CFLAGS      := $(COMMON_FLAGS) $(RELEASE_FLAGS)
  CPPFLAGS    += -DNDEBUG
  $(info Build mode : RELEASE)
  $(info Flags      : $(CFLAGS))
else
  $(error Unknown build mode '$(MODE)'. Must be 'debug' or 'release')
endif
$(info )

# - - - Source & Object Resolution - - -
LIB_SRCS      := $(shell find $(SRC_DIR) -type f -name '*.c')
LIB_OBJS      := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/$(SRC_DIR)/%.o, $(LIB_SRCS))

SHARED_LIB    := $(BIN_DIR)/libjust.so
STATIC_LIB    := $(BIN_DIR)/libjust.a

# - - - Set active library target depending on LIB_TYPE
ifeq ($(LIB_TYPE),shared)
  LIB_TARGETS := $(SHARED_LIB)
else ifeq ($(LIB_TYPE),static)
  LIB_TARGETS := $(STATIC_LIB)
else ifeq ($(LIB_TYPE),both)
  LIB_TARGETS := $(SHARED_LIB) $(STATIC_LIB)
else
  $(error Unknown library type '$(LIB_TYPE)'. Must be 'shared', 'static', or 'both')
endif

# - - - Optional discovery for examples and tests directories
TEST_SRCS     := $(shell find $(TEST_DIR) -type f -name '*.c' 2>/dev/null)
TEST_OBJS     := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/$(TEST_DIR)/%.o, $(TEST_SRCS))
TEST_BINS     := $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/tests/%, $(TEST_SRCS))

EXAMPLE_SRCS  := $(shell find $(EXAMPLES_DIR) -type f -name '*.c' 2>/dev/null)
EXAMPLE_OBJS  := $(patsubst $(EXAMPLES_DIR)/%.c, $(BUILD_DIR)/$(EXAMPLES_DIR)/%.o, $(EXAMPLE_SRCS))
EXAMPLE_BINS  := $(patsubst $(EXAMPLES_DIR)/%.c, $(BIN_DIR)/examples/%, $(EXAMPLE_SRCS))

ALL_OBJS      := $(LIB_OBJS) $(TEST_OBJS) $(EXAMPLE_OBJS)
HEADER_DEPS   := $(ALL_OBJS:.o=.d)


# - - - Primary Targets - - -
.PHONY: all libs shared static examples tests runTests clean remake help

all: libs examples tests

libs: $(LIB_TARGETS)

shared: $(SHARED_LIB)

static: $(STATIC_LIB)

examples: $(LIB_TARGETS) $(EXAMPLE_BINS)

tests: $(LIB_TARGETS) $(TEST_BINS)

runTests: tests
	@if [ -z "$(TEST_BINS)" ]; then \
		echo "No test binaries found in $(BIN_DIR)/tests/"; \
	else \
		for t in $(TEST_BINS); do \
			echo ""; \
			echo "Running $$t..."; \
			./$$t || exit 1; \
		done; \
		echo ""; \
		echo "All tests executed successfully."; \
	fi

clean:
	@rm -rf bin .build
	@echo "Cleaned all build artifacts."

remake: clean all


# - - - Library Linking Rules - - -

# - - - Export JUST_API symbols when compiling library objects
$(LIB_OBJS): CPPFLAGS += -DJUST_EXPORT

$(SHARED_LIB): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@$(CC) -shared $(CFLAGS) -o $@ $^ -lm -pthread
	@echo ""
	@echo "[SHARED LIB] : $@"
	@for obj in $(LIB_OBJS); do \
		echo "  └── $$obj"; \
	done
	@echo ""

$(STATIC_LIB): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@$(AR) $(ARFLAGS) $@ $^
	@echo ""
	@echo "[STATIC LIB] : $@"
	@for obj in $(LIB_OBJS); do \
		echo "  └── $$obj"; \
	done
	@echo ""


# - - - Binaries Linking Rules - - -

$(BIN_DIR)/examples/%: $(BUILD_DIR)/$(EXAMPLES_DIR)/%.o $(LIB_TARGETS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
	@echo "[EXAMPLE]    : $@"

$(BIN_DIR)/tests/%: $(BUILD_DIR)/$(TEST_DIR)/%.o $(LIB_TARGETS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
	@echo "[TEST]       : $@"


# - - - Object Compilation Rules - - -

$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC (LIB)] $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC (TST)] $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(EXAMPLES_DIR)/%.o: $(EXAMPLES_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC (EXP)] $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


# - - - Dependency Tracking - - -
-include $(HEADER_DEPS)

# - - - Help Target - - -
help:
	@echo "JustLibrary Build System"
	@echo "Usage: make [target] [MODE=debug|release] [LIB_TYPE=both|shared|static]"
	@echo ""
	@echo "Targets:"
	@echo "  all        Builds libraries, examples, and tests (default)"
	@echo "  libs       Builds selected library target(s)"
	@echo "  shared     Builds libjust.so only"
	@echo "  static     Builds libjust.a only"
	@echo "  examples   Builds all executable examples"
	@echo "  tests      Builds all unit test binaries"
	@echo "  runTests   Builds and executes all unit tests"
	@echo "  clean      Deletes bin/ and .build/ folders"
	@echo "  remake     Cleans and rebuilds everything from scratch"
