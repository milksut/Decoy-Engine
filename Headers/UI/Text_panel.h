//#pragma once
//#include "Widget.h"
//#include "TextRenderer.h"
//#include "Shader.h"
//#include <vector>
//#include <string>
//
//class Text_panel : public Widget
//{
//private:
//    glm::vec4 background_color = { 0.0f, 0.0f, 0.0f, 0.0f };
//    // default: fully transparent
//
//public:
//    std::vector<std::string> lines;
//
//    Shader* ui_shader = nullptr;
//    TextRenderer* text_renderer = nullptr;
//
//    float line_spacing = 0.05f;
//
//    Text_panel(glm::vec2 position,
//        glm::vec2 size,
//        Shader* ui_shader,
//        TextRenderer* text_renderer)
//        : Widget(position, size),
//        ui_shader(ui_shader),
//        text_renderer(text_renderer)
//    {
//    }
//
//    void set_background_color(glm::vec4 color)
//    {
//        background_color = color;
//    }
//
//    void set_line(int index, const std::string& text)
//    {
//        if (index >= (int)lines.size())
//            lines.resize(index + 1);
//
//        lines[index] = text;
//    }
//
//    void clear()
//    {
//        lines.clear();
//    }
//
//    void update(float mouse_x, float mouse_y, bool clicked) override
//    {
//        // NO INPUT (static widget)
//    }
//
//    void render() override
//    {
//        if (!visible) return;
//
//        // =========================
//        // BACKGROUND (optional)
//        // =========================
//        if (ui_shader && background_color.a > 0.0f)
//        {
//            ui_shader->use();
//            ui_shader->setVec4("uColor", background_color);
//
//            float x = position.x;
//            float y = position.y;
//            float w = size.x;
//            float h = size.y;
//
//            glDrawArrays(GL_TRIANGLES, 0, 6);
//        }
//
//        // =========================
//        // TEXT
//        // =========================
//        if (text_renderer)
//        {
//            float y_offset = 0.0f;
//
//            for (const auto& line : lines)
//            {
//                float text_x = position.x;
//                float text_y = position.y + y_offset;
//
//                text_renderer->render_text(line, text_x, text_y, 1.0f);
//
//                y_offset += line_spacing * size.y;
//            }
//        }
//    }
//};