SFML := vendor/SFML-3.1.0
PROGRAM := build/FactoryGame.exe
SOURCES := src/main.cpp src/Grid.cpp src/Debug.cpp src/Tile.cpp src/Engineer.cpp src/IsometricArt.cpp \
           src/TerrainScene.cpp src/EditorUI.cpp src/AssetCatalog.cpp src/AssetArt.cpp \
           src/AssetBrowser.cpp src/AssetBrowserDraw.cpp src/MapEditor.cpp src/EditHistory.cpp
OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(SOURCES))
SFML_LIBRARIES := $(SFML)/lib/libsfml-graphics.a \
                  $(SFML)/lib/libsfml-window.a \
                  $(SFML)/lib/libsfml-system.a

CPPFLAGS := -Iinclude -I$(SFML)/include
CXXFLAGS := -std=c++17 -Wall -Wextra -g

.DELETE_ON_ERROR:

$(PROGRAM): $(OBJECTS) $(SFML_LIBRARIES) Makefile
	$(CXX) $(OBJECTS) $(SFML_LIBRARIES) -o $@

# GCC writes a .d file beside each object with the headers it uses.
build/obj/%.o: src/%.cpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)
