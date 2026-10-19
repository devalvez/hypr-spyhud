ifeq ($(CXX),g++)
    EXTRA_FLAGS = --no-gnu-unique
else
    EXTRA_FLAGS =
endif

CXXFLAGS ?= -O2
CXXFLAGS += -shared -fPIC -std=c++2b -Wno-narrowing
INCLUDES = `pkg-config --cflags pixman-1 libdrm hyprland libinput libudev wayland-server xkbcommon`

SRC = main.cpp HudDeco.cpp HudPassElement.cpp
TARGET = spyhud.so

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(EXTRA_FLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f ./$(TARGET)

.PHONY: all clean
