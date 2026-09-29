CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iimgui -Iimgui/backends `sdl2-config --cflags`

SOURCES = src/main.cpp src/chip8.cpp \
          imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp \
          imgui/imgui_widgets.cpp \
          imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_sdlrenderer2.cpp

OBJECTS = $(SOURCES:.cpp=.o)
TARGET = chip8

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) `sdl2-config --libs`

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean