SRC := src
OUT := bin
BUILDDIR := build

CC := g++
CCFLAGS := -std=c++17 -g -ffast-math -Wall -I$(SRC)
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

.PHONY: all clean gbaemu

all: gbaemu

gbaemu: $(objs)
	mkdir -p $(OUT)
	$(CC) $^ -o $(OUT)/$@ $(LDFLAGS)

#%.o: %.cpp
$(objs): $(BUILDDIR)/%.o : %.cpp
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(objs) $(deps) $(OUT) $(BUILDDIR)

-include $(deps)
