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

    /// <summary>
    ///     Adds a UI widget to the container.
    /// </summary>
    /// <param name="widget">[in] Pointer to the widget to add.</param>
    void add_widget(Widget* widget)
    {
        widgets.push_back(widget);
    }

    /// <summary>
    ///     Updates all visible widgets in the container.
    /// </summary>
    /// <remarks>
    ///     Iterates through the widget list and forwards input state to each active widget.
    /// </remarks>
    /// <param name="mouse_x">[in] Mouse X position.</param>
    /// <param name="mouse_y">[in] Mouse Y position.</param>
    /// <param name="clicked">[in] Mouse click state.</param>
    void update(float mouse_x, float mouse_y, bool clicked)
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible)
                widget->update(mouse_x, mouse_y, clicked);
        }
    }

    /// <summary>
    ///     Renders all visible widgets in the container.
    /// </summary>
    /// <remarks>
    ///     Iterates through the widget list and calls each widget's render function.
    /// </remarks>
    void render()
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible)
                widget->render();
        }
    }

    /// <summary>
    ///     Checks whether the mouse is hovering over any visible widget.
    /// </summary>
    /// <param name="mouse_x">[in] Mouse X position.</param>
    /// <param name="mouse_y">[in] Mouse Y position.</param>
    /// <returns>True if any widget is hovered.</returns>
    bool is_hovered(float mouse_x, float mouse_y)
    {
        for (Widget* widget : widgets)
        {
            if (widget && widget->visible && widget->is_hovered(mouse_x, mouse_y))
                return true;
        }
        return false;
    }
};