#pragma once
#include "Widget.h"
#include "Shader.h"
#include "TextRenderer.h"
#include <functional>
#include <string>

class Button : public Widget
{
private:
    bool was_pressed = false;

    Shader* shader;
    TextRenderer* text;

public:
    std::string label;
    std::function<void()> on_click;

    glm::vec4 base_color = { 0.3f, 0.3f, 0.3f, 1.0f };
    glm::vec4 hover_color = { 0.5f, 0.5f, 0.5f, 1.0f };
    glm::vec4 pressed_color = { 0.2f, 0.2f, 0.2f, 1.0f };

    glm::vec4 current_color;

    float text_scale = 1.0f;

    Button(glm::vec2 pos,
        glm::vec2 size,
        const std::string& label,
        std::function<void()> on_click,
        Shader* shader,
        TextRenderer* text)
        : Widget(pos, size),
        label(label),
        on_click(on_click),
        shader(shader),
        text(text)
    {
        current_color = base_color;
    }

    /// <summary>
    ///     Sets the button's base and current color.
    /// </summary>
    /// <param name="color">[in] RGBA color value to assign.</param>
    void set_color(const glm::vec4& color)
    {
        base_color = color;
        current_color = color;
    }

    /// <summary>
    ///     Sets the scale factor used for text rendering.
    /// </summary>
    /// <param name="s">[in] Scale multiplier for text size.</param>
    void set_text_scale(float s)
    {
        text_scale = s;
    }

    /// <summary>
    ///     Updates button state based on mouse interaction and triggers click callback.
    /// </summary>
    /// <param name="mouse_x">[in] Mouse X position.</param>
    /// <param name="mouse_y">[in] Mouse Y position.</param>
    /// <param name="clicked">[in] True if mouse button is currently pressed.</param>
    void update(float mouse_x, float mouse_y, bool clicked) override
    {
        if (!visible) return;

        if (is_hovered(mouse_x, mouse_y))
        {
            if (clicked)
            {
                current_color = pressed_color;

                if (!was_pressed && on_click)
                    on_click();

                was_pressed = true;
            }
            else
            {
                current_color = hover_color;
                was_pressed = false;
            }
        }
        else
        {
            current_color = base_color;
            was_pressed = false;
        }
    }

    /// <summary>
    ///     Renders the UI button background and centered text label.
    /// </summary>
    /// <remarks>
    ///     Creates a temporary quad for the button background each frame,
    ///     then renders centered text on top using measured text size.
    /// </remarks>
    void render() override
    {
        // =========================
        // BACKGROUND (UI QUAD)
        // =========================
        draw_quad(shader, current_color);

        // =========================
        // TEXT CENTERING
        // =========================
        glm::vec2 text_size = text->get_text_size(label, text_scale);

        float text_x = position.x + (size.x - text_size.x) * 0.5f;
        float text_y = position.y + (size.y - text_size.y) * 0.5f;

        text->render_text(label, text_x, text_y, text_scale);
    }
};