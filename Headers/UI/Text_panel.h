#pragma once
#include "Widget.h"
#include "TextRenderer.h"
#include "Shader.h"
#include <vector>
#include <string>

class Text_panel : public Widget
{
private:
    glm::vec4 background_color = { 0.0f, 0.0f, 0.0f, 0.0f };

public:
    std::vector<std::string> lines;

    Shader* ui_shader = nullptr;
    TextRenderer* text_renderer = nullptr;

    float line_spacing = 0.05f;
    float text_scale = 1.0f;

    Text_panel(glm::vec2 position,
        glm::vec2 size,
        Shader* ui_shader,
        TextRenderer* text_renderer)
        : Widget(position, size),
        ui_shader(ui_shader),
        text_renderer(text_renderer)
    {
    }

    /// <summary>
    ///     Sets the background color of the UI element.
    /// </summary>
    /// <param name="color">[in] RGBA background color.</param>
    void set_background_color(glm::vec4 color)
    {
        background_color = color;
    }

    /// <summary>
    ///     Sets the scale factor used for rendering text.
    /// </summary>
    /// <param name="s">[in] Text scale multiplier.</param>
    void set_text_scale(float s)
    {
        text_scale = s;
    }

    /// <summary>
    ///     Sets the spacing between text lines.
    /// </summary>
    /// <param name="s">[in] Line spacing multiplier or offset value.</param>
    void set_line_spacing(float s)
    {
        line_spacing = s;
    }

    /// <summary>
    ///     Sets the text content of a specific line.
    /// </summary>
    /// <remarks>
    ///     Automatically resizes the line container if the index is out of range.
    /// </remarks>
    /// <param name="index">[in] Line index to modify.</param>
    /// <param name="text">[in] New text content for the line.</param>
    void set_line(int index, const std::string& text)
    {
        if (index >= (int)lines.size())
            lines.resize(index + 1);
        lines[index] = text;
    }

    /// <summary>
    ///     Clears all stored text lines.
    /// </summary>
    /// <remarks>
    ///     Removes all entries from the internal line container.
    /// </remarks>
    void clear()
    {
        lines.clear();
    }

    /// <summary>
    ///     Updates the widget state.
    /// </summary>
    /// <remarks>
    ///     This implementation is static and does not process any input.
    /// </remarks>
    /// <param name="mouse_x">[in] Mouse X position.</param>
    /// <param name="mouse_y">[in] Mouse Y position.</param>
    /// <param name="clicked">[in] Mouse click state.</param>
    void update(float mouse_x, float mouse_y, bool clicked) override
    {
        // static widget, no input
    }

    /// <summary>
    ///     Renders the UI widget including background and multi-line text.
    /// </summary>
    /// <remarks>
    ///     Draws a colored quad background (if enabled), then renders each text line
    ///     with configurable spacing and scale.
    /// </remarks>
    void render() override
    {
        if (!visible) return;

        // =========================
        // BACKGROUND
        // =========================
        if (ui_shader && background_color.a > 0.0f)
        {
            ui_shader->use();
            ui_shader->setVec4("uColor", background_color);

            float x = position.x;
            float y = position.y;
            float w = size.x;
            float h = size.y;

            float vertices[] =
            {
                x,     y,     0.0f,
                x + w, y,     0.0f,
                x + w, y + h, 0.0f,
                x,     y,     0.0f,
                x + w, y + h, 0.0f,
                x,     y + h, 0.0f
            };

            unsigned int VAO, VBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            glDeleteBuffers(1, &VBO);
            glDeleteVertexArrays(1, &VAO);
        }

        // =========================
        // TEXT
        // =========================
        if (text_renderer)
        {
            float y_offset = 0.0f;
            for (const auto& line : lines)
            {
                float text_x = position.x;
                float text_y = position.y + y_offset;
                text_renderer->render_text(line, text_x, text_y, text_scale);
                y_offset += line_spacing;
            }
        }
    }
};