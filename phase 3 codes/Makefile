# ══════════════════════════════════════════════════════════════
# Makefile for Smart Public Complaint Analyzer
# C (Engine Layer) + C++ (Console Interface) + engine_cli (Web bridge)
#
#   mingw32-make            build the console app and engine_cli
#   mingw32-make run        build and start the console app
#   mingw32-make web        build engine_cli and start the website
#   mingw32-make test       build and run the engine tests
#   mingw32-make eval       measure the NLP against the hand-labelled templates
#   mingw32-make stress     check that parallel writes lose nothing (on a copy of data/)
#   mingw32-make clean      remove build output (keeps data/)
#   mingw32-make cleandata  DELETE data/ (users, complaints, logs)
#
# Works from cmd, PowerShell and Git Bash on Windows (recipes always run
# in cmd.exe there), and with GNU make on Linux/macOS.
# ══════════════════════════════════════════════════════════════

CC       = gcc
CXX      = g++
CFLAGS   = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2 -std=c++11
PYTHON  ?= python

ifeq ($(OS),Windows_NT)
  SHELL   := cmd.exe
  EXE     := .exe
  LDFLAGS := -Wl,--stack,8388608
  MKDIR    = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
  RM_FILE  = if exist "$(subst /,\,$(1))" del /q "$(subst /,\,$(1))"
  RM_DIR   = if exist "$(subst /,\,$(1))" rmdir /s /q "$(subst /,\,$(1))"
  RUN      = $(subst /,\,$(1))
  BLANK    = echo.
else
  EXE     :=
  LDFLAGS :=
  MKDIR    = mkdir -p "$(1)"
  RM_FILE  = rm -f "$(1)"
  RM_DIR   = rm -rf "$(1)"
  RUN      = ./$(1)
  BLANK    = echo
endif

# Directories
SRC_ENGINE    = src/engine
SRC_INTERFACE = src/interface
SRC_CLI       = src/cli
OBJ_DIR       = obj
BIN_DIR       = bin

# Targets
TARGET = $(BIN_DIR)/complaint_analyzer$(EXE)
CLI    = $(BIN_DIR)/engine_cli$(EXE)
TEST   = $(BIN_DIR)/test_duplicate$(EXE)
EVAL   = $(BIN_DIR)/eval_nlp$(EXE)

ENGINE_MODULES    = hash_table priority_queue nlp_analyzer duplicate_detector \
                    file_manager route_optimizer workflow
INTERFACE_MODULES = Person User Admin Worker DepartmentOfficer AuthManager \
                    Dashboard MenuSystem

ENGINE_OBJECTS    = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(ENGINE_MODULES)))
INTERFACE_OBJECTS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(INTERFACE_MODULES))) $(OBJ_DIR)/main.o

ENGINE_HEADERS    = $(wildcard include/engine/*.h)
INTERFACE_HEADERS = $(wildcard include/interface/*.h)

# ──── Default target ────
all: $(TARGET) $(CLI)
	@$(BLANK)
	@echo   Build successful!  Console app: $(TARGET)   Web bridge: $(CLI)
	@$(BLANK)

# ──── Directories ────
$(OBJ_DIR) $(BIN_DIR) data:
	@$(call MKDIR,$@)

# ──── Link ────
$(TARGET): $(ENGINE_OBJECTS) $(INTERFACE_OBJECTS) | $(BIN_DIR) data
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(CLI): $(ENGINE_OBJECTS) $(OBJ_DIR)/engine_cli.o | $(BIN_DIR) data
	$(CC) $(CFLAGS) -o $@ $^

$(TEST): test_duplicate.c $(OBJ_DIR)/duplicate_detector.o $(OBJ_DIR)/nlp_analyzer.o $(OBJ_DIR)/hash_table.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. -o $@ $^

$(EVAL): tools/eval_nlp.c $(OBJ_DIR)/nlp_analyzer.o $(OBJ_DIR)/hash_table.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# ──── Compile ────
$(OBJ_DIR)/%.o: $(SRC_ENGINE)/%.c $(ENGINE_HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_INTERFACE)/%.cpp $(ENGINE_HEADERS) $(INTERFACE_HEADERS) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/main.o: main.cpp $(ENGINE_HEADERS) $(INTERFACE_HEADERS) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/engine_cli.o: $(SRC_CLI)/engine_cli.c $(ENGINE_HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ──── Convenience targets ────
run: $(TARGET)
	$(call RUN,$(TARGET))

web: $(CLI)
	$(PYTHON) web/server.py

test: $(TEST)
	$(call RUN,$(TEST))

eval: $(EVAL)
	$(PYTHON) tools/eval_nlp.py

stress: $(CLI)
	$(PYTHON) tools/stress_test.py

# ──── Clean (build output only — data/ is kept) ────
clean:
	@$(call RM_DIR,$(OBJ_DIR))
	@$(call RM_FILE,$(TARGET))
	@$(call RM_FILE,$(CLI))
	@$(call RM_FILE,$(TEST))
	@$(call RM_FILE,$(EVAL))
	@echo Cleaned build artifacts.

# ──── Clean data (deletes users, complaints, departments and logs!) ────
cleandata:
	@$(call RM_DIR,data)
	@echo Cleaned data files.

.PHONY: all run web test eval stress clean cleandata
