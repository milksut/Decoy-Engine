#pragma once

#include <vector>
#include "UI/Widget.h"
#include "UI/Button.h"
#include "UI/Text_panel.h"


class UI_manager
{
private:
    std::vector<Widget*> widgets;

public:

    void add_widget(Widget* widget)
    {
        widgets.push_back(widget);
    }

    void update(float mouse_x, float mouse_y, bool clicked)
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible)
                widget->update(mouse_x, mouse_y, clicked);
        }
    }

    void render()
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible)
                widget->render();
        }
    }
};