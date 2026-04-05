#pragma once

#include "Shader.h"
#include "Globals.h"
#include "Some_functions.h"

#include "game_object_basic_model.h"

class Quad_renderer
{
private:
	//model is only used for its methods,
	//we are gonna give point list as vertices and create shapes using the geometry shader
	game_object_basic_model quad_model;
	Shader *shader;
	std::shared_ptr<class_region> region;

	//quad-shape
	float total_width = 0.0f;
	float total_height = 0.0f;

	//texture-shape
	float tex_total_width = 0.0f;
	float tex_total_height = 0.0f;

public:
	Quad_renderer(Shader *shader)
		:shader(shader)
	{}

	void draw(const std::vector<std::vector<float>>& points, const std::vector<Texture>& textures)
	{
		const std::vector<std::vector<float>> empty;
		draw(points, textures, empty);
	}

	void draw(const std::vector<std::vector<float>> points,const std::vector<Texture>& textures,
		const std::vector<std::vector<float>>& tex_coord)
	{
		if(!tex_coord.empty() && tex_coord.size() != points.size())
		{
			LOG_FATAL("Quad_renderer: Recived point and tex_coords is not the same size");
			throw std::runtime_error("Quad_renderer: Recived point and tex_coords is not the same size");
		}
		std::vector<vertex_data> data;
		data.reserve(points.size());
		for(int i = 0; i< points.size(); i++)
		{

			if(points[i].size() != 3)
			{
				LOG_FATAL("Quad_renderer: Recived points is not expeted shape");
				throw std::runtime_error("Quad_renderer: Recived points is not expeted shape");
			}
			float tex_coord_temp[2] = { 0,0 };

			if(!tex_coord.empty())
			{
				if(tex_coord[i].size() != 2)
				{
					LOG_FATAL("Quad_renderer: Recived tex_coords is not expeted shape");
					throw std::runtime_error("Quad_renderer: Recived tex_coords is not expeted shape");
				}
				else
				{
					tex_coord_temp[0] = tex_coord[i][0];
					tex_coord_temp[1] = tex_coord[i][1];

				}
			}
			
			vertex_data point_data =
			{
				{points[i][0],points[i][1],points[i][2]},
				{tex_coord_temp[0],tex_coord_temp[1]},
				{0,0,1}
			};
			data.push_back(point_data);
		}
		std::vector<unsigned int> empty;

		if(quad_model.Meshes.empty())
		{
			quad_model.add_mesh(data, empty, textures);
			region = quad_model.reserve_class_region(1);
		}
		else
		{
			quad_model.Meshes[0]->update_mesh(data, empty, textures, true);
		}

		quad_model.draw(*shader, region);

	}
	
	void draw_last()
	{
		if(quad_model.Meshes.empty())
		{
			LOG_WARNING("Quad_renderer: No mesh to draw");
			return;
		}
		quad_model.draw(*shader, region);
	}

};