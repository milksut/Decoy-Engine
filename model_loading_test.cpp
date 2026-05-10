#include "Camera_test.h"
#include "game_object_basic_model.h"
#include "Globals.h"
#include "Shader.h"
#include "Some_functions.h"
#include "TextRenderer.h"

#include "Input_Manager.h"
#include "The_event_manager.h"
#include "Headers/game_object_basic.h"

bool camera_control = false;

const double Target_fps = 144;
const double Target_frame_time = 1.0 / Target_fps;
const bool enable_vSync = false;

const unsigned int width = 800, height = 600;
const float aspect_ratio = (float)width / (float)height;


Event_manager manager;
Input_Manager* input_manager;



class Tree : public game_object_basic<Tree> {
public:
	Tree(entt::registry& reg, const std::string& tag, game_object_basic* parent = nullptr)
		: game_object_basic(reg, tag, parent) {
	}
};

TextRenderer* printer;
camera_test* camera;
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	//// make sure the viewport matches the new window dimensions; note that width and 
	//// height will be significantly larger than specified on retina displays.
	const float new_aspect_ratio = (float)width / (float)height;
	//if (new_aspect_ratio < aspect_ratio)
	//{
	//	//if the new aspect ratio is smaller than the original one, we need to adjust the viewport
	//	int new_height = (int)(width / aspect_ratio);
	//	glViewport(0, (height - new_height) / 2, width, new_height);
	//}
	//else
	//{
	//	//if the new aspect ratio is larger than the original one, we need to adjust the viewport
	//	int new_width = (int)(height * aspect_ratio);
	//	glViewport((width - new_width) / 2, 0, new_width, height);
	//}
	glViewport(0, 0, width, height);

	camera->update_projection(45.0f, new_aspect_ratio, 0.1f, 1000.0f);
	printer->change_screen_size(width, height);
}

bool pressable = true;
void processInput(GLFWwindow* window, float camera_speed, camera_test* camera)
{

	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && pressable)
	{
		static bool is_fullscreen = false;
		static int windowed_xpos = 100, windowed_ypos = 100, windowed_width = 800, windowed_height = 600;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (!is_fullscreen)
		{
			glfwGetWindowPos(window, &windowed_xpos, &windowed_ypos);
			glfwGetWindowSize(window, &windowed_width, &windowed_height);
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			is_fullscreen = true;
		}
		else
		{
			glfwSetWindowMonitor(window, nullptr, windowed_xpos, windowed_ypos, windowed_width, windowed_height, 0);
			is_fullscreen = false;
		}
		pressable = false;
	}
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE)
	{
		pressable = true;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera_speed *= 2;

	bool changes[6] = { false, false, false, false, false, false };

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		changes[0] = true; // front change
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		changes[1] = true; // back change
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		changes[2] = true; // left change
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		changes[3] = true; // right change
	}


	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		changes[4] = true; // up change
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		changes[5] = true; // down change
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		camera->camera_tilt(-camera_speed * 5);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		camera->camera_tilt(camera_speed * 5);
	}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	camera->camera_move(
		1, camera_speed,
		changes[0], changes[1],
		changes[2], changes[3],
		changes[4], changes[5]);
}

int main()
{
	glfwInit();
	GLFWwindow* window = init_window(width, height, "Shader Tester");
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	if (!window) {
		return -1;
	}

	glfwSwapInterval(enable_vSync);
	camera = new camera_test(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	camera->update_projection(45.0f, aspect_ratio, 0.1f, 1000.0f);

	printer = new TextRenderer("Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.png",
		"Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.txt",
		width, height, 16, 32,
		"Shaders/Vertex_shaders/Text_render_vertex.vert",
		"Shaders/Fragment_shaders/Text_render_fragment.frag",
		"Shaders/Geometry_shaders/Text_render_geometry.geom",
		0.005f);

	printer->change_deleted_colors(0, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.5f, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
	printer->change_deleted_colors(1, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	printer->push_deleted_colors();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	Shader shader("Shaders/Vertex_shaders/Loaded_model_vertex.vert",
		"Shaders/Fragment_shaders/Loaded_model_fragment.frag");

	game_object_basic_model backpack;
	//backpack.import_model_from_file("C:\\Users\\altay\\Desktop\\pull_from_this_easy\\Backpack.obj");

	backpack.import_model_from_file("Models/Tree1.obj");
	
	//-*-*-*-*-*-*-*-**-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*-*
	int grid_amount = 10;
	//-*-*-*-*-*-*-*-**-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*-*


	std::shared_ptr<class_region> grid_region = backpack.reserve_class_region(grid_amount * grid_amount);
	
	backpack.add_instance_buffer(16, 3); //attrib size-mat4-16floats, attrib index, for model

	backpack.add_instance_buffer(9, 7); //attrib size-mat3-12floats, attrib index, for transpose_inverse_viewXmodel



	std::vector<glm::mat4> model_matrices_grid;
	model_matrices_grid.reserve(grid_amount * grid_amount);
	for (int i = 0; i < grid_amount; i++)
	{
		for (int j = 0; j < grid_amount; j++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(i * 5.0f, 5.0f, j * 5.0f));
			model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
			model_matrices_grid.push_back(model);
		}

	}
	
	grid_region->data_ptrs[3] = std::shared_ptr<float>((float*)model_matrices_grid.data(), [](float*) {});
	grid_region->data_amount[3] = model_matrices_grid.size() * 16;


	backpack.load_instance_buffer((float*)model_matrices_grid.data(), model_matrices_grid.size(), 3, grid_region);
	Logger::checkGLError("After loading models");

	std::vector<glm::mat3> transpose_inverse_model_matrices_grid;
	transpose_inverse_model_matrices_grid.reserve(grid_amount * grid_amount);
	for (int i = 0; i < grid_amount; i++)
	{
		for (int j = 0; j < grid_amount; j++)
		{
			glm::mat4 model = model_matrices_grid[i * grid_amount + j];
			glm::mat3 transpose_inverse_model = glm::transpose(glm::inverse(glm::mat3(model)));
			transpose_inverse_model_matrices_grid.push_back(transpose_inverse_model);
		}
	}

	grid_region->data_ptrs[7] = std::shared_ptr<float>((float*)transpose_inverse_model_matrices_grid.data(), [](float*) {});
	grid_region->data_amount[7] = transpose_inverse_model_matrices_grid.size() * 9;
	

	backpack.load_instance_buffer((float*)transpose_inverse_model_matrices_grid.data(), transpose_inverse_model_matrices_grid.size(), 7, grid_region);
	Logger::checkGLError("After loading transpose_inverse");


	Light sun = {false, glm::vec3(0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(1.0,1.0,1.0), glm::vec3(5.0,5.0,5.0), glm::vec3(0.5f,0.5f,0.5f),0,0,0,0,0};
	
	shader.use();
	shader.setInt("num_of_lights", 1);
	Logger::checkGLError("After changing light amount");

	shader.setBool("lights[0].has_a_source", sun.has_a_source);
	shader.setVec3("lights[0].light_pos", sun.light_pos);
	shader.setVec3("lights[0].light_target", sun.light_target);

	shader.setVec3("lights[0].ambient", sun.ambient);
	shader.setVec3("lights[0].diffuse", sun.diffuse);
	shader.setVec3("lights[0].specular", sun.specular);
	
	shader.setFloat("lights[0].cos_soft_cut_off_angle", sun.cos_soft_cut_off_angle);
	shader.setFloat("lights[0].cos_hard_cut_off_angle", sun.cos_hard_cut_off_angle);

	shader.setFloat("lights[0].constant", sun.constant);
	shader.setFloat("lights[0].linear", sun.linear);
	shader.setFloat("lights[0].quadratic", sun.quadratic);

	Logger::checkGLError("After loading light");

	
	Tree::set_model(&backpack, 100, 3);
	entt::registry registry;

	for (int i = 0; i < grid_amount; i++)
	{
		for (int j = 0; j < grid_amount; j++)
		{
			Tree* t = new Tree(registry, "Tree_" + std::to_string(i) + "_" + std::to_string(j));
			t->set_position({ i * 5.0f, 5.0f, j * 5.0f });
		}
	}

	//-------------------------------------------------------------------------------------------------------------
	input_manager = new Input_Manager(manager,window);

	

	Event_management::Event_receiver_shared camera_trigger = Event_management::make_receiver([](const Event_management::Event& e)
	{
			if (e.type == Event_management::Event_type::Mouse_moved)
			{
				const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);

				if (camera_control)
				{
					camera->process_mouse_movement(
						mouse.mouse_x_offset,
						mouse.mouse_y_offset,
						input_manager->mouse_sensitivity
					);
				}
			}
	});

	input_manager->subscribe(Mouse_input, Event_management::Event_type::Mouse_moved, camera_trigger);

	/*/? TEST CODE------------------------------------------------------------------------------------------------------
	// subscriber that prints mouse movement
	Event_receiver_shared mouse_receiver = make_receiver([](const Event& e)
	{
		if (e.type == Event_type::Mouse_moved)
		{
			const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);
			LOG_INFO("Mouse moved: %f, %f", mouse.mouse_x_offset, mouse.mouse_y_offset);
			const_cast<Mouse_move_event&>(mouse).is_alive = false; // consume it
		}
	});

	input_manager->subscribe(Mouse_input, Event_type::Mouse_moved, mouse_receiver);
	// Test all keyboard events
	Event_receiver_shared kb_receiver = make_receiver([](const Event& e)
		{
			if (e.type == Event_type::Keyboard_button_pressed)
			{
				const auto& ke = dynamic_cast<const Key_press_event&>(e);
				LOG_INFO("Key PRESSED: %d", ke.key.code);
			}
			else if (e.type == Event_type::Keyboard_button_hold)
			{
				const auto& ke = dynamic_cast<const Key_hold_event&>(e);
				LOG_INFO("Key HELD: %d", ke.key.code);
			}
			else if (e.type == Event_type::Keyboard_button_released)
			{
				const auto& ke = dynamic_cast<const Key_release_event&>(e);
				LOG_INFO("Key RELEASED: %d", ke.key.code);
			}
		});
	input_manager->subscribe(Keyboard_input, Event_type::Keyboard_button_pressed, kb_receiver);
	input_manager->subscribe(Keyboard_input, Event_type::Keyboard_button_hold, kb_receiver);
	input_manager->subscribe(Keyboard_input, Event_type::Keyboard_button_released, kb_receiver);

	// Test mouse buttons
	Event_receiver_shared mb_receiver = make_receiver([](const Event& e)
		{
			if (e.type == Event_type::Mouse_button_pressed)
			{
				const auto& me = dynamic_cast<const Mouse_button_press_event&>(e);
				LOG_INFO("Mouse PRESSED: %d", me.key.code);
			}
			else if (e.type == Event_type::Mouse_button_released)
			{
				const auto& me = dynamic_cast<const Mouse_button_release_event&>(e);
				LOG_INFO("Mouse RELEASED: %d", me.key.code);
			}
			else if (e.type == Event_type::Mouse_button_hold)
			{
				const auto& me = dynamic_cast<const Mouse_button_hold_event&>(e);
				LOG_INFO("Mouse HELD: %d", me.key.code);
			}
		});
	input_manager->subscribe(Mouse_input, Event_type::Mouse_button_pressed, mb_receiver);
	input_manager->subscribe(Mouse_input, Event_type::Mouse_button_released, mb_receiver);
	input_manager->subscribe(Mouse_input, Event_type::Mouse_button_hold, mb_receiver);

	// Test scroll
	Event_receiver_shared scroll_receiver = make_receiver([](const Event& e)
		{
			if (e.type == Event_type::Mouse_scrolled)
			{
				const auto& se = dynamic_cast<const Mouse_scroll_event&>(e);
				LOG_INFO("Scroll: %f, %f", se.x_offset, se.y_offset);
			}
		});
	input_manager->subscribe(Mouse_input, Event_type::Mouse_scrolled, scroll_receiver);

	// Test a combo — G + M
	input_manager->register_combo({
		Input_key{ Input_channel::Keyboard_input, GLFW_KEY_G },
		Input_key{ Input_channel::Keyboard_input, GLFW_KEY_M }
		});
	Event_receiver_shared combo_receiver = make_receiver([](const Event& e)
		{
			if (e.type == Event_type::Combo_button_pressed)
				LOG_INFO("Combo_button PRESSED");
			else if (e.type == Event_type::Combo_button_released)
				LOG_INFO("Combo_button RELEASED");
			else if (e.type == Event_type::Combo_button_hold)
				LOG_INFO("Combo_button HELD");
		});
	input_manager->subscribe(Combo_input, Event_type::Combo_button_pressed, combo_receiver);
	input_manager->subscribe(Combo_input, Event_type::Combo_button_released, combo_receiver);
	input_manager->subscribe(Combo_input, Event_type::Combo_button_hold, combo_receiver);

	//? END OF TEST CODE-----------------------------------------------------------------------------------------------*/

	//-------------------------------------------------------------------------------------------------------------

	glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D rendering
	glEnable(GL_CULL_FACE); // Enable face culling to improve performance
	//glCullFace(GL_FRONT_AND_BACK);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	int x = 0, y = 0, z = 0;
	double time_of_last_frame = 1;
	std::string fps_text = "";
	glfwSetTime(0.0);

	shader.bind_UBO("projectionXview_block",camera->Ubo_slot);

	while (!glfwWindowShouldClose(window))
	{

		game_object_base::Tick(registry);
		camera_control = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

		processInput(window, 0.1 * ((glfwGetTime() - time_of_last_frame) / Target_frame_time), camera);
		time_of_last_frame = glfwGetTime();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		shader.setVec3("viewPos", camera->camera_position);
		
		backpack.draw(shader, grid_region, grid_amount * grid_amount);

		//Tree::draw(shader);

		Logger::checkGLError("After drawing grid backpack");

		x++;
		if (glfwGetTime() - z >= 1.0f)
		{
			y = x;
			x = 0;
			z = glfwGetTime();
			fps_text = "FPS: " + std::to_string(y);
			LOG_INFO("FPS: %d Draw calls per second: %d", y, draw_call_count);
			draw_call_count = 0;
		}
		printer->render_text(fps_text, -1, 0.9, 2.0f);
		Logger::checkGLError("After drawing fps");

		glfwSwapBuffers(window);
		input_manager->Poll_keys();
		glfwPollEvents();
	}
	delete printer;
}
