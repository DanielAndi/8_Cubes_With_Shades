CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -Iincludes
LDFLAGS = -lglfw -ldl -lGL -lX11 -lpthread -lXrandr -lXi

TARGET = shininess_scene
SRC = Shiny_scene.cpp glad.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o
