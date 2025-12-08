CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -O2 -g -MMD -MP

# Source files
CLIENT_SRC := ./client.cpp
SERVER_SRC := ./server.cpp
TEST_SRC := $(shell find . -name "test_*.cpp")
COMMON_SRC := $(filter-out $(CLIENT_SRC) $(SERVER_SRC) $(TEST_SRC), $(shell find . -name "*.cpp")) 

# Object files
CLIENT_OBJ := $(CLIENT_SRC:.cpp=.o)
SERVER_OBJ := $(SERVER_SRC:.cpp=.o)
TEST_OBJ := $(TEST_SRC:.cpp=.o)
COMMON_OBJ := $(COMMON_SRC:.cpp=.o)

# Executables
CLIENT = client
SERVER = server
TEST_BIN := $(notdir $(TEST_SRC:.cpp=))

# Rules
all: $(CLIENT) $(SERVER) 

$(CLIENT): $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SERVER): $(SERVER_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_BIN): %: $(TEST_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(filter %/$@.o,$(TEST_OBJ)) $(COMMON_OBJ)

tests: $(TEST_BIN) 

run-tests: $(TEST_BIN) 
	@for t in $(TEST_BIN); do \
		echo "Running $$t..."; \
		./$$t || exit 1; \
		echo "Finished $$t..."; \
	done

clean:
	rm -f $(CLIENT) $(SERVER) $(TEST_BIN) $(COMMON_OBJ) $(CLIENT_OBJ) $(SERVER_OBJ) $(TEST_OBJ)

# Dependency files
-include $(CLIENT_OBJ:.o=.d) \
         $(SERVER_OBJ:.o=.d) \
         $(TEST_OBJ:.o=.d) \
         $(COMMON_OBJ:.o=.d)

.PHONY: all clean
