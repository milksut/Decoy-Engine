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
	void update_camera_vectors()
	{
		// Normalize orientation to prevent floating-point drift over time
		orientation = glm::normalize(orientation);

		// Rotate the base forward, right, and up vectors by the current orientation
		camera_front = glm::normalize(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
		camera_right = glm::normalize(orientation * glm::vec3(1.0f, 0.0f, 0.0f));
		camera_up = glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f));

		update_view_matrix();
	}

	unsigned int Camera_UBO;
	
public:
	glm::vec3 camera_right;
	glm::vec3 camera_position;
	glm::quat orientation;
	glm::vec3 camera_front;
	
	glm::vec3 camera_up;

	glm::vec3 world_up;

	glm::mat4 projection;
	glm::mat4 view;

	int Ubo_slot = -1;

	/// <summary>
	///     Constructs the camera and initializes its uniform buffer object (UBO).
	/// </summary>
	/// <remarks>
	///     Sets initial position, rotation, and world up vector.
	///     Creates and binds a UBO for storing camera matrices,
	///     assigns it to a free UBO slot, and initializes projection and view matrices.
	/// </remarks>
	/// <param name="position_in">[in] Initial camera position.</param>
	/// <param name="camera_angles_in">[in] Initial camera rotation angles (Euler).</param>
	/// <param name="world_up_in">[in] World up direction vector.</param>
	camera_test(
		glm::vec3 position_in = glm::vec3(0.0f, 0.0f, 3.0f),
		glm::quat camera_orientation_in = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec3 world_up_in = glm::vec3(0.0f, 1.0f, 0.0f))
		: camera_position(position_in), orientation(camera_orientation_in), world_up(world_up_in)
	{
		glGenBuffers(1, &Camera_UBO);
		glBindBuffer(GL_UNIFORM_BUFFER, Camera_UBO);
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(glm::mat4), glm::value_ptr(projection*view), GL_DYNAMIC_DRAW);
		
		Ubo_slot = Ubo_slots::get_first_empty_slot();
		Ubo_slots::bind_ubo_to_slot(Camera_UBO, Ubo_slot);

		LOG_INFO("UBO initialized. Camera UBO: %d bytes", sizeof(glm::mat4));
		
		update_camera_vectors();
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
		float near_plane = 0.1f, float far_plane = 10000.0f)
	{
		projection = glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
		glBindBuffer(GL_UNIFORM_BUFFER, Camera_UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection * view));
	}

	//TODO:fix param
	void process_mouse_movement(float xoffset, float yoffset, float sensitivity = 0.1f)
	{
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		glm::quat yaw_quat = glm::angleAxis(glm::radians(-xoffset), camera_up);

		glm::quat pitch_quat = glm::angleAxis(glm::radians(-yoffset), camera_right);

		orientation = orientation * yaw_quat * pitch_quat;

		update_camera_vectors();
	}

	//TODO:fix param
	void camera_tilt(float angle)
	{
		glm::quat roll_quat = glm::angleAxis(glm::radians(angle), camera_front);
		orientation = orientation * roll_quat;
		update_camera_vectors();
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

	//TODO:fix param
	void look_at(glm::vec3 target)
	{
		glm::vec3 direction = glm::normalize(target - camera_position);

		orientation = glm::quatLookAt(direction, world_up);

		update_camera_vectors();
	}
};

