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

// Link against this one if you're building an SDL app.

#include "log_utils.h"
#include "SDL.h"

void log_debug(const std::string &msg, LogCategory category)
{
    SDL_LogDebug(static_cast<int>(category), msg.c_str());
}

void log_info(const std::string &msg, LogCategory category)
{
    SDL_LogInfo(static_cast<int>(category), msg.c_str());
}

void log_warn(const std::string &msg, LogCategory category)
{
    SDL_LogWarn(static_cast<int>(category), msg.c_str());
}

void log_error(const std::string &msg, LogCategory category)
{
    SDL_LogError(static_cast<int>(category), msg.c_str());
}

void log_critical(const std::string &msg, LogCategory category)
{
    SDL_LogCritical(static_cast<int>(category), msg.c_str());
}
