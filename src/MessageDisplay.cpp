/*
    Copyright (C) 2026 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "MessageDisplay.h"

#include "pixel_utils.h"

namespace
{
    const int MARGIN_X = 20;
    const int MARGIN_Y = 40;
    const int MAX_WIDTH = 250;
}


MessageDisplay::MessageDisplay(SdlWindow &win)
    : PopupDisplay(win),
    font_(FontType::sans_serif, 14),
    message_()
{
}

void MessageDisplay::set_message(const std::string &msg)
{
    auto surf = font_.render_wrapped(msg, COLOR_LIGHT_GREY, MAX_WIDTH);
    message_ = SdlTexture::make_image(surf, *win_);
    center_in_window(message_.width() + MARGIN_X * 2, message_.height() + MARGIN_Y * 2);
    status_ = PopupStatus::running;
}

void MessageDisplay::draw(Uint32)
{
    draw_background();
    draw_border();
    message_.draw(SDL_Point{displayArea_.x + MARGIN_X, displayArea_.y + MARGIN_Y});
}
