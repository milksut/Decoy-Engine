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

    void draw_quad(Shader* shader, const glm::vec4& color)
    {
        shader->use();
        shader->setVec4("uColor", color);

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
};