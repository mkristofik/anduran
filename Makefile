#    Copyright (C) 2016-2020 by Michael Kristofik <kristo605@gmail.com>
#    Part of the Champions of Anduran project.
# 
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License version 2
#    or at your option any later version.
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY.
# 
#    See the COPYING.txt file for more details.

# Import OS-specific stuff, notably:
# - include and linker paths for libs we depend on
# - build dir name
# - shell commands
ifeq ($(OS),Windows_NT)
    include Makefile.win
else
    include Makefile.lx
endif

CC = gcc
CFLAGS = -Wall -Wextra -Werror -O3
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++17
SRC_DIR = src
TEST_DIR = tests

CPPFLAGS += -I$(SRC_DIR)

# Search these directories for any bare filenames that appear later.
vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

RMAPGEN = rmapgen$(EXE)
RMAPGEN_SRC = RandomMap.cpp hex_utils.cpp json_utils.cpp object_types.cpp rmapgen.cpp
RMAPGEN_OBJS = $(RMAPGEN_SRC:%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/open-simplex-noise.o
RMAPGEN_DEPS = $(RMAPGEN_OBJS:%.o=%.d)
# note: don't want to list open-simplex-noise.c here. We only list the cpp
# files so that we can substitute .o to get the object files to build. Listing
# the noise object file directly is good enough because the implicit rules
# below will find open-simplex-noise.c.

MAPVIEW = mapview$(EXE)
MAPVIEW_SRC = MapDisplay.cpp \
	RandomMap.cpp \
	SdlApp.cpp \
	SdlImageManager.cpp \
	SdlSurface.cpp \
	SdlTexture.cpp \
	SdlWindow.cpp \
	hex_utils.cpp \
	json_utils.cpp \
	mapview.cpp \
	object_types.cpp
MAPVIEW_OBJS = $(MAPVIEW_SRC:%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/open-simplex-noise.o
MAPVIEW_DEPS = $(MAPVIEW_OBJS:%.o=%.d)

ANDURAN = anduran$(EXE)
ANDURAN_SRC = GameState.cpp \
	MapDisplay.cpp \
	Pathfinder.cpp \
	RandomMap.cpp \
	SdlApp.cpp \
	SdlImageManager.cpp \
	SdlSurface.cpp \
	SdlTexture.cpp \
	SdlWindow.cpp \
	UnitManager.cpp \
	anduran.cpp \
	anim_utils.cpp \
	battle_utils.cpp \
	hex_utils.cpp \
	json_utils.cpp \
	object_types.cpp \
	team_color.cpp
ANDURAN_OBJS = $(ANDURAN_SRC:%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/open-simplex-noise.o
ANDURAN_DEPS = $(ANDURAN_OBJS:%.o=%.d)

UNITTESTS = unittests$(EXE)
UNITTESTS_SRC = battle_utils.cpp \
	object_types.cpp \
	$(wildcard $(TEST_DIR)/*.cpp)
UNITTESTS_OBJS = $(UNITTESTS_SRC:%.cpp=$(BUILD_DIR)/%.o)
UNITTESTS_DEPS = $(UNITTESTS_OBJS:%.o=%.d)

.PHONY : all clean test

all : $(RMAPGEN) $(MAPVIEW) $(ANDURAN) $(UNITTESTS)

test : $(UNITTESTS)

$(RMAPGEN) : $(RMAPGEN_OBJS)
	$(CXX) $(RMAPGEN_OBJS) -o $@

$(MAPVIEW) : $(MAPVIEW_OBJS)
	$(CXX) $(MAPVIEW_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(ANDURAN) : $(ANDURAN_OBJS)
	$(CXX) $(ANDURAN_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(UNITTESTS) : $(UNITTESTS_OBJS)
	$(CXX) $(UNITTESTS_OBJS) $(LDFLAGS) -o $@
	@./$(UNITTESTS)

# Auto-generate a dependency file for each cpp file. We first create a build
# directory to house all intermediate files. See example under "Automatic
# Prerequisites" in the GNU make manual.
#     leading '@' = run this command silently
#     -MM = use compiler to write a rule for the file with build dependencies
#     -MT #1 = substitute *.o for *.d, write the rule for the .o file target
#     -MT #2 = also write the rule for the .d file target so it also depends on
#              the same files as the .o file
#     $@ = path to the .d file
#     $(@D) = directory part of the .d file, must use quotes because Windows
#             mkdir can't handle forward slashes without them
#     $< = path to the .cpp file
$(BUILD_DIR)/%.d : %.cpp
	@$(MKDIR_CMD) "$(@D)"
	$(CXX) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CPPFLAGS) $< > $@

$(BUILD_DIR)/%.d : %.c
	@$(MKDIR_CMD) "$(@D)"
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $< > $@

# Modify the standard implicit rule for building cpp files so the .o files land
# in our build directory.
$(BUILD_DIR)/%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Include the auto-generated dependency files. Each file contains the
# dependencies for that object file, the above implicit rule says how to build
# it.  Skip this step if we're doing 'make clean'. Otherwise, 'make clean' on
# an already clean build will create those files only to delete them again.
ifeq ($(MAKECMDGOALS), test)
    include $(UNITTESTS_DEPS)
else ifneq ($(MAKECMDGOALS), clean)
    include $(RMAPGEN_DEPS)
    include $(MAPVIEW_DEPS)
    include $(ANDURAN_DEPS)
    include $(UNITTESTS_DEPS)
endif

# Remove intermediate build files and the executables.  Leading '-' means ignore
# errors (e.g., if any files are already deleted).
clean :
	-$(RM_DIR_CMD) $(BUILD_DIR)
	-$(RM_CMD) $(RMAPGEN) $(MAPVIEW) $(ANDURAN) $(UNITTESTS)
