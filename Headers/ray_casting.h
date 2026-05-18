#pragma once

#include <glm/glm.hpp>
#include <Camera_test.h>

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
        { 
            //LOG_ERROR("RayCasting: Ray-sphere intersection failed. No real intersection (discriminant < 0)");
            return -1.0f;
        }

        float sqrt_d = sqrt(discriminant);

        float t1 = (-b - sqrt_d) * 0.5f;
        float t2 = (-b + sqrt_d) * 0.5f;

        if (t1 > 0.0f) return t1;
        if (t2 > 0.0f) return t2;

        LOG_ERROR("RayCasting: No ray-sphere intersection. Discriminant < 0 (no real roots)");
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

    bool aabb_in_frustum(const Frustum& f, const AABB& world)
    {
        for (const auto& p : f.planes)
        {
            // Pick the corner most aligned with the plane normal
            glm::vec3 positive = {
                p.normal.x >= 0 ? world.max.x : world.min.x,
                p.normal.y >= 0 ? world.max.y : world.min.y,
                p.normal.z >= 0 ? world.max.z : world.min.z
            };

            if (glm::dot(p.normal, positive) + p.d < 0.0f)
                return false; // entirely outside this plane
        }
        return true;
    }

    Frustum extract_frustum(const glm::mat4& pv)
    {
        Frustum f;

        // Each plane is extracted from a row combination of the matrix
        // The math here is the Gribb/Hartmann method — standard and reliable
        f.planes[0] = { {pv[0][3] + pv[0][0], pv[1][3] + pv[1][0], pv[2][3] + pv[2][0]}, pv[3][3] + pv[3][0] }; // left
        f.planes[1] = { {pv[0][3] - pv[0][0], pv[1][3] - pv[1][0], pv[2][3] - pv[2][0]}, pv[3][3] - pv[3][0] }; // right
        f.planes[2] = { {pv[0][3] + pv[0][1], pv[1][3] + pv[1][1], pv[2][3] + pv[2][1]}, pv[3][3] + pv[3][1] }; // bottom
        f.planes[3] = { {pv[0][3] - pv[0][1], pv[1][3] - pv[1][1], pv[2][3] - pv[2][1]}, pv[3][3] - pv[3][1] }; // top
        f.planes[4] = { {pv[0][3] + pv[0][2], pv[1][3] + pv[1][2], pv[2][3] + pv[2][2]}, pv[3][3] + pv[3][2] }; // near
        f.planes[5] = { {pv[0][3] - pv[0][2], pv[1][3] - pv[1][2], pv[2][3] - pv[2][2]}, pv[3][3] - pv[3][2] }; // far

        // Normalize so the dot product test gives real distances
        for (auto& p : f.planes) {
            float len = glm::length(p.normal);
            p.normal /= len;
            p.d /= len;
        }
        return f;
    }

    Frustum extract_frustum(const glm::mat4& projection, const glm::mat4& view)
    {
        return extract_frustum(projection * view);
	}

    Frustum extract_frustum(const camera_test& cam)
    {
        return extract_frustum(cam.projection, cam.view);
	}

    static glm::vec3 ray_plane_intersection(
        const glm::vec3& ray_origin,
        const glm::vec3& ray_dir,
        float plane_y = 0.0f)
    {
        if (glm::abs(ray_dir.y) < 1e-6f)
            return glm::vec3(0.0f);

        float t = (plane_y - ray_origin.y) / ray_dir.y;

        if (t < 0.0f)
            return glm::vec3(0.0f);

        return ray_origin + t * ray_dir;
    }

}