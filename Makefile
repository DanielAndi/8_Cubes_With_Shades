CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -Iincludes
LIBS = -lglfw -ldl -lGL -lX11 -lpthread -lXrandr -lXi
SRC = Shiny_scene.cpp glad.c
TARGET = shininess_scene

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o
