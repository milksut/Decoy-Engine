#pragma once

#include "Globals.h"

#include <glm/gtc/matrix_transform.hpp>
class camera_test
{
private:

	
	void update_camera_vectors(bool front_change, bool right_change, bool up_change)
	{
		if (front_change)
		{
			camera_front = glm::vec3(
				cos(glm::radians(camera_angles.x)) * cos(glm::radians(camera_angles.y)),	//camera_front.x
				sin(glm::radians(camera_angles.y)),											//camera_front.y
				sin(glm::radians(camera_angles.x)) * cos(glm::radians(camera_angles.y))		//camera_front.z
			);
			camera_front = glm::normalize(camera_front);
		}

		if(right_change)
		{
			if(!up_change)
			{
				camera_right = glm::normalize(glm::cross(camera_front, camera_up));
			}
			else
			{
				camera_right = glm::normalize(glm::cross(camera_front, world_up));
				glm::mat4 view = glm::mat4(1.0f);
				view = glm::rotate(view, glm::radians(camera_angles.z ), camera_front);
				camera_right = glm::normalize(view * glm::vec4(camera_right, 0.0f));
			}
		}

		if(up_change)
		{
			camera_up = glm::normalize(glm::cross(camera_right, camera_front));
		}
		update_view_matrix();
	}


public:
	glm::vec3 camera_right;
	glm::vec3 camera_position;
	glm::vec3 camera_angles;
	glm::vec3 camera_front;
	
	glm::vec3 camera_up;

	glm::vec3 world_up;

	glm::mat4 projection;
	glm::mat4 view;

	camera_test(
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3 camera_angles = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f))
		: camera_position(position), camera_angles(camera_angles), world_up(world_up)
	{
		update_camera_vectors(true,true,true);
		update_projection();
		update_view_matrix();
	}

	void update_projection(
		float fov = 45.0f, float aspect_ratio = 800.0f / 600.0f,
		float near_plane = 0.1f, float far_plane = 100.0f)
	{
		projection = glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
	}

	bool flip = false;
	void process_mouse_movement(float xoffset, float yoffset, float sensitivity = 0.1f)
	{
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		camera_angles.x += xoffset * cos(glm::radians(camera_angles.z))
			- yoffset * sin(glm::radians(camera_angles.z));

		camera_angles.y -= (flip ? -1:1) *( xoffset * sin(glm::radians(camera_angles.z))
			+ yoffset * cos(glm::radians(camera_angles.z)));


		if (camera_angles.x >= 360.0f)
			camera_angles.x -= 360.0f;

		if (camera_angles.x < 0.0f)
			camera_angles.x += 360.0f;


		if (camera_angles.y >= 360.0f)
			camera_angles.y -= 360.0f;

		if (camera_angles.y < 0.0f)
			camera_angles.y += 360.0f;


		if (camera_angles.y >= 90.0f && camera_angles.y <= 270.0f)
		{
			if(!flip)
			{
				camera_angles.z += 180.0f; flip = true;
			}
		}
		else
		{
			if(flip)
			{
				camera_angles.z += 180.0f; flip = false;
			}
		}

		if (camera_angles.z >= 360.0f)
			camera_angles.z -= 360.0f;

		if (camera_angles.z < 0.0f)
			camera_angles.z += 360.0f;

		//update_camera_vectors(xoffset||yoffset,xoffset,yoffset);
		update_camera_vectors(true,true,true);
	}

	void camera_tilt(float angle)
	{
		camera_angles.z += angle;
		if (camera_angles.z >= 360.0f)
			camera_angles.z -= 360.0f;
		if (camera_angles.z < 0.0f)
			camera_angles.z += 360.0f;
		update_camera_vectors(false, true, true);
	}

	void update_camera_position(glm::vec3 position)
	{
		camera_position = position;
		update_view_matrix();
	}

	void camera_move(
		float delta_time, float speed = 2.5f,
		bool move_forward = false, bool move_backward = false,
		bool move_left = false, bool move_right = false,
		bool move_up = false, bool move_down = false)
	{
		float velocity = speed * delta_time;
		if (move_forward)
			camera_position += camera_front * velocity;
		if (move_backward)
			camera_position -= camera_front * velocity;
		if (move_left)
			camera_position -= camera_right * velocity;
		if (move_right)
			camera_position += camera_right * velocity;
		if (move_up)
			camera_position += camera_up * velocity;
		if (move_down)
			camera_position -= camera_up * velocity;

		update_view_matrix();
	}

	void update_view_matrix()
	{
		view = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
	}
};

