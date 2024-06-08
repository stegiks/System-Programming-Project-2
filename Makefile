# Author : Stefanos Gkikas
# Declare some path variables
SRC := ./src
INCLUDE := ./include
BUILD := ./build
BIN := ./bin

# Compilers
CC := gcc
CXX := g++

# Arguments for programs (IP and port)
# Default values are
ARGS_COM := localhost 8000 issueJob ls
ARGS_SER := 8000 10 2

# Flags and includes. Threads are needed so we add -pthread -lrt
CFLAGS := -Wall -Wextra -Werror -g -pthread -lrt -I$(INCLUDE)

# Targets
SRC_FILES := $(wildcard $(SRC)/*.c)
OBJ_FILES := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(SRC_FILES))
COMMANDER_OBJ_FILES := $(filter-out $(BUILD)/jobExecutorServer.o $(BUILD)/helpServer.o $(BUILD)/list_jobs.o, $(OBJ_FILES))
SERVER_OBJ_FILES := $(filter-out $(BUILD)/jobCommander.o $(BUILD)/helpCommander.o, $(OBJ_FILES))

# Compilation
all: $(BIN)/jobCommander $(BIN)/jobExecutorServer $(BIN)/progDelay

$(BUILD)/%.o: $(SRC)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD)/progDelay.o: progDelay.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Linking
$(BIN)/jobCommander: $(COMMANDER_OBJ_FILES)
	$(CC) $^ -o $@  $(CFLAGS)

$(BIN)/jobExecutorServer: $(SERVER_OBJ_FILES) 
	$(CC) $^ -o $@ $(CFLAGS)

$(BIN)/progDelay: $(BUILD)/progDelay.o
	$(CC) $^ -o $@ $(CFLAGS)

run_commander: $(BIN)/jobCommander
	$(BIN)/jobCommander $(ARGS_COM)

run_executor: $(BIN)/jobExecutorServer
	$(BIN)/jobExecutorServer $(ARGS_SER)

# Clean
# Find object files and executables and remove them
# Some dummies txt files are kept so the directories get commited
clean:
	find $(BUILD) -type f -name '*.o' -exec rm -f {} +
	find $(BIN) -type f ! -name '*.txt' -exec rm -f {} +
	
# Valgrind
valgrind_commander: $(BIN)/jobCommander
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(BIN)/jobCommander $(ARGS_COM)

valgrind_executor: $(BIN)/jobExecutorServer
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(BIN)/jobExecutorServer $(ARGS_SER)