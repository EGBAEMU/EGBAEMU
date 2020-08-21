CC := g++
CCFLAGS := -std=c++17 -g -Wall
LDFLAGS := -lSDL2
SRC := src
OUT := bin
BUILDDIR := build

srcs := $(wildcard $(SRC)/*.cpp)
objs = $(patsubst %, $(BUILDDIR)/%,$(srcs:.cpp=.o))
deps = $(patsubst %, $(BUILDDIR)/%,$(srcs:.cpp=.d))

.PHONY: all clean gbaemu

all: gbaemu

gbaemu: $(objs) $(BUILDDIR)/src/lcd.o
	mkdir -p $(OUT)
	$(CC) $^ -o $(OUT)/$@ $(LDFLAGS)

#%.o: %.cpp
$(objs): $(BUILDDIR)/%.o : %.cpp
	mkdir -p $(BUILDDIR)/$(SRC)
	$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/src/lcd.o: $(SRC)/lcd/lcd.cpp
	mkdir -p $(BUILDDIR)/$(SRC)
	$(CC) $(CCFLAGS) -I$(SRC) -c $^ -o $(BUILDDIR)/$(SRC)/lcd.o

clean:
	rm -rf $(objs) $(deps) $(OUT) $(BUILDDIR)

-include $(deps)