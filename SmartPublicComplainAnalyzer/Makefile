# ══════════════════════════════════════════════════════════════
# Makefile for Smart Public Complaint Analyzer
# C (Engine Layer) + C++ (Interface Layer)
# ══════════════════════════════════════════════════════════════

CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2 -std=c++11

# Directories
SRC_ENGINE    = src/engine
SRC_INTERFACE = src/interface
INC_ENGINE    = include/engine
INC_INTERFACE = include/interface
OBJ_DIR       = obj
BIN_DIR       = bin

# Target
TARGET = $(BIN_DIR)/complaint_analyzer.exe

# Source files
C_SOURCES   = $(SRC_ENGINE)/hash_table.c \
              $(SRC_ENGINE)/priority_queue.c \
              $(SRC_ENGINE)/nlp_analyzer.c \
              $(SRC_ENGINE)/duplicate_detector.c \
              $(SRC_ENGINE)/file_manager.c \
              $(SRC_ENGINE)/route_optimizer.c

CPP_SOURCES = $(SRC_INTERFACE)/Person.cpp \
              $(SRC_INTERFACE)/User.cpp \
              $(SRC_INTERFACE)/Admin.cpp \
              $(SRC_INTERFACE)/Worker.cpp \
              $(SRC_INTERFACE)/DepartmentOfficer.cpp \
              $(SRC_INTERFACE)/AuthManager.cpp \
              $(SRC_INTERFACE)/Dashboard.cpp \
              $(SRC_INTERFACE)/MenuSystem.cpp \
              main.cpp

# Object files
C_OBJECTS   = $(patsubst $(SRC_ENGINE)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
CPP_OBJECTS = $(patsubst $(SRC_INTERFACE)/%.cpp,$(OBJ_DIR)/%.o,$(filter $(SRC_INTERFACE)/%.cpp,$(CPP_SOURCES)))
CPP_OBJECTS += $(OBJ_DIR)/main.o

# All object files
OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS)

# ──── Default target ────
all: dirs $(TARGET)
	@echo ""
	@echo "  Build successful! Run with: $(TARGET)"
	@echo ""

# ──── Create directories ────
dirs:
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	@if not exist data mkdir data

# ──── Link all object files ────
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ──── Compile C engine files ────
$(OBJ_DIR)/hash_table.o: $(SRC_ENGINE)/hash_table.c $(INC_ENGINE)/hash_table.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/priority_queue.o: $(SRC_ENGINE)/priority_queue.c $(INC_ENGINE)/priority_queue.h $(INC_ENGINE)/complaint_types.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/nlp_analyzer.o: $(SRC_ENGINE)/nlp_analyzer.c $(INC_ENGINE)/nlp_analyzer.h $(INC_ENGINE)/hash_table.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/duplicate_detector.o: $(SRC_ENGINE)/duplicate_detector.c $(INC_ENGINE)/duplicate_detector.h $(INC_ENGINE)/nlp_analyzer.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/file_manager.o: $(SRC_ENGINE)/file_manager.c $(INC_ENGINE)/file_manager.h $(INC_ENGINE)/complaint_types.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/route_optimizer.o: $(SRC_ENGINE)/route_optimizer.c $(INC_ENGINE)/route_optimizer.h $(INC_ENGINE)/complaint_types.h
	$(CC) $(CFLAGS) -c $< -o $@

# ──── Compile C++ interface files ────
$(OBJ_DIR)/Person.o: $(SRC_INTERFACE)/Person.cpp $(INC_INTERFACE)/Person.h $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/User.o: $(SRC_INTERFACE)/User.cpp $(INC_INTERFACE)/User.h $(INC_INTERFACE)/Person.h $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/Admin.o: $(SRC_INTERFACE)/Admin.cpp $(INC_INTERFACE)/Admin.h $(INC_INTERFACE)/Person.h $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/Worker.o: $(SRC_INTERFACE)/Worker.cpp $(INC_INTERFACE)/Worker.h $(INC_INTERFACE)/Person.h $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/DepartmentOfficer.o: $(SRC_INTERFACE)/DepartmentOfficer.cpp $(INC_INTERFACE)/DepartmentOfficer.h $(INC_INTERFACE)/Person.h $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/AuthManager.o: $(SRC_INTERFACE)/AuthManager.cpp $(INC_INTERFACE)/AuthManager.h $(INC_INTERFACE)/Person.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/Dashboard.o: $(SRC_INTERFACE)/Dashboard.cpp $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/MenuSystem.o: $(SRC_INTERFACE)/MenuSystem.cpp $(INC_INTERFACE)/MenuSystem.h $(INC_INTERFACE)/AuthManager.h $(INC_INTERFACE)/Dashboard.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/main.o: main.cpp $(INC_INTERFACE)/MenuSystem.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ──── Clean ────
clean:
	@if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
	@echo Cleaned build artifacts.

# ──── Clean data ────
cleandata:
	@if exist data rmdir /s /q data
	@echo Cleaned data files.

# ──── Run ────
run: all
	$(TARGET)

.PHONY: all dirs clean cleandata run
