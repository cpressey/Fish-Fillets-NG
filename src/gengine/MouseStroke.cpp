/*
 * Copyright (C) 2004 Ivo Danihelka (ivo@danihelka.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "MouseStroke.h"

#include "StringTool.h"

//-----------------------------------------------------------------
/**
 * Create new stroke from MouseButtonEvent.
 */
MouseStroke::MouseStroke(const SDL_MouseButtonEvent &event)
    : m_loc(event.x, event.y)
{
    m_button = event.button;
}
//-----------------------------------------------------------------
/**
 * Create manualy MouseStroke.
 * @param button the mouse button index
 * (SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT)
 * @param x x coordinate of press
 * @param y y coordinate of press
 */
MouseStroke::MouseStroke(Uint8 button, int x, int y)
    : m_loc(x, y)
{
    m_button = button;
}
//-----------------------------------------------------------------
/**
 * Compare pressed buttons.
 * NOTE: x, y is ignored
 */
bool
MouseStroke::equals(const MouseStroke &other) const
{
    return m_button == other.m_button;
}
//-----------------------------------------------------------------
/**
 * Return text fashion.
 */
std::string
MouseStroke::toString() const
{
    return StringTool::toString(m_button);
}

