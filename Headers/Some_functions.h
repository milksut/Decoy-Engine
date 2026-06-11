#pragma once

#include "Globals.h"

#include <iostream>
#include "stb_image.h"

//Todo:add param
//a helper function used to load images
unsigned int process_image(unsigned char* data, int& width, int& height, int& nrChannels, const int packing = 4)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (data)
	{
		//set the pixel storage mode "packing" byte alignment(default is 4 bytes)
		//we do this becosue the if image data is not aligned to 4 bytes it throw an error
		glPixelStorei(GL_UNPACK_ALIGNMENT, packing);

		GLenum format = GL_RGB;
		switch (nrChannels)
		{
		case 1:  format = GL_RED;   break;
		case 2:  format = GL_RG;    break;
		case 3:  format = GL_RGB;   break;
		case 4:  format = GL_RGBA;  break;
		default: /* error */        break;
		}

		//GL_TEXTURE_2D - specifies the target texture / 0 - minimap level / format - the type we store the texture 
		//width, height - dimensions of the texture / 0 - legacy, always 0 / format - format of the pixel data
		//GL_UNSIGNED_BYTE - data type of the pixel data / data - pointer to the pixel data
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
		return texture;
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
		stbi_image_free(data);
		return 0;
	}
}

/// <summary>
/// Loads an image from disk and creates an OpenGL 2D texture.
/// The image is optionally flipped vertically and uploaded to the GPU.
/// Texture parameters (wrapping and filtering) and mipmaps are configured automatically.
/// </summary>
/// <param name="image_path">[in] File path of the image to load</param>
/// <param name="width">[out] Width of the loaded image in pixels</param>
/// <param name="height">[out] Height of the loaded image in pixels</param>
/// <param name="nrChannels">[out] Number of color channels in the image (e.g., RGB = 3, RGBA = 4)</param>
/// <param name="packing">[in] Pixel alignment for OpenGL unpacking (default is 4 bytes)</param>
/// <param name="flip_vertically">[in] Whether to flip the image vertically during loading</param>
/// <returns>OpenGL texture ID if successful, 0 if loading fails</returns>
unsigned int load_texture_from_file(const char* image_path, int& width, int& height, int& nrChannels, const int packing = 4, const bool flip_vertically = true)
{

	//this is used to flip the image vertically when loading it becouse on OpenGL the origin is at the
	//bottom left corner and increases upwards, while in most image formats the origin is at the top left corner
	//and increases downwards
	stbi_set_flip_vertically_on_load(flip_vertically);
	// load and generate the texture
	unsigned char* data = stbi_load(image_path, &width, &height, &nrChannels, 0);
	unsigned int texture_id = process_image(data, width, height, nrChannels, packing);
	LOG_INFO("Image loaded: %s width: %d height: %d channels: %d", image_path, width, height, nrChannels);
	return texture_id;
}

//TODO: add param
unsigned int load_image_from_memory(const unsigned char* data, size_t data_size,
	int& width, int& height, int& nrChannels,
	int packing = 4, bool flip_vertically = true)
{
	stbi_set_flip_vertically_on_load(flip_vertically);
	unsigned char* decoded = stbi_load_from_memory(data, (int)data_size,&width, &height, &nrChannels, 0);

	if (!decoded)
		return 0;

	unsigned int texture_id = process_image(decoded, width, height, nrChannels, packing);

	LOG_INFO("Image loaded from memory: width: %d height: %d channels: %d", width, height, nrChannels);
	return texture_id;
}


