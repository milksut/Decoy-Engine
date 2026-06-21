#pragma once
#include "Widget.h"
#include "Shader.h"

class Bar : public Widget
{
public:
    float value = 1.0f;
    float min_value = 0.0f;
    float max_value = 1.0f;

    glm::vec4 background_color = { 0.2f, 0.2f, 0.2f, 0.8f };
    glm::vec4 fill_color = { 0.1f, 0.8f, 0.1f, 1.0f };
    glm::vec4 border_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    float     border_thickness = 0.002f;

    enum class Direction { LEFT_TO_RIGHT, RIGHT_TO_LEFT, BOTTOM_TO_TOP, TOP_TO_BOTTOM };
    Direction direction = Direction::LEFT_TO_RIGHT;

    Bar(glm::vec2 pos, glm::vec2 size, Shader* shader)
        : Widget(pos, size), m_shader(shader)
    {
    }

    /// <summary>
    ///     Sets the current value, clamped between min_value and max_value.
    /// </summary>
    /// <param name="v">[in] New value.</param>
    void set_value(float v)
    {
        value = glm::clamp(v, min_value, max_value);
    }

    /// <summary>
    ///     Sets the min/max range and clamps the current value into it.
    /// </summary>
    /// <param name="mn">[in] Minimum value.</param>
    /// <param name="mx">[in] Maximum value.</param>
    void set_range(float mn, float mx)
    {
        min_value = mn;
        max_value = mx;
        value = glm::clamp(value, min_value, max_value);
    }

    /// <summary>
    ///     Returns the fill ratio in [0, 1].
    /// </summary>
    float get_ratio() const
    {
        if (max_value <= min_value) return 0.0f;
        return (value - min_value) / (max_value - min_value);
    }

    void update(float /*mouse_x*/, float /*mouse_y*/, bool /*clicked*/) override {}

    void render() override
    {
        if (!visible) return;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        draw_quad(m_shader, background_color);

        float ratio = get_ratio();
        if (ratio > 0.0f)
        {
            float x = position.x;
            float y = position.y;
            float w = size.x;
            float h = size.y;

            float fx = x, fy = y, fw = w, fh = h;

            switch (direction)
            {
            case Direction::LEFT_TO_RIGHT:  fw = w * ratio; break;
            case Direction::RIGHT_TO_LEFT:  fx = x + w * (1.0f - ratio); fw = w * ratio; break;
            case Direction::BOTTOM_TO_TOP:  fy = y + h * (1.0f - ratio); fh = h * ratio; break;
            case Direction::TOP_TO_BOTTOM:  fh = h * ratio; break;
            }

            draw_quad_raw(m_shader, fill_color, fx, fy, fw, fh);
        }

        if (border_thickness > 0.0f)
        {
            float x = position.x;
            float y = position.y;
            float w = size.x;
            float h = size.y;
            float t = border_thickness;

            draw_quad_raw(m_shader, border_color, x, y, t, h);
            draw_quad_raw(m_shader, border_color, x + w - t, y, t, h);
            draw_quad_raw(m_shader, border_color, x, y, w, t);
            draw_quad_raw(m_shader, border_color, x, y + h - t, w, t);
        }

        glEnable(GL_DEPTH_TEST);
    }

private:
    Shader* m_shader;

    void draw_quad_raw(Shader* shader, const glm::vec4& color,
        float x, float y, float w, float h)
    {
        shader->use();
        shader->setVec4("uColor", color);

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
};