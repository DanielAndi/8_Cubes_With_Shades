# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

# Source files
SRC = main.cpp glad.c
OBJ = $(SRC:.cpp=.o)
OBJ := $(OBJ:.c=.o)

# Libraries
LIBS = -lglfw -ldl -lGL -lX11 -lpthread -lXrandr -lXi

# Output binary
TARGET = lighting_demo

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)
