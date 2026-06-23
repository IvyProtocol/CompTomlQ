CXX ?= c++
TARGET := CompTomlQ.out

SOURCES := \
	src/Main.cpp \
	src/Lexer.cpp \
	src/Parser.cpp \
	src/FileHandler.cpp \
	src/Transpiler.cpp

OBJECTS := $(SOURCES:.cpp=.o)

CPPFLAGS += -Iinclude
CXXFLAGS += \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wconversion \
	-Wsign-conversion \
	-Wfloat-equal \
	-Wshadow \
	-Wold-style-cast \
	-Wnon-virtual-dtor \
	-Wunused \
	-Woverloaded-virtual \
	-Werror \
	-Weverything \
	-Wno-c++98-compat \
	-Wno-switch-default \
	-Wno-c++98-compat-pedantic \
	-Wno-covered-switch-default \
	-ferror-limit=999999 \
	-fdiagnostics-color \
	-std=c++2c \
	-pipe

# CMake target_compile_features(a.out PRIVATE cxx_std_26)
CXXFLAGS += -std=c++2c

LDFLAGS += -static
LDLIBS += -lstdc++exp

# Match:
# if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#     add_compile_options(-O3 -flto -march=native)
#     add_link_options(-flto)
# endif()
ifneq (,$(findstring clang,$(shell $(CXX) --version 2>/dev/null | head -n 1)))
	CXXFLAGS += -O3 -flto -march=native
	LDFLAGS += -flto
else ifneq (,$(findstring gcc,$(shell $(CXX) --version 2>/dev/null | head -n 1)))
	CXXFLAGS += -O3 -flto -march=native
	LDFLAGS += -flto
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
