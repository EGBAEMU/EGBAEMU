CC := g++
CCFLAGS := -std=c++17 -g -Wall
LDFLAGS :=
SRC := src
OUT := bin
BUILDDIR := build

srcs := $(wildcard $(SRC)/*.cpp)
objs = $(patsubst %, $(BUILDDIR)/%,$(srcs:.cpp=.o))
deps = $(patsubst %, $(BUILDDIR)/%,$(srcs:.cpp=.d))

.PHONY: all clean gbaemu

all: gbaemu

gbaemu: $(objs)
	mkdir -p $(OUT)
	$(CC) $^ -o $(OUT)/$@ $(LDFLAGS)

#%.o: %.cpp
$(objs): $(BUILDDIR)/%.o : %.cpp
	mkdir -p $(BUILDDIR)/$(SRC)
	$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(objs) $(deps) $(OUT) $(BUILDDIR)

-include $(deps)