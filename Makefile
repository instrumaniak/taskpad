CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc
DEBUG_FLAGS = -g -O0 -DDEBUG
LDFLAGS = -lyaml-cpp

PREFIX ?= $(HOME)/.local

SRCDIR = src
BUILDDIR = build
TARGET = taskpad

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))

# Test files
TESTDIR = tests
TEST_SRCS = $(wildcard $(TESTDIR)/test_*.cpp)
TEST_OBJS = $(patsubst $(TESTDIR)/%.cpp, $(BUILDDIR)/%.o, $(TEST_SRCS))
TEST_TARGET = $(BUILDDIR)/test_taskpad

# Skill data directory (for install-skills command)
TASKPAD_DATA_DIR ?= .agents

.PHONY: all clean install install-bin install-skills-data test debug uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DTASKPAD_DATA_DIR=\"$(TASKPAD_DATA_DIR)\" -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean all

$(BUILDDIR)/test_main.o: $(TESTDIR)/test_main.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/test_%.o: $(TESTDIR)/test_%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TEST_TARGET): $(TEST_OBJS) $(filter-out $(BUILDDIR)/main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

install: install-bin install-skills-data

install-bin: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

install-skills-data:
	install -d $(DESTDIR)$(PREFIX)/share/taskpad/skills
	cp -r .agents/skills/taskpad $(DESTDIR)$(PREFIX)/share/taskpad/skills/

uninstall:
	rm -f $(HOME)/.local/bin/$(TARGET)
	rm -rf $(HOME)/.local/share/taskpad
	rm -rf $(HOME)/.agents/skills/taskpad
