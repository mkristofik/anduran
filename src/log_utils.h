/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <string>

// SDL apps require the SDL_Log* family of functions to write to the console.
// Normal printing to std::cout or std::cerr will not appear there.  This file
// provides a unified interface.  Link against the appropriate implementation,
// either log_utils_sdl.cpp or log_utils_console.cpp.

// This matches the SDL_LogCategory enum in SDL_log.h
enum class LogCategory {
    app,
    error,
    assert,
    system,
    audio,
    video,
    render,
    input
};

void log_info(const std::string &msg, LogCategory category = LogCategory::app);
void log_warn(const std::string &msg, LogCategory category = LogCategory::app);
void log_error(const std::string &msg, LogCategory category = LogCategory::app);
void log_critical(const std::string &msg, LogCategory category = LogCategory::app);

#endif
