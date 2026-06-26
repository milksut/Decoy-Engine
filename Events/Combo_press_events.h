#pragma once

#include "Globals.h"

class Combo_button_press_event : public Event_management::Event
{
public:
    Key_combo combo;

    Combo_button_press_event(const Key_combo combo_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Combo_button_pressed),
        combo(combo_in){}
};

class Combo_button_hold_event : public Event_management::Event
{
public:
    Key_combo combo;

    Combo_button_hold_event(const Key_combo combo_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Combo_button_hold),
        combo(combo_in){}
};

class Combo_button_release_event : public Event_management::Event
{
public:
    Key_combo combo;

    Combo_button_release_event(const Key_combo combo_in)
        : Event(Event_management::Event_timing::Immediate, Event_management::Event_type::Combo_button_released),
        combo(combo_in){}
};