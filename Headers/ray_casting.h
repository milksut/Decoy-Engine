#pragma once

#include <glm/glm.hpp>

namespace Ray_casting
{
    glm::vec2 mouse_to_ndc(float mouse_x, float mouse_y,
        float screen_width, float screen_height)
    {
        float ndc_x = (2.0f * mouse_x) / screen_width - 1.0f;

        float ndc_y = 1.0f - (2.0f * mouse_y) / screen_height;

        return glm::vec2(ndc_x, ndc_y);
    }

    glm::vec3 screen_to_world_ray(float mouse_x, float mouse_y,
        float screen_width, float screen_height,
        const glm::mat4& projection, const glm::mat4& view)
    {
        glm::vec2 ndc = mouse_to_ndc(mouse_x, mouse_y, screen_width, screen_height);

        glm::vec4 clip = glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);

        glm::vec4 view_space = glm::inverse(projection) * clip;

        view_space.z = -1.0f;
        view_space.w = 0.0f;

        glm::vec4 world = glm::inverse(view) * view_space;

        return glm::normalize(glm::vec3(world));
    }
}