#pragma once

#include <glm/glm.hpp>

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
    glm::vec3  ScreenToWorldRay(float mouseX, float mouseY, unsigned int screenWidth, unsigned int screenHeight,
        const glm::mat4& projection, const glm::mat4& view)
    {
        // 1. NDC
        float x = (2.0f * mouseX) / screenWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / screenHeight;

        glm::vec4 rayClip(x, y, -1.0f, 1.0f);

        // 2. Eye space
        glm::mat4 invProj = glm::inverse(projection);
        glm::vec4 rayEye = invProj * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        // 3. World space
        glm::mat4 invView = glm::inverse(view);
        glm::vec3 rayDir = glm::normalize(glm::vec3(invView * rayEye));

        return rayDir;
    }

    /// <summary>
    ///     Computes ray-sphere intersection distance.
    /// </summary>
    /// <remarks>
    ///     Returns the nearest positive hit distance (t) if intersection exists,
    ///     otherwise returns -1.
    /// </remarks>
    /// <param name="ray_origin">[in] Origin of the ray.</param>
    /// <param name="ray_dir">[in] Normalized ray direction.</param>
    /// <param name="sphere_center">[in] Center of the sphere.</param>
    /// <param name="radius">[in] Sphere radius.</param>
    /// <returns>Distance along ray to intersection, or -1 if no hit.</returns>
    float ray_sphere_intersection(const glm::vec3& ray_origin, const glm::vec3& ray_dir,
        const glm::vec3& sphere_center, float radius)
    {
        glm::vec3 oc = ray_origin - sphere_center;

        float b = 2.0f * glm::dot(oc, ray_dir);
        float c = glm::dot(oc, oc) - radius * radius;

        float discriminant = b * b - 4.0f * c;

        if (discriminant < 0.0f)
            return -1.0f;

        float sqrt_d = sqrt(discriminant);

        float t1 = (-b - sqrt_d) * 0.5f;
        float t2 = (-b + sqrt_d) * 0.5f;

        if (t1 > 0.0f) return t1;
        if (t2 > 0.0f) return t2;

        return -1.0f;
    }

    /// <summary>
    ///     Checks if an object is inside a simplified view frustum (cone + distance test).
    /// </summary>
    /// <param name="camera_pos">[in] Camera world position.</param>
    /// <param name="camera_front">[in] Camera forward direction.</param>
    /// <param name="object_pos">[in] Object world position.</param>
    /// <param name="max_distance">[in] Maximum visible distance.</param>
    /// <param name="fov_cosine">[in] Precomputed cosine of half FOV angle.</param>
    /// <returns>True if object is within view cone and distance range.</returns>
    bool is_in_frustum(const glm::vec3& camera_pos,  const glm::vec3& camera_front, const glm::vec3& object_pos,
        float max_distance, float fov_cosine)
    {
        // Vector from camera to object
        glm::vec3 to_obj = object_pos - camera_pos;

        // Distance check
        float dist = glm::length(to_obj);
        if (dist > max_distance) return false;

        // Normalize direction
        glm::vec3 dir = glm::normalize(to_obj);

        // Dot product with camera front
        float dot = glm::dot(dir, glm::normalize(camera_front));

        // If dot is greater than cos(fov/2), object is inside view cone
        return dot > fov_cosine;
    }

}