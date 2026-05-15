#include "Camera_test.h"
#include "game_object_basic_model.h"
#include "Globals.h"
#include "Shader.h"
#include "Some_functions.h"
#include "TextRenderer.h"

#include "Input_Manager.h"
#include "The_event_manager.h"
#include "Headers/game_object_basic.h"
#include "Headers/ray_casting.h"
#include "UI_Manager.h"

bool camera_control = false;

const double Target_fps = 144;
const double Target_frame_time = 1.0 / Target_fps;
const bool enable_vSync = false;

unsigned int screen_width = 800, screen_height = 600;
const float aspect_ratio = (float)screen_width / (float)screen_height;


Event_manager manager;
Input_Manager* my_input_manager;

//-*-*-*-*-*-*-*-**-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*-*
int grid_amount = 60;
//-*-*-*-*-*-*-*-**-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*-*

class Tree : public game_object_basic<Tree> {
public:
	Tree(entt::registry& reg, const std::string& tag, game_object_basic* parent = nullptr)
		: game_object_basic(reg, tag, parent)
	{
	}
};

class Arrow : public game_object_basic<Arrow>
{
public:
	Arrow(entt::registry& reg, const std::string& tag)
		: game_object_basic(reg, tag)
	{}
};

TextRenderer* printer;
camera_test* fps_camera;
UI_manager ui_manager;

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
	const float new_aspect_ratio = (float)width / (float)height;

	glViewport(0, 0, width, height);

	fps_camera->update_projection(45.0f, new_aspect_ratio, 0.1f, 1000.0f);
	printer->change_screen_size(width, height);

	screen_width = width;
	screen_height = height;
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
	GLFWwindow* window = init_window(screen_width, screen_height, "Shader Tester");
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	if (!window) {
		return -1;
	}

	glfwSwapInterval(enable_vSync);
	fps_camera = new camera_test(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	fps_camera->update_projection(45.0f, aspect_ratio, 0.1f, 1000.0f);

	printer = new TextRenderer("Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.png",
		"Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.txt",
		screen_width, screen_height, 16, 32,
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

	Shader ui_shader("Shaders/Vertex_shaders/Ui_vertex.vert",
		"Shaders/Fragment_shaders/Ui_fragment.frag");

	Button* addButton = new Button(
		glm::vec2(-0.95f, 0.0f),
		glm::vec2(0.8f, 0.4f),
		"Ekle",
		[]() { std::cout << "clicked\n"; },
		&ui_shader,
		printer
	);

	ui_manager.add_widget(addButton);

	addButton->set_color({ 0.0f, 0.6f, 5.2f, 1.0f });
	addButton->set_text_scale(2.3f);



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

	game_object_basic_model backpack;

	backpack.import_model_from_file("Models\\Tree1.obj");
	
	backpack.add_instance_buffer(16, 3); //attrib size-mat4-16floats, attrib index, for model

	backpack.add_instance_buffer(9, 7); //attrib size-mat3-12floats, attrib index, for transpose_inverse_viewXmodel

	Tree::set_model(&backpack, grid_amount * grid_amount, 3);

	for (int i = 0; i < grid_amount; i++)
	{
		for (int j = 0; j < grid_amount; j++)
		{
			Tree* t = new Tree(global_registry, "Tree_" + std::to_string(i) + "_" + std::to_string(j));
			t->set_position({ i * 5.0f, 0.0f, j * 5.0f });
		}
	}

	game_object_basic_model arrow;
	arrow.import_model_from_file("Models\\Cylinder.obj");
	

	int root_index = arrow.import_model_from_file("Models\\Cone.obj");
	arrow.add_instance_buffer(16, 3);
	arrow.add_instance_buffer(9, 7);
	if(root_index < 0)
	{
		LOG_ERROR("Failed to load cone model!");
	}
	else
	{
		LOG_INFO("Cone model loaded successfully with root index: %d", root_index);

		glm::mat4 offset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
		offset = glm::rotate(offset, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		offset = glm::scale(offset, glm::vec3(1.5f, 0.5f, 1.5f));

		arrow.offset_mesh_vertices(arrow.roots[root_index].flattened_index_start, arrow.roots[root_index].flattened_index_end, offset);
	}
	
	

	Arrow::set_model(&arrow, 1);
	Arrow pointer_arrow(global_registry, "Pointer_arrow");
	pointer_arrow.set_position({ 0.0f, 4.5f, 0.0f });

	//-------------------------------------------------------------------------------------------------------------
	my_input_manager = new Input_Manager(manager,window);

	Event_management::Event_receiver_shared click_receiver = Event_management::make_receiver([&pointer_arrow](const Event_management::Event& e)
		{
			if (e.type == Event_management::Event_type::Mouse_button_pressed)
			{
				const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);

				if (mouse.key.code != GLFW_MOUSE_BUTTON_LEFT) return;


				entt::entity closest_entity = entt::null;
				float closest_distance = -1.0f;

				auto view = global_registry.view<Transform_component, Tag_component>();
				view.each([&](entt::entity entity, Transform_component& transform, Tag_component& tag)
				{
					if(tag.tag.find("Tree") == std::string::npos)
						return; //only check entities with "Tree" in their tag

					glm::vec3 rayDir = Ray_casting::ScreenToWorldRay((float)mouse.mouse_x, (float)mouse.mouse_y,
						screen_width, screen_height, fps_camera->projection, fps_camera->view);

					float dist = Ray_casting::ray_sphere_intersection(fps_camera->camera_position, rayDir,
						glm::vec3(transform.position), 3.0f);

					if (dist > 0)
					{
						if (closest_distance < 0 || dist < closest_distance)
						{
							closest_distance = dist;
							closest_entity = entity;
						}
					}
				});

				if (closest_entity != entt::null)
				{
					auto& tag = global_registry.get<Tag_component>(closest_entity);
					auto& transform = global_registry.get<Transform_component>(closest_entity);

					LOG_INFO("Selected: %s pos: %.2f, %.2f, %.2f", tag.tag.c_str(),
						transform.position.x, transform.position.y, transform.position.z);

					pointer_arrow.set_position(glm::vec3(transform.position.x, pointer_arrow.get_position().y, transform.position.z));
				}
			}
		});

	my_input_manager->subscribe(Mouse_input, Event_management::Event_type::Mouse_button_pressed, click_receiver);

	Event_management::Event_receiver_shared camera_trigger = Event_management::make_receiver([](const Event_management::Event& e)
	{
			if (e.type == Event_management::Event_type::Mouse_moved)
			{
				const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);

				if (camera_control)
				{
					fps_camera->process_mouse_movement(
						(float)mouse.mouse_x_offset,
						(float)mouse.mouse_y_offset,
						(float)my_input_manager->mouse_sensitivity
					);
				}
			}
	});

	my_input_manager->subscribe(Mouse_input, Event_management::Event_type::Mouse_moved, camera_trigger);

	glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D rendering
	glEnable(GL_CULL_FACE); // Enable face culling to improve performance
	//glCullFace(GL_FRONT_AND_BACK);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	int x = 0, y = 0;
	double time_of_last_frame = 1.0, z = 0;
	std::string fps_text = "";
	glfwSetTime(0.0);
	bool reverse_arrow_animation = false;

	shader.bind_UBO("projectionXview_block",fps_camera->Ubo_slot);

	
	while (!glfwWindowShouldClose(window))
	{

		//// Frustum culling test start --------------------------------------------------------------
		//float fov = 10.0f;
		//float fov_cosine = cos(glm::radians(fov / 2.0f));

		//std::unordered_set<unsigned int> visible_list;
		//std::unordered_set<int> available_indices;

		//auto view = global_registry.view<Transform_component, Id_component, Tag_component >();

		//view.each([&fov_cosine, &visible_list](auto /*entity*/, Transform_component& transform,
		//	Id_component& id_comp, Tag_component& tag_comp)
		//{
		//	if (tag_comp.tag.find("Tree") == std::string::npos)
		//	{
		//		return; //only check entities with "Tree" in their tag
		//	}
		//			
		//	if (Ray_casting::is_in_frustum(fps_camera->camera_position,
		//		fps_camera->camera_front, transform.position, 100.0f, fov_cosine))
		//	{
		//		visible_list.insert(id_comp.id);
		//	}
		//});

		//const unsigned int visible_amount = (unsigned int)visible_list.size();

		//std::vector<void*> region = Tree::get_class_region()->object_ptrs;

		//for(unsigned int i =0; i < visible_amount; i++)
		//{
		//	game_object_base* ptr = static_cast<game_object_base*>(region[i]);

		//	if(ptr == nullptr)
		//	{
		//		available_indices.insert(i);
		//		continue;
		//	}

		//	unsigned int temp_id = ptr->get_id();

		//	if(visible_list.find(temp_id) != visible_list.end())
		//	{
		//		visible_list.erase(temp_id);
		//		continue;
		//	}
		//	else
		//	{
		//		available_indices.insert(i);
		//		continue;
		//	}
		//}

		//for(unsigned int id : visible_list)
		//{
		//	auto it = available_indices.begin();
		//	int new_index = *it;
		//	available_indices.erase(it);

		//	void* obj_ptr = region[new_index];
		//	if(obj_ptr == nullptr)
		//	{
		//		Global_object_map::get_object(id)->use_null_region_pos(new_index);
		//	}
		//	else
		//	{
		//		static_cast<game_object_base*>(obj_ptr)->swap_region_pos(id);
		//	}
		//}

		//// Frustum culling test end -----------------------------------------------------------------

		game_object_base::Tick(global_registry);
		camera_control = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

		bool mouse_clicked = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		//mouse position in NDC for UI interactions
		double mouse_xd, mouse_yd;
		glfwGetCursorPos(window, &mouse_xd, &mouse_yd);

		float mouse_ndc_x = (float)(mouse_xd / screen_width) * 2.0f - 1.0f;
		float mouse_ndc_y = 1.0f - (float)(mouse_yd / screen_height) * 2.0f;

		processInput(window, 0.1f * (float)((glfwGetTime() - time_of_last_frame) / Target_frame_time), fps_camera);
		time_of_last_frame = glfwGetTime();

		ui_manager.update(mouse_ndc_x, mouse_ndc_y, mouse_clicked);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		shader.setVec3("viewPos", fps_camera->camera_position);

		Tree::draw(shader);
		Arrow::draw(shader);

		Logger::checkGLError("After drawing grid backpack");

		x++;
		if (glfwGetTime() - z >= 1.0f)
		{
			y = x;
			x = 0;
			z = glfwGetTime();
			fps_text = "FPS: " + std::to_string(y);
			//LOG_INFO("FPS: %d Draw calls per second: %d", y, draw_call_count);
			draw_call_count = 0;
			reverse_arrow_animation = !reverse_arrow_animation;
		}
		printer->render_text(fps_text, -1.0f, 0.9f, 2.0f);
		Logger::checkGLError("After drawing fps");

		ui_manager.render();

		pointer_arrow.rotate(glm::vec3(0.0f, 0.005f, 0.0f));

		pointer_arrow.move(glm::vec3(0.0f, reverse_arrow_animation ? 0.001f : -0.001f, 0.0f));

		glfwSwapBuffers(window);
		my_input_manager->Poll_keys();
		glfwPollEvents();
	}
	delete printer;
}
