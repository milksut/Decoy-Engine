#pragma once
#include "Shader.h"
#include "Globals.h"

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

	/// <summary>
	/// Extracts the ASCII code from a string formatted with two spaces around the number.
	/// For example, given "Key 65 Pressed", it will return 65.
	/// Returns -1 if the string does not contain exactly two spaces.
	/// </summary>
	/// <param name="str">[in] Input string containing the ASCII code between two spaces</param>
	/// <returns>
	/// The ASCII code as an integer if found; -1 if the expected format is not met
	/// </returns>
	static int get_ascii_code(const std::string& str)
	{
		size_t start = str.find(' '); // find first space
		if (start == std::string::npos)
		{
			LOG_ERROR("StringParser: No space character found in input string");
			return -1; // no space found
		}
		start++; // move past first space

		size_t fin = str.find(' ', start); // find next space after start
		if (fin == std::string::npos)
		{
			LOG_ERROR("StringParser: Second space not found after start position");
			return -1; // no second space found
		}
		return std::stoi(str.substr(start, fin - start));
	}


public:
	glm::vec4 deleted_colors[8] = {};
	float tolerances[8] = {};
	glm::vec4 replace_colors[8] = {};
	int num_color = 0;

	std::string last_rendered_text = "";


	/// <summary>
	/// Constructs a TextRenderer object for rendering text using a texture atlas.
	/// Loads the texture, character set, and sets up rendering parameters including screen and character sizes.
	/// Also initializes the shader with optional geometry shader and prepares character positions for rendering.
	/// </summary>
	/// <param name="texture_path">[in] Path to the texture atlas image</param>
	/// <param name="char_set_path">[in] Path to the character set file</param>
	/// <param name="screen_width">[in] Width of the screen in pixels</param>
	/// <param name="screen_height">[in] Height of the screen in pixels</param>
	/// <param name="char_width">[in] Width of a single character in the texture atlas</param>
	/// <param name="char_height">[in] Height of a single character in the texture atlas</param>
	/// <param name="vertex_shader_path">[in] File path to the vertex shader</param>
	/// <param name="fragment_shader_path">[in] File path to the fragment shader</param>
	/// <param name="geometry_shader_path">[in] File path to the geometry shader</param>
	/// <param name="add_advance_per_char">[in] Extra horizontal spacing added per character (default 0.0f)</param>
	/// <param name="image_packing">[in] Pixel alignment for the texture image (default 4)</param>
	TextRenderer(
		const char* texture_path, const char* char_set_path,
		const int screen_width_in, const int screen_height_in,
		const int char_width_in, const int char_height_in, 
		const char* vertex_shader_path, const char* fragment_shader_path, const char* geometry_shader_path,
		const float add_advance_per_char_in = 0.0f,const int image_packing = 4)
		: shader(vertex_shader_path, fragment_shader_path), char_width(char_width_in), char_height(char_height_in)
		  , screen_width(screen_width_in), screen_height(screen_height_in), add_advance_per_char(add_advance_per_char_in)
	{
		shader.add_geometry_shader(geometry_shader_path);

		renderer = new Quad_renderer(&shader);

		// Load texture atlas
		texture_atlas.id = load_texture_from_file(texture_path, width, height, nrChannels, image_packing, false);
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

	/// <summary>
	/// Destructor for TextRenderer.
	/// Cleans up the texture loaded into GPU memory by deleting it from Texture_slots.
	/// </summary>
	~TextRenderer()
	{
		Texture_slots::delete_texture(texture_atlas.id);
	}

	/// <summary>
	/// Updates a deleted color slot with a new deleted color, tolerance, and replacement color.
	/// </summary>
	/// <param name="index_of_color">[in] Index of the slot to update in the deleted colors list (0-7)</param>
	/// <param name="new_deleted_color">[in] The color that will be marked as deleted (stored in deleted_colors array)</param>
	/// <param name="tolerance">[in] Matching tolerance for deletion; how close a color needs to be to be considered deleted (default 0.5f)</param>
	/// <param name="new_replace_color">[in] Color to replace the deleted color with (stored in replace_colors array, default transparent white)</param>
	/// <returns>
	/// The index of the updated color slot; returns -1 if the index is invalid
	/// </returns>
	int change_deleted_colors(int index_of_color, const glm::vec4& new_deleted_color, float tolerance = 0.5f, const glm::vec4& new_replace_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f))
	{
		if (index_of_color < 0 || index_of_color > 7)
		{
			LOG_ERROR("ColorManager: Invalid color index. Must be in range [0, 7]");
			return -1;
		}
		if (index_of_color >= num_color)
			index_of_color = num_color++;

		deleted_colors[index_of_color] = new_deleted_color;
		tolerances[index_of_color] = tolerance;
		replace_colors[index_of_color] = new_replace_color;

		return index_of_color;
	}

	/// <summary>
	/// Sends the current deleted colors, replacement colors, and tolerances to the GPU shader.
	/// </summary>
	void push_deleted_colors()
	{
		shader.use();
		shader.setVec4("delete_colors", deleted_colors,num_color);
		shader.setFloat("tolerances", tolerances, num_color);
		shader.setVec4("replace_colors", replace_colors, num_color);
		shader.setInt("num_colors", num_color);
	}

	/// <summary>
	/// Updates the screen dimensions and recalculates the normalized character width and height for rendering.
	/// </summary>
	/// <param name="new_width">[in] The new width of the screen in pixels.</param>
	/// <param name="new_height">[in] The new height of the screen in pixels.</param>
	void change_screen_size(int new_width, int new_height)
	{
		screen_width = new_width;
		screen_height = new_height;
		screen_char_width = (static_cast<float>(char_width) / screen_width) + add_advance_per_char;
		screen_char_height = static_cast<float>(char_height) / screen_height;
	}

	/// <summary>
	/// Updates the additional advance per character and recalculates the normalized character width and height for rendering.
	/// </summary>
	/// <param name="new_value">[in] The new value to add to the horizontal advance of each character.</param>
	void change_add_advance_per_char(float new_value)
	{
		add_advance_per_char = new_value;
		screen_char_width = (static_cast<float>(char_width) / screen_width) + add_advance_per_char;
		screen_char_height = static_cast<float>(char_height) / screen_height;
	}

	/// <summary>
	/// Renders the given text on the screen at the specified starting coordinates with the provided scale factor.
	/// It calculates positions and texture coordinates for each character and sends them to the renderer.
	/// </summary>
	/// <param name="text">[in] The string of text to render.</param>
	/// <param name="starting_x">[in] The x-coordinate on the screen where rendering starts.</param>
	/// <param name="starting_y">[in] The y-coordinate on the screen where rendering starts.</param>
	/// <param name="scale_factor">[in] The scaling factor to apply to the character size.</param>
	void render_text(const std::string& text, float starting_x, float starting_y, float scale_factor)
	{

		if (text.empty())
			return;

		if(text == last_rendered_text)
		{
			renderer->draw_last();
			return;
		}

		int text_size = (int)text.size();

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

	/// <summary>
	///     Calculates the on-screen size of a text string.
	/// </summary>
	/// <param name="text">[in] Text to measure.</param>
	/// <param name="scale">[in] Scale factor applied to character size.</param>
	/// <returns>Width and height of rendered text in screen space.</returns>
	glm::vec2 get_text_size(const std::string& text, float scale)
	{
		float char_w = screen_char_width * scale;
		float char_h = screen_char_height * scale;

		return {
			text.size() * char_w,
			char_h
		};
	}
};

