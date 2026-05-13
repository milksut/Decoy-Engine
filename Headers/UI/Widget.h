#pragma once
#include <glm/glm.hpp>
#include <functional>

class Widget
{
public:
    glm::vec2 position; // NDC edge
	glm::vec2 size;     // NDC height weight
    bool visible = true;

    Widget(glm::vec2 position, glm::vec2 size)
        : position(position), size(size) {
    }

    virtual ~Widget() = default;

    /// <summary>
    ///     Checks whether the mouse cursor is inside the button bounds.
    /// </summary>
    /// <param name="mouse_x">[in] Mouse X position.</param>
    /// <param name="mouse_y">[in] Mouse Y position.</param>
    /// <returns>True if mouse is inside the button rectangle.</returns>
    bool is_hovered(float mouse_x, float mouse_y) const
    {
        return mouse_x >= position.x && mouse_x <= position.x + size.x &&
            mouse_y >= position.y && mouse_y <= position.y + size.y;
    }

    /// <summary>
    ///     Updates the UI element state based on input.
    /// </summary>
    /// <param name="mouse_x">[in] Mouse X position.</param>
    /// <param name="mouse_y">[in] Mouse Y position.</param>
    /// <param name="clicked">[in] Whether mouse button is pressed.</param>
    virtual void update(float mouse_x, float mouse_y, bool clicked) = 0;

    /// <summary>
    ///     Renders the UI element to the screen.
    /// </summary>
    virtual void render() = 0;
};