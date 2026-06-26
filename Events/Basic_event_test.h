#pragma once
#include "Globals.h"

class Mouse_move_event : public Event_management::Event
{
public:
    float x, y;
    Mouse_move_event(const float x, const float y)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Mouse_moved), x(x), y(y) {}

};

class Key_event : public Event_management::Event
{
public:
    int key_code;
    Key_event(const int key_code)
        : Event(Event_management::Event_timing::Queued, Event_management::Event_type::Key_pressed), key_code(key_code) {}

};