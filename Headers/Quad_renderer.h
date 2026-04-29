#pragma once
#include "Shader.h"
#include "Globals.h"
#include "Some_functions.h"
#include <glad/glad.h>

struct QuadVertex {
    float position[3];
    float tex_coord[2];
};

class Quad_renderer
{
private:
    Shader* shader;

    unsigned int VAO = 0, VBO = 0;
    int last_point_count = 0;
    bool has_data = false;

    /// <summary>
    ///     Ensures VAO/VBO buffers exist and are sized correctly for quad vertex data.
    /// </summary>
    /// <remarks>
    ///     Allocates buffers if needed and configures vertex attribute layout.
    ///     Reallocates GPU memory only when vertex count changes.
    /// </remarks>
    /// <param name="point_count">[in] Number of vertices to allocate space for.</param>
    void ensure_buffers(int point_count)
    {
        if (VAO == 0)
        {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
        }

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Only reallocate if size changed
        if (point_count != last_point_count)
        {
            glBufferData(GL_ARRAY_BUFFER,
                point_count * sizeof(QuadVertex),
                nullptr,
                GL_DYNAMIC_DRAW);
            last_point_count = point_count;
        }

        // position: location 0, vec3
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(QuadVertex),
            (void*)offsetof(QuadVertex, position));

        // tex_coord: location 1, vec2
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
            sizeof(QuadVertex),
            (void*)offsetof(QuadVertex, tex_coord));
    }

    /// <summary>
    ///     Binds textures and updates shader sampler arrays for rendering.
    /// </summary>
    /// <remarks>
    ///     Ensures textures are assigned to GPU slots and grouped by type.
    ///     Sends TEX_COUNTS and per-type texture slot arrays to the shader.
    ///     Quad renderer does not use materials (material_index = -1).
    /// </remarks>
    /// <param name="main_textures">[in] List of textures used for rendering.</param>
    void bind_textures(const std::vector<Texture>& main_textures)
    {
        // Bind textures to slots first
        for (int i = 0; i < TEXTURE_SLOTS && i < (int)main_textures.size(); i++)
        {
            Texture_slots::bound_texture(main_textures[i].id); // ensures it's in a slot
        }

        std::array<std::vector<int>, Tex_type_amount> texture_locations;
        for (std::vector<int>& var : texture_locations)
        {
           var.reserve(TEXTURE_SLOTS);
        }

        int counts[Tex_type_amount] = { 0 };

        for (int i = 0; i < TEXTURE_SLOTS && i < (int)main_textures.size(); i++)
        {
            int slot_index = Texture_slots::bound_texture(main_textures[i].id);
            counts[main_textures[i].type]++;
            texture_locations[main_textures[i].type].push_back(slot_index);
        }

        shader->setInt("TEX_COUNTS", counts, Tex_type_amount);
        for (int i = 0; i < Tex_type_amount; i++)
        {
            if (counts[i] > 0)
                shader->setInt(Tex_Types_Names[i], texture_locations[i].data(), counts[i]);
        }

        // Quad renderer never has a material, so always -1
        shader->setInt("material_index", -1);
    }

public:
    Quad_renderer(Shader* shader) : shader(shader) {}

    ~Quad_renderer()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

    // No copy
    Quad_renderer(const Quad_renderer&) = delete;
    Quad_renderer& operator=(const Quad_renderer&) = delete;

    /// <summary>
    ///     Draws a dynamic quad/point set using CPU-generated vertex data.
    /// </summary>
    /// <remarks>
    ///     Builds interleaved vertex buffer (position + texcoord), uploads it to GPU,
    ///     binds textures, and renders using GL_POINTS.
    ///     Performs validation on input sizes and uses a dynamic buffer strategy.
    /// </remarks>
    /// <param name="points">[in] List of vertex positions (vec3 per vertex).</param>
    /// <param name="textures">[in] Textures used for rendering.</param>
    /// <param name="tex_coords">[in] Optional texture coordinates (vec2 per vertex).</param>
    void draw(const std::vector<std::vector<float>>& points,
        const std::vector<Texture>& textures,
        const std::vector<std::vector<float>>& tex_coords = {})
    {
        if (points.empty()) return;

        if (!tex_coords.empty() && tex_coords.size() != points.size())
        {
            LOG_FATAL("Quad_renderer: points and tex_coords size mismatch");
            throw std::runtime_error("Quad_renderer: size mismatch");
        }

        int count = (int)points.size();
        

        // Build interleaved vertex data
        std::vector<QuadVertex> verts(count);
        for (int i = 0; i < count; i++)
        {
            if (points[i].size() != 3)
            {
                LOG_FATAL("Quad_renderer: point must be vec3");
                throw std::runtime_error("Quad_renderer: point must be vec3");
            }
            verts[i].position[0] = points[i][0];
            verts[i].position[1] = points[i][1];
            verts[i].position[2] = points[i][2];

            if (!tex_coords.empty())
            {
                if (tex_coords[i].size() != 2)
                {
                    LOG_FATAL("Quad_renderer: tex_coord must be vec2");
                    throw std::runtime_error("Quad_renderer: tex_coord must be vec2");
                }
                verts[i].tex_coord[0] = tex_coords[i][0];
                verts[i].tex_coord[1] = tex_coords[i][1];
            }
            else
            {
                verts[i].tex_coord[0] = 0.0f;
                verts[i].tex_coord[1] = 0.0f;
            }
        }
        ensure_buffers(count);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            count * sizeof(QuadVertex),
            verts.data());

        bind_textures(textures);
        glDrawArrays(GL_POINTS, 0, count);;

        has_data = true;
    }

    /// <summary>
    ///     Draws the last uploaded quad/point data without re-uploading buffers.
    /// </summary>
    /// <remarks>
    ///     Uses previously stored VAO and last vertex count for rendering.
    ///     Intended for repeated draws of static/dynamic data.
    /// </remarks>
    void draw_last()
    {
        if (!has_data || VAO == 0)
        {
            LOG_WARNING("Quad_renderer: no mesh to draw");
            return;
        }
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, last_point_count);
        glBindVertexArray(0);
    }
};