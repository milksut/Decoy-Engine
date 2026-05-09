#pragma once

#include <glm/glm.hpp>
#include "Camera_test.h"

namespace Ray_casting
{

    /// <summary>
    ///     Converts screen-space mouse coordinates into a normalized world-space ray direction.
    /// </summary>
    /// <remarks>
    ///     Transforms coordinates from:
    ///     screen space -> NDC -> view space -> world space
    ///     using the camera projection and view matrices.
    /// </remarks>
    /// <param name="mouse_x">[in] Mouse X position in screen coordinates.</param>
    /// <param name="mouse_y">[in] Mouse Y position in screen coordinates.</param>
    /// <param name="screen_width">[in] Width of the screen/window.</param>
    /// <param name="screen_height">[in] Height of the screen/window.</param>
    /// <param name="camera">[in] Camera containing projection and view matrices.</param>
    /// <returns>Normalized world-space ray direction.</returns>
    glm::vec3 screen_to_world_ray(
        float mouse_x,
        float mouse_y,
        float screen_width,
        float screen_height,
        const camera_test& camera
    )
    {
        // Pixel to NDC
        float ndc_x = (2.0f * mouse_x) / screen_width - 1.0f;
        float ndc_y = 1.0f - (2.0f * mouse_y) / screen_height;

        // NDC to View space
        glm::vec4 clip = glm::vec4(ndc_x, ndc_y, -1.0f, 1.0f);
        glm::vec4 view_space = glm::inverse(camera.projection) * clip;
        view_space.z = -1.0f;
        view_space.w = 0.0f;

        // View space to World space
        glm::vec4 world = glm::inverse(camera.view) * view_space;

        return glm::normalize(glm::vec3(world));
    }
}