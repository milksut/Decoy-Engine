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

    void set_background_color(glm::vec4 color) { background_color = color; }
    void set_text_scale(float s) { text_scale = s; }
    void set_line_spacing(float s) { line_spacing = s; }

    void set_line(int index, const std::string& text)
    {
        if (index >= (int)lines.size())
            lines.resize(index + 1);
        lines[index] = text;
    }

    void clear() { lines.clear(); }

    void update(float /*mouse_x*/, float /*mouse_y*/, bool /*clicked*/) override {}

    void render() override
    {
        if (!visible) return;

        if (ui_shader && background_color.a > 0.0f)
            draw_quad(ui_shader, background_color);

        if (text_renderer)
        {
            float y_offset = 0.0f;
            for (const auto& line : lines)
            {
                text_renderer->render_text(line, position.x, position.y + y_offset, text_scale);
                y_offset += line_spacing;
            }
        }
    }
};