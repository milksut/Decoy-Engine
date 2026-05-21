#include "Camera_test.h"
#include "game_object_basic.h"
#include "Globals.h"
#include "Shader.h"
#include "Some_functions.h"
#include "TextRenderer.h"
#include "ray_casting.h"
#include "UI_Manager.h"
#include "Window_manager.h"

bool camera_control = false;

const double Target_fps = 144;
const double Target_frame_time = 1.0 / Target_fps;
const bool enable_vSync = false;

game_object_basic_model backpack;

Event_manager manager;
Input_Manager* my_input_manager;

//-*-*-*-*-*-*-*-**-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*-*-*-*-*-*-*-*
int grid_amount = 100;
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

std::unordered_map<unsigned int, Tree*> tree_map;
unsigned int selected_id = 0;
entt::entity selected_entity = entt::null;
Text_panel* info_panel = nullptr;

TextRenderer* printer;
camera_test* fps_camera;
UI_manager ui_manager;

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
	Window_Manager my_window_manager(manager);
	my_input_manager = my_window_manager.get_input_manager();
	Window_Manager::Config config_ref = my_window_manager.get_config_ref();
	ui_manager.init(my_input_manager, config_ref.width, config_ref.height);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	glfwSwapInterval(enable_vSync);
	fps_camera = new camera_test(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(270.0f, 0.0f, 0.0f));
	fps_camera->update_projection(45.0f, config_ref.aspect_ratio, 0.1f, 500.0f);

	printer = new TextRenderer("Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.png",
		"Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.txt",
		config_ref.width, config_ref.height, 16, 32,
		"Shaders/Vertex_shaders/Text_render_vertex.vert",
		"Shaders/Fragment_shaders/Text_render_fragment.frag",
		"Shaders/Geometry_shaders/Text_render_geometry.geom",
		0.005f);

	printer->change_deleted_colors(0, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.5f, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
	printer->change_deleted_colors(1, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	printer->push_deleted_colors();


	Shader shader("Shaders/Vertex_shaders/Loaded_model_vertex.vert",
		"Shaders/Fragment_shaders/Loaded_model_fragment.frag");

	Shader ui_shader("Shaders/Vertex_shaders/Ui_vertex.vert",
		"Shaders/Fragment_shaders/Ui_fragment.frag");

	Button* addButton = new Button(
		glm::vec2(-0.95f, 0.0f),
		glm::vec2(0.3f, 0.2f),
		"Sil",
		[]()
		{
			if (selected_id != 0)
			{
				auto it = tree_map.find(selected_id);
				if (it != tree_map.end())
				{
					Tree* t = it->second;
					auto region = Tree::get_class_region();
					int deleted_slot = t->get_region_slot_index();

					int last_slot = (int)region->object_ptrs.size() - 1;
					while (last_slot >= 0 && region->object_ptrs[last_slot] == nullptr)
						last_slot--;


					if (last_slot >= 0 && last_slot != deleted_slot)
					{
						game_object_basic<Tree>* last_obj = static_cast<game_object_basic<Tree>*>(region->object_ptrs[last_slot]);
						last_obj->move_to_slot(deleted_slot);
						glm::mat4 world = last_obj->get_transform_copy().world;
						backpack.load_instance_buffer(
							reinterpret_cast<float*>(&world),
							1,
							3,
							region,
							(unsigned int)deleted_slot
						);
					}

					delete t;
					tree_map.erase(it);
				}
				selected_entity = entt::null;
				selected_id = 0;
			}
		},
		&ui_shader,
		printer
	);

	ui_manager.add_widget(addButton);

	addButton->set_color({ 0.0f, 0.6f, 5.2f, 1.0f });
	addButton->set_text_scale(2.3f);

	Button* addTreeButton = new Button(
		glm::vec2(-0.95f, 0.5f),
		glm::vec2(0.3f, 0.2f),
		"Ekle",
		[&config_ref]()
		{
			glm::vec3 ray_dir = Ray_casting::ScreenToWorldRay(
				(float)config_ref.width / 2.0f,
				(float)config_ref.height / 2.0f,
				config_ref.width, config_ref.height,
				fps_camera->projection,
				fps_camera->view
			);

			glm::vec3 pos = Ray_casting::ray_plane_intersection(
				fps_camera->camera_position,
				ray_dir,
				0.0f
			);

			if (pos == glm::vec3(0.0f))
				return;

			Tree* t = new Tree(global_registry, "Tree_added_" + std::to_string(tree_map.size()));
			t->set_position(pos);
			tree_map[t->get_id()] = t;
		},
		&ui_shader,
		printer
	);

	ui_manager.add_widget(addTreeButton);
	addTreeButton->set_text_scale(2.3f);
	addTreeButton->set_color({ 0.0f, 5.6f, 0.2f, 1.0f });
	addTreeButton->set_text_scale(2.3f);

	info_panel = new Text_panel(
		glm::vec2(0.5f, -0.9f),
		glm::vec2(0.4f, 0.3f),
		&ui_shader,
		printer
	);
	info_panel->visible = false;
	ui_manager.add_widget(info_panel);

	info_panel->set_background_color({ 0.15f, 0.15f, 0.15f, 0.9f });
	info_panel->set_text_scale(0.75f);
	info_panel->set_line_spacing(0.08f);

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


	backpack.import_model_from_file("Models\\Tree1.obj");
	
	backpack.add_instance_buffer(16, 3); //attrib size-mat4-16floats, attrib index, for model

	backpack.add_instance_buffer(9, 7); //attrib size-mat3-12floats, attrib index, for transpose_inverse_viewXmodel

	Tree::set_model(&backpack, grid_amount* grid_amount, 3);

	for (int i = 0; i < grid_amount; i++)
	{
		for (int j = 0; j < grid_amount; j++)
		{
			Tree* t = new Tree(global_registry, "Tree_" + std::to_string(i) + "_" + std::to_string(j));
			t->set_position({ i * 5.0f, 0.0f, j * 5.0f });
			tree_map[t->get_id()] = t;
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

	Event_management::Event_receiver_shared click_receiver =Event_management::make_receiver([&pointer_arrow, &config_ref](const Event_management::Event& e)
		{
			if (e.type == Event_management::Event_type::Mouse_button_pressed)
			{
				const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);

				if (mouse.key.code != GLFW_MOUSE_BUTTON_LEFT) return;

				if (ui_manager.is_hovered_ndc())
					return;

				entt::entity closest_entity = entt::null;
				float closest_distance = -1.0f;

				auto view = global_registry.view<Transform_component, Tag_component>();
				view.each([&](entt::entity entity, Transform_component& transform, Tag_component& tag)
				{
					if(tag.tag.find("Tree") == std::string::npos)
						return; //only check entities with "Tree" in their tag

					glm::vec3 rayDir = Ray_casting::ScreenToWorldRay((float)mouse.mouse_x, (float)mouse.mouse_y,
						config_ref.width, config_ref.height, fps_camera->projection, fps_camera->view);

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

					selected_entity = closest_entity;
					selected_id = global_registry.get<Id_component>(closest_entity).id;
					pointer_arrow.set_position(glm::vec3(transform.position.x, pointer_arrow.get_position().y, transform.position.z));
					info_panel->clear();
					info_panel->set_line(0, "Tag: " + tag.tag);
					info_panel->set_line(1, "Pos X: " + std::to_string(transform.position.x));
					info_panel->set_line(2, "Pos Y: " + std::to_string(transform.position.y));
					info_panel->set_line(3, "Pos Z: " + std::to_string(transform.position.z));
					info_panel->visible = true;
				}
			}
		});

	my_input_manager->subscribe(Input_channel_names[Mouse_input], Event_management::Event_type::Mouse_button_pressed, click_receiver);

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

	my_input_manager->subscribe(Input_channel_names[Mouse_input], Event_management::Event_type::Mouse_moved, camera_trigger);

	bool window_should_close = false;

	Event_management::Event_receiver_shared window_close_reciver = Event_management::make_receiver([&window_should_close](const Event_management::Event& e)
		{
			if(e.type == Event_management::Event_type::Window_closed)
			{
				window_should_close = true;
			}
		});

	my_window_manager.subscribe(Event_management::Event_type::Window_closed, window_close_reciver);

	Event_management::Event_receiver_shared framebuffer_chnage_reciver = Event_management::make_receiver([](const Event_management::Event& e)
		{
			if (e.type == Event_management::Event_type::Window_framebuffer_resized)
			{
				const auto& resize = static_cast<const Window_framebuffer_resize_event&>(e);

				fps_camera->update_projection(45.0f, resize.new_aspect_ratio, 0.1f, 500.0f);
				printer->change_screen_size(resize.new_width, resize.new_height);
			}
			
		});

	my_window_manager.subscribe(Event_management::Event_type::Window_framebuffer_resized, framebuffer_chnage_reciver);

	glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D rendering
	glEnable(GL_CULL_FACE); // Enable face culling to improve performance
	//glCullFace(GL_FRONT_AND_BACK);
	int x = 0, y = 0;
	double time_of_last_frame = 1.0, z = 0;
	std::string fps_text = "";
	glfwSetTime(0.0);
	bool reverse_arrow_animation = false;

	shader.bind_UBO("projectionXview_block",fps_camera->Ubo_slot);

	while (!window_should_close)
	{

		// Frustum culling test start --------------------------------------------------------------

		Frustum frustum = Ray_casting::extract_frustum(*fps_camera);

		std::vector<unsigned int> visible_list;
		std::vector<int> available_indices;

		std::vector<void*>& region = Tree::get_class_region()->object_ptrs;
		unsigned int region_size = (unsigned int)region.size();

		visible_list.reserve(region_size * 10);
		available_indices.reserve(region_size);

		auto view = global_registry.view<Transform_component, Id_component, Tag_component, World_AABB_component>();

		view.each([&visible_list, &frustum](auto /*entity*/, Transform_component& /*transform*/,
			Id_component& id_comp, Tag_component& tag_comp, World_AABB_component& aabb_comp)
			{

				if (tag_comp.tag.find("Tree") == std::string::npos)
				{
					return; //only check entities with "Tree" in their tag
				}

				if (Ray_casting::aabb_in_frustum(frustum, aabb_comp.aabb))
				{
					visible_list.push_back(id_comp.id);
				}
			});

		const unsigned int visible_amount = (unsigned int)visible_list.size();

		for (unsigned int i = 0; i < visible_amount && i < region_size; i++)
		{
			game_object_base* ptr = static_cast<game_object_base*>(region[i]);

			if (ptr == nullptr)
			{
				available_indices.push_back(i);
				continue;
			}

			unsigned int temp_id = ptr->get_id();

			auto index = std::find(visible_list.begin(), visible_list.end(), temp_id);

			if (index != visible_list.end())
			{
				*index = 0;
				continue;
			}
			else
			{
				available_indices.push_back(i);
				continue;
			}
		}
		int index_in_availble_list = 0;

		for (unsigned int id : visible_list)
		{
			if (index_in_availble_list >= available_indices.size())
			{
				break;
			}
			if (id == 0)
			{
				continue;
			}

			int new_index = available_indices[index_in_availble_list++];
			void* obj_ptr = region[new_index];

			if (obj_ptr == nullptr)
			{
				Global_object_map::get_object(id)->use_null_region_pos(new_index);
			}
			else
			{
				static_cast<game_object_base*>(obj_ptr)->swap_region_pos(id);
			}
		}
		// Frustum culling test end -----------------------------------------------------------------

		Tree::max_region_upload_index = visible_amount;
		game_object_base::Tick(global_registry);

		Key_state state_holder_temp = my_input_manager->Get_key_state({ Mouse_input,GLFW_MOUSE_BUTTON_RIGHT });

		camera_control = state_holder_temp == Pressed || state_holder_temp == Hold;




		processInput(my_window_manager.get_handle(), 0.1f * (float)((glfwGetTime() - time_of_last_frame) / Target_frame_time), fps_camera);
		time_of_last_frame = glfwGetTime();

		ui_manager.update();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		shader.setVec3("viewPos", fps_camera->camera_position);

		if(visible_amount>0)
			Tree::draw(shader, visible_amount);
		Arrow::draw(shader);

		Logger::checkGLError("After drawing grid backpack");

		x++;
		if (glfwGetTime() - z >= 1.0f)
		{
			y = x;
			x = 0;
			z = glfwGetTime();
			fps_text = "FPS: " + std::to_string(y);
			draw_call_count = 0;
			reverse_arrow_animation = !reverse_arrow_animation;
		}
		printer->render_text(fps_text, -1.0f, 0.9f, 2.0f);
		Logger::checkGLError("After drawing fps");

		ui_manager.render();

		pointer_arrow.rotate(glm::vec3(0.0f, 0.005f, 0.0f));

		pointer_arrow.move(glm::vec3(0.0f, reverse_arrow_animation ? 0.001f : -0.001f, 0.0f));

		my_input_manager->Poll_keys();
		my_window_manager.Tick();
	}
	delete printer;
}
