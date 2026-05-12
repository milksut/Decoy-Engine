#pragma once

#include "Globals.h"

class camera_test
{
private:

	
	/// <summary>
	/// Updates the camera direction vectors (front, right, up) based on the provided flags and camera angles.
	/// Recalculates the view matrix after updating vectors.
	/// </summary>
	/// <param name="front_change">[in] If true, recalculates the front vector of the camera.</param>
	/// <param name="right_change">[in] If true, recalculates the right vector of the camera.</param>
	/// <param name="up_change">[in] If true, recalculates the up vector of the camera.</param>
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
				glm::mat4 rotationMat = glm::mat4(1.0f);
				rotationMat = glm::rotate(rotationMat, glm::radians(camera_angles.z), camera_front);
				camera_right = glm::normalize(rotationMat * glm::vec4(camera_right, 0.0f));
			}
		}

		if(up_change)
		{
			camera_up = glm::normalize(glm::cross(camera_right, camera_front));
		}
		update_view_matrix();
	}

	unsigned int Camera_UBO;
	
public:
	glm::vec3 camera_right;
	glm::vec3 camera_position;
	glm::vec3 camera_angles;
	glm::vec3 camera_front;
	
	glm::vec3 camera_up;

	glm::vec3 world_up;

	glm::mat4 projection;
	glm::mat4 view;

	int Ubo_slot = -1;
	camera_test(
		glm::vec3 position_in = glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3 camera_angles_in = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 world_up_in = glm::vec3(0.0f, 1.0f, 0.0f))
		: camera_position(position_in), camera_angles(camera_angles_in), world_up(world_up_in)
	{
		glGenBuffers(1, &Camera_UBO);
		glBindBuffer(GL_UNIFORM_BUFFER, Camera_UBO);
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(glm::mat4), glm::value_ptr(projection*view), GL_DYNAMIC_DRAW);
		
		Ubo_slot = Ubo_slots::get_first_empty_slot();
		Ubo_slots::bind_ubo_to_slot(Camera_UBO, Ubo_slot);

		LOG_INFO("UBO initialized. Camera UBO: %d bytes", sizeof(glm::mat4));
		
		update_camera_vectors(true, true, true);
		update_projection();
		update_view_matrix();
	}


	/// <summary>
	/// Updates the camera's projection matrix using perspective projection based on the provided parameters.
	/// </summary>
	/// <param name="fov">[in] Field of view in degrees. Default is 45.0f.</param>
	/// <param name="aspect_ratio">[in] Aspect ratio of the screen (width / height). Default is 800.0f / 600.0f.</param>
	/// <param name="near_plane">[in] Near clipping plane distance. Default is 0.1f.</param>
	/// <param name="far_plane">[in] Far clipping plane distance. Default is 100.0f.</param>
	void update_projection(
		float fov = 45.0f, float aspect_ratio = 800.0f / 600.0f,
		float near_plane = 0.1f, float far_plane = 100.0f)
	{
		projection = glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
		glBindBuffer(GL_UNIFORM_BUFFER, Camera_UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection * view));
	}

	bool flip = false;

	/// <summary>
	/// Processes mouse movement input and updates the camera's angles (pitch, yaw, roll) accordingly.
	/// Updates the camera vectors after applying the changes.
	/// </summary>
	/// <param name="xoffset">[in] The horizontal mouse movement offset.</param>
	/// <param name="yoffset">[in] The vertical mouse movement offset.</param>
	/// <param name="sensitivity">[in] Sensitivity factor for the mouse movement. Default is 0.1f.</param>
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

	/// <summary>
	/// Tilts (rolls) the camera around its front axis by the given angle.
	/// Updates the right and up vectors after applying the tilt.
	/// </summary>
	/// <param name="angle">[in] The angle in degrees to tilt the camera. Positive values tilt clockwise.</param>
	void camera_tilt(float angle)
	{
		camera_angles.z += angle;
		if (camera_angles.z >= 360.0f)
			camera_angles.z -= 360.0f;
		if (camera_angles.z < 0.0f)
			camera_angles.z += 360.0f;
		update_camera_vectors(false, true, true);
	}

	/// <summary>
	/// Updates the camera's world position to the specified coordinates.
	/// Automatically updates the view matrix after changing the position.
	/// </summary>
	/// <param name="position">[in] The new position of the camera as a glm::vec3 (x, y, z).</param>
	void update_camera_position(glm::vec3 position)
	{
		camera_position = position;
		update_view_matrix();
	}

	/// <summary>
	/// Moves the camera in 3D space based on the input movement flags and speed.
	/// Updates the view matrix after applying the movement.
	/// </summary>
	/// <param name="delta_time">[in] Time elapsed since the last frame, used to scale movement speed.</param>
	/// <param name="speed">[in] Movement speed multiplier. Default is 2.5f.</param>
	/// <param name="move_forward">[in] If true, moves the camera forward along the front vector.</param>
	/// <param name="move_backward">[in] If true, moves the camera backward along the front vector.</param>
	/// <param name="move_left">[in] If true, moves the camera left along the right vector.</param>
	/// <param name="move_right">[in] If true, moves the camera right along the right vector.</param>
	/// <param name="move_up">[in] If true, moves the camera up along the up vector.</param>
	/// <param name="move_down">[in] If true, moves the camera down along the up vector.</param>
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

	/// <summary>
	/// Updates the camera's view matrix based on its current position, front, and up vectors.
	/// This matrix is used for rendering the scene from the camera's perspective.
	/// </summary>
	void update_view_matrix()
	{
		view = glm::lookAt(camera_position, camera_position + camera_front, camera_up);
		glBindBuffer(GL_UNIFORM_BUFFER, Camera_UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection * view));
	}
};

