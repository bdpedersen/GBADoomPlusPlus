# Simple Makefile for GBADoom (Qt6, macOS, Apple Silicon)

# ---- Project -----------------------------------------------------

TARGET      := GBADoomCpp

SRC_DIR     := ../../cppsrc
OBJ_DIR     := cppbuild

# Use all C sources in source/, plus the C++ ones we know about.
# (If you add more .cpp files, just drop them in $(SRC_DIR)/)
CPP_SOURCES := $(wildcard $(SRC_DIR)/*.cc)

SRCS        := $(CPP_SOURCES)

# Port specific source files
SRCS += i_system_e32.cc
SRCS += wadfilereader.cc
SRCS += i_main.cc


# ---- Original Doom Sources --------------------------------------------
#SRCS += ../../gamedata/original/doom_iwad.cc
#SRCS += ../../gamedata/original/w_wad.cc
#SRCS += ../../gamedata/original/w_nc.cc
#SRCS += ../../gamedata/original/z_bmalloc.cc
#vpath %.cc $(SRC_DIR) ../../gamedata/original

# ---- Guardmalloc Sources ----------------------------------------
#SRCS += ../../gamedata/original/doom_iwad.cc
#SRCS += ../../gamedata/original/w_wad.cc
#SRCS += ../../gamedata/original/z_bmalloc.cc
#SRCS += ../../gamedata/guard/w_nc.cc
#SRCS += guardmalloc/guardmalloc.cc
#vpath %.cc guardmalloc ../../gamedata/guard ../../gamedata/original $(SRC_DIR)

# ---- Minimem Sources ----------------------------------------
SRCS += ../../gamedata/minimem/w_nc.cc
SRCS += ../../gamedata/minimem/tagheap.cc
SRCS += ../../gamedata/minimem/z_mem_emu.cc
vpath %.cc ../../gamedata/minimem . $(SRC_DIR)


# ---- Objects -----------------------------------------------------
OBJS        := $(patsubst %.cc,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

# ---- Toolchain ---------------------------------------------------

CXX         := clang++
CC          := clang
AR          := ar
RM          := rm -f
MKDIR_P     := mkdir -p

# QT configuration
QT_MODULE   := Qt6Widgets
QT_CFLAGS   := $(shell pkg-config --cflags $(QT_MODULE))
QT_LIBS     := $(shell pkg-config --libs $(QT_MODULE))

# ---- Flags / Defines ---------------------------------------------

DEFINES     := \
    -DQT_DEPRECATED_WARNINGS \
    -DRANGECHECK \
	-DRPT_MALLOC \
    -D_CRT_SECURE_NO_WARNINGS \

INCLUDEPATH := \
    -I../../include \
	-I../../gamedata/minimem 


CXXFLAGS    := -std=c++17 -Wall -Wextra -Werror -Wno-unknown-pragmas  -Os $(DEFINES) $(INCLUDEPATH)
CFLAGS      += $(QT_CFLAGS)
CXXFLAGS    += $(QT_CFLAGS)

LDFLAGS     := $(QT_LIBS)

# ---- Targets -----------------------------------------------------

.PHONY: all clean distclean run

all: $(TARGET)

$(TARGET): $(OBJ_DIR) $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(OBJ_DIR):
	$(MKDIR_P) $(OBJ_DIR)


# C++ compilation rule - works for .cc files from any directory in vpath
$(OBJ_DIR)/%.o: %.cc
	$(MKDIR_P) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)/*.o

distclean: clean
	$(RM) $(TARGET)

run: all
	./$(TARGET)
