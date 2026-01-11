CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lreadline

# Build configuration
DEBUG ?= 1
ifeq ($(DEBUG),1)
    CFLAGS += -DDEBUG -O0
else
    CFLAGS += -O2
endif

# Installation paths
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

# Source directories
SRC_DIRS = src src/core src/builtins src/profiler src/utils
TEST_DIRS = tests/core tests/utils tests/builtins tests/profiler

# Find all source files
SRCS := $(wildcard src/*.c src/*/*.c)
TEST_SRCS = $(foreach dir,$(TEST_DIRS),$(wildcard $(dir)/*.c))

# Generate object file paths
OBJS := $(patsubst src/%.c,obj/%.o,$(SRCS))
TEST_OBJS = $(patsubst tests/%.c,obj/%.o,$(TEST_SRCS))

# Generate dependency files
DEPS = $(patsubst src/%.c,obj/%.d,$(SRCS))

# Target executable
TARGET = bin/qsh

# Object directories
OBJ_DIRS = $(sort $(dir $(OBJS)))

# Test executables
TEST_TARGETS = test_parser test_tokenizer test_profiler test_shell test_input test_variables

.PHONY: all clean install uninstall test docs $(TEST_TARGETS)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Create object directories and compile source files
$(OBJS): | $(OBJ_DIRS)

$(OBJ_DIRS):
	@mkdir -p $@

# Compile with dependency generation
obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include dependencies
-include $(DEPS)

clean:
	@rm -rf obj bin
	@echo "Cleaned build artifacts"

install: $(TARGET)
	@mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)
	@mkdir -p $(DESTDIR)$(MANDIR)
	install -m 644 doc/qsh.1 $(DESTDIR)$(MANDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/qsh
	rm -f $(DESTDIR)$(MANDIR)/qsh.1

# Test binaries
test_parser: tests/utils/test_parser.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_parser $^ $(LDFLAGS)

test_shell: tests/core/test_shell.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_shell $^ $(LDFLAGS)

test_profiler: tests/profiler/test_profiler.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_profiler $^ $(LDFLAGS)

test_input: tests/utils/test_input.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_input $^ $(LDFLAGS)

test_tokenizer: tests/utils/test_tokenizer.o obj/utils/tokenizer.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_tokenizer $^ $(LDFLAGS)

test_variables: tests/utils/test_variables.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_variables $^ $(LDFLAGS)

test_command_substitution: tests/utils/test_command_substitution.o $(filter-out obj/main.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_command_substitution $^ $(LDFLAGS)

# Test target
test: $(TEST_TARGETS) test_command_substitution
	@echo "Running all tests..."
	@echo "=== Running test_parser ===" && ./bin/test_parser || (echo "test_parser FAILED" && exit 1)
	@echo "=== Running test_tokenizer ===" && ./bin/test_tokenizer || (echo "test_tokenizer FAILED" && exit 1)
	@echo "=== Running test_input ===" && ./bin/test_input || (echo "test_input FAILED" && exit 1)
	@echo "=== Running test_shell ===" && ./bin/test_shell || (echo "test_shell FAILED" && exit 1)
	@echo "=== Running test_profiler ===" && ./bin/test_profiler || (echo "test_profiler FAILED" && exit 1)
	@echo "=== Running test_variables ===" && ./bin/test_variables || (echo "test_variables FAILED" && exit 1)
	@echo "=== Running test_command_substitution ===" && ./bin/test_command_substitution || (echo "test_command_substitution FAILED" && exit 1)
	@echo ""
	@echo "🎉 All tests passed! 🎉"

# Documentation target
docs:
	@echo "Generating documentation..."
	@python3 scripts/generate_docs.py
	@doxygen Doxyfile
	@echo "Documentation generated in docs/html/" 