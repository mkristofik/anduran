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

// Link against this one if you're NOT building an SDL app.

#include "log_utils.h"
#include <iostream>

namespace
{
    void log_msg(const char *level, const std::string &msg)
    {
        std::cerr << level << ": " << msg << '\n';
    }
}


// LogCategory is only relevant for SDL, allows control over whether certain
// classes of messages print or not.  It does not appear in the output.
void log_info(const std::string &msg, LogCategory)
{
    log_msg("INFO", msg);
}

void log_warn(const std::string &msg, LogCategory)
{
    log_msg("WARN", msg);
}

void log_error(const std::string &msg, LogCategory)
{
    log_msg("ERROR", msg);
}

void log_critical(const std::string &msg, LogCategory)
{
    log_msg("CRITICAL", msg);
}
