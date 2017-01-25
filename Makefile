#    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
#    Part of the Champions of Anduran project.
# 
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License version 2
#    or at your option any later version.
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY.
# 
#    See the COPYING.txt file for more details.

CC = gcc
CFLAGS = -Wall -Wextra -O3
CPPFLAGS = -Ic:/libraries \
	-Ic:/MinGW/include \
	-Ic:/libraries/SDL2-2.0.5/x86_64-w64-mingw32/include/SDL2 \
	-Ic:/libraries/SDL2_image-2.0.1/x86_64-w64-mingw32/include/SDL2
CXXFLAGS = -g -Wall -Wextra -std=c++1z
LDFLAGS = -Lc:/libraries/SDL2-2.0.5/x86_64-w64-mingw32/lib \
	-Lc:/libraries/SDL2_image-2.0.1/x86_64-w64-mingw32/lib
LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

BUILD_DIR = build
SRC_DIR = src

RMAPGEN = rmapgen.exe
RMAPGEN_SRC = RandomMap.cpp hex_utils.cpp json_utils.cpp rmapgen.cpp
RMAPGEN_OBJS = $(RMAPGEN_SRC:%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/open-simplex-noise.o
RMAPGEN_DEPS = $(RMAPGEN_OBJS:%.o=%.d)
# note: don't want to list open-simplex-noise.c here. We only list the cpp
# files so that we can substitute .o to get the object files to build. Listing
# the noise object file directly is good enough because the implicit rules
# below will find open-simplex-noise.c.

MAPVIEW = mapview.exe
MAPVIEW_SRC = MapDisplay.cpp RandomMap.cpp SdlSurface.cpp SdlTexture.cpp \
	SdlTextureAtlas.cpp SdlWindow.cpp hex_utils.cpp json_utils.cpp mapview.cpp
MAPVIEW_OBJS = $(MAPVIEW_SRC:%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/open-simplex-noise.o
MAPVIEW_DEPS = $(MAPVIEW_OBJS:%.o=%.d)

.PHONY : all clean

all : $(RMAPGEN) $(MAPVIEW)

$(RMAPGEN) : $(RMAPGEN_OBJS)
	$(CXX) $(RMAPGEN_OBJS) -o $@

$(MAPVIEW) : $(MAPVIEW_OBJS)
	$(CXX) $(MAPVIEW_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Auto-generate a dependency file for each cpp file. We first create a build
# directory to house all intermediate files. See example under "Automatic
# Prerequisites" in the GNU make manual.
#     leading '@' = run this command silently
#     /NUL = testing for a nul file ensures we have an actual directory
#     -MM = use compiler to write a rule for the file with build dependencies
#     -MT #1 = substitute *.o for *.d, write the rule for the .o file target
#     -MT #2 = also write the rule for the .d file target so it also depends on
#              the same files as the .o file
#     $@ = path to the .d file
#     $< = path to the .cpp file
$(BUILD_DIR)/%.d : $(SRC_DIR)/%.cpp
	@if not exist $(BUILD_DIR)/NUL mkdir $(BUILD_DIR)
	$(CXX) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CPPFLAGS) $< > $@

$(BUILD_DIR)/%.d : $(SRC_DIR)/%.c
	@if not exist $(BUILD_DIR)/NUL mkdir $(BUILD_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $< > $@

# Modify the standard implicit rule for building cpp files so the .o files land
# in our build directory.
$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Include the auto-generated dependency files. Each file contains the
# dependencies for that object file, the above implicit rule says how to build
# it.  Skip this step if we're doing 'make clean'. Otherwise, 'make clean' on
# an already clean build will create those files only to delete them again.
ifneq ($(MAKECMDGOALS),clean)
include $(RMAPGEN_DEPS)
include $(MAPVIEW_DEPS)
endif

# Remove intermediate build files and the executables.  Leading '-' means ignore
# errors (e.g., if any files are already deleted).
clean :
	-rmdir /q /s $(BUILD_DIR)
	-del $(RMAPGEN) $(MAPVIEW)
