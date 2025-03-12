# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2
LIBS = -lcurl
INCLUDES = -I./rapidjson/include

# Source Files
SRC = bfs_graph.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = bfs_graph

# Default target
all: $(EXEC)

# Compile the program
$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(EXEC) $(OBJ) $(LIBS)

# Compile object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(EXEC)

