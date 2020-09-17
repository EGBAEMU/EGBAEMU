SRC := src
OUT := bin
BUILDDIR := build

MSC_BUILDDIR := msc_build

CC := g++
CCFLAGS := -std=c++17 -ffast-math -Wall -I$(SRC)
ifeq ($(OS),Windows_NT)
  LDFLAGS := -lmingw32 -lSDL2main -lSDL2 -pthread
else
  LDFLAGS := -lSDL2 -pthread
endif

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

#srcs := $(wildcard $(SRC)/*.cpp)
srcs := $(call rwildcard,$(SRC),*.cpp)
objs = $(patsubst %, $(BUILDDIR)/%,$(srcs:.cpp=.o))
deps = $(patsubst %, $(BUILDDIR)/%,$(srcs:.cpp=.d))

.PHONY: all clean gbaemu windows CMakeLists.txt release debug

all: release

debug: CCFLAGS += -g
debug: gbaemu
release: CCFLAGS += -Ofast
release: gbaemu

gbaemu: $(objs)
	@mkdir -p $(OUT)
	$(CC) $^ -o $(OUT)/$@ $(LDFLAGS)

#%.o: %.cpp
$(objs): $(BUILDDIR)/%.o : %.cpp
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@

CMakeLists.txt:
	echo "cmake_minimum_required(VERSION 3.7)" > CMakeLists.txt
	echo "project(gbaemu)" >> CMakeLists.txt
	echo "set (CMAKE_CXX_STANDARD 17)" >> CMakeLists.txt
	echo "find_package(SDL2 REQUIRED)" >> CMakeLists.txt
	echo -n "include_directories($$" >> CMakeLists.txt
	echo "{SDL2_INCLUDE_DIRS} $(SRC))" >> CMakeLists.txt
	echo "add_executable(gbaemu $(srcs))" >> CMakeLists.txt
	echo -n "target_link_libraries(gbaemu " >> CMakeLists.txt
	echo "SDL2main SDL2)" >> CMakeLists.txt
#	echo -n "target_link_libraries(gbaemu $$" >> CMakeLists.txt
#	echo "{SDL2_LIBRARIES})" >> CMakeLists.txt

windows: CMakeLists.txt
	rm -rf $(MSC_BUILDDIR) && \
	mkdir -p $(MSC_BUILDDIR) && \
	cd $(MSC_BUILDDIR) && \
	cmake .. && \
	cmake --build . --config Release --target ALL_BUILD

clean:
	rm -rf $(objs) $(deps) $(OUT) $(BUILDDIR) CMakeLists.txt $(MSC_BUILDDIR)

-include $(deps)
