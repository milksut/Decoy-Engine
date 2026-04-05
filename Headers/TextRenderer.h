#pragma once
#include "Shader.h"
#include "Globals.h"

#include <glm/gtc/matrix_transform.hpp>


#include <fstream>

#include "Some_functions.h"
#include "Quad_renderer.h"

class TextRenderer
{
private:
	Shader shader;
	Quad_renderer *renderer;
	Texture texture_atlas;
	int char_width, char_height;
	float normalized_char_width, normalized_char_height;
	int screen_width, screen_height;
	float screen_char_width, screen_char_height, add_advance_per_char;

	
	int nrChannels, width, height;

	std::unordered_map<uint32_t, std::pair<float, float>> char_pos;

	static int get_ascii_code(const std::string& str)
	{
		size_t start = str.find(' '); // find first space
		if (start == std::string::npos) return -1; // no space found
		start++; // move past first space

		size_t fin = str.find(' ', start); // find next space after start
		if (fin == std::string::npos) return -1; // no second space found

		return std::stoi(str.substr(start, fin - start));
	}


public:
	glm::vec4 deleted_colors[8];
	float tolerances[8];
	glm::vec4 replace_colors[8];
	int num_color = 0;

	std::string last_rendered_text = "";

	TextRenderer(
		const char* texture_path, const char* char_set_path,
		const int screen_width, const int screen_height,
		const int char_width, const int char_height, 
		const char* vertex_shader_path, const char* fragment_shader_path, const char* geometry_shader_path,
		const float add_advance_per_char = 0.0f,const int image_packing = 4)
		: shader(vertex_shader_path, fragment_shader_path), char_width(char_width), char_height(char_height)
		  , screen_width(screen_width), screen_height(screen_height), add_advance_per_char(add_advance_per_char)
	{
		shader.add_geometry_shader(geometry_shader_path);

		renderer = new Quad_renderer(&shader);

		// Load texture atlas
		texture_atlas.id = load_image(texture_path, width, height, nrChannels, image_packing, false);
		texture_atlas.type = TextureType::DIFFUSE;
		texture_atlas.path = texture_path;
		Texture_slots::new_texture_loaded(texture_atlas);

		normalized_char_width = static_cast<float>(char_width) / width;
		normalized_char_height = static_cast<float>(char_height) / height;

		screen_char_width = (static_cast<float>(char_width) / screen_width) + add_advance_per_char;
		screen_char_height = static_cast<float>(char_height) / screen_height;


		int char_per_row = width / char_width;
		int char_per_col = height / char_height;

		char_pos.reserve(char_per_row * char_per_col);

		std::ifstream file(char_set_path); // Open the file
		if (!file.is_open())
		{
			LOG_FATAL("Could not open character set file");
			throw std::runtime_error("Could not open character set file");
		}

		int x = 0, y = 0;
		std::string line;
		while (std::getline(file, line)) // Read line by line
		{
			int ascii_code = get_ascii_code(line);

			if (ascii_code == -1)
			{
				LOG_ERROR("error on line: %d", x);
				throw std::runtime_error("Invalid line format");
			}
			char_pos[ascii_code] = {(float)(x * char_width) / width, (float)(y * char_height) / height};
			if (x >= char_per_row - 1)
			{
				x = 0;
				y++;
			}
			else
				x++;
		}

		file.close();
	}

	~TextRenderer()
	{
		Texture_slots::delete_texture(texture_atlas.id);
	}

	int change_deleted_colors(int index_of_color, const glm::vec4& new_deleted_color, float tolerance = 0.5f, const glm::vec4& new_replace_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f))
	{
		if (index_of_color < 0 || index_of_color > 7)
			return -1;
		if (index_of_color >= num_color)
			index_of_color = num_color++;

		deleted_colors[index_of_color] = new_deleted_color;
		tolerances[index_of_color] = tolerance;
		replace_colors[index_of_color] = new_replace_color;

		return index_of_color;
	}

	void push_deleted_colors()
	{
		shader.use();
		shader.setVec4("delete_colors", deleted_colors,num_color);
		shader.setFloat("tolerances", tolerances, num_color);
		shader.setVec4("replace_colors", replace_colors, num_color);
		shader.setInt("num_colors", num_color);
	}

	void change_screen_size(int new_width, int new_height)
	{
		screen_width = new_width;
		screen_height = new_height;
		screen_char_width = (static_cast<float>(char_width) / screen_width) + add_advance_per_char;
		screen_char_height = static_cast<float>(char_height) / screen_height;
	}

	void change_add_advance_per_char(float new_value)
	{
		add_advance_per_char = new_value;
		screen_char_width = (static_cast<float>(char_width) / screen_width) + add_advance_per_char;
		screen_char_height = static_cast<float>(char_height) / screen_height;
	}

	void render_text(const std::string& text, float starting_x, float starting_y, float scale_factor)
	{

		if (text.empty())
			return;

		if(text == last_rendered_text)
		{
			renderer->draw_last();
			return;
		}

		int text_size = text.size();

		const float add_advance_per_char_temp = add_advance_per_char <0 ? -1 * add_advance_per_char/2 : 0;

		std::vector<std::vector<float>> points;
		std::vector<std::vector<float>> tex_coords;
		points.reserve(text_size);
		tex_coords.reserve(text_size);

		for (uint32_t c : text)
		{
			auto map_pos = char_pos.find(c);
			if (map_pos == char_pos.end())
				continue; // Skip characters not in the map

			float tex_x = static_cast<float>(map_pos->second.first) + add_advance_per_char_temp;
			float tex_y = static_cast<float>(map_pos->second.second) + normalized_char_height;

			tex_coords.push_back({tex_x, tex_y});

			points.push_back({starting_x, starting_y, 0.0f});

			starting_x += screen_char_width * scale_factor;
		}
		shader.use();

		const float total_char_width = screen_char_width * scale_factor;
		shader.setFloat("width", total_char_width);

		const float total_char_height = screen_char_height * scale_factor;
		shader.setFloat("height", total_char_height);

		const float total_tex_width = normalized_char_width - add_advance_per_char_temp;
		shader.setFloat("tex_width", total_tex_width);

		const float total_tex_height = -normalized_char_height;
		shader.setFloat("tex_height", total_tex_height);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		renderer->draw(points,{texture_atlas},tex_coords);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
};

