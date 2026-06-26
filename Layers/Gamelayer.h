#pragma once

#include "Layer_manager.h"
#include "App.h"
#include "Camera_test.h"
#include "game_object_basic.h"
#include "Animation_manager.h"
#include "Globals.h"
#include "Shader.h"
#include "Some_functions.h"
#include "TextRenderer.h"
#include "ray_casting.h"
#include "UI_Manager.h"
#include "Window_manager.h"

// ---------------------------------------------------------------------------
// Game object types
// ---------------------------------------------------------------------------
class Tree : public game_object_basic<Tree>
{
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
    {
    }
};

class Man : public game_object_basic<Man>
{
public:
    Man(entt::registry& reg, const std::string& tag)
        : game_object_basic(reg, tag)
    {
    }
};

// ---------------------------------------------------------------------------
class GameLayer : public Layer
{
    static const int    GRID_AMOUNT = 100;
    static const int    MAN_GRID = 31;
    static constexpr double TARGET_FPS = 144.0;
    static constexpr double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

public:
    GameLayer(App& app)
        : Layer("GameLayer")
        , m_app(app)
    {
    }

    // -----------------------------------------------------------------------
    void onAttach() override
    {
        Window_Manager& win = m_app.get_window();
        Input_Manager* input = m_app.get_input_manager();
        const auto& win_config = win.get_config_ref();

        // Camera
        m_camera = std::make_unique<camera_test>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(270.0f, 0.0f, 0.0f)
        );
        m_camera->update_projection(45.0f, win_config.aspect_ratio, 0.1f, 500.0f);

        // Text renderer
        m_printer = std::make_unique<TextRenderer>(
            "Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.png",
            "Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.txt",
            win_config.width, win_config.height, 16, 32,
            "Shaders/Vertex_shaders/Text_render_vertex.vert",
            "Shaders/Fragment_shaders/Text_render_fragment.frag",
            "Shaders/Geometry_shaders/Text_render_geometry.geom",
            0.005f
        );
        m_printer->change_deleted_colors(0, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.5f, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
        m_printer->change_deleted_colors(1, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        m_printer->push_deleted_colors();

        // Shaders
        m_shader = std::make_unique<Shader>(
            "Shaders/Vertex_shaders/Loaded_model_vertex.vert",
            "Shaders/Fragment_shaders/Loaded_model_fragment.frag");
        m_ui_shader = std::make_unique<Shader>(
            "Shaders/Vertex_shaders/Ui_vertex.vert",
            "Shaders/Fragment_shaders/Ui_fragment.frag");
        m_animation_shader = std::make_unique<Shader>(
            "Shaders/Vertex_shaders/Loaded_animation_vertex.vert",
            "Shaders/Fragment_shaders/Loaded_model_fragment.frag");

        // UI
        UI_manager::Config ui_config;
        ui_config.screen_width = static_cast<unsigned int>(win_config.width);
        ui_config.screen_height = static_cast<unsigned int>(win_config.height);
        m_ui.init(input, ui_config);

        setup_ui();
        setup_lights();
        setup_models();
        setup_animations();

        // UBO bindings
        m_shader->bind_UBO("projectionXview_block", m_camera->Ubo_slot);
        m_animation_shader->bind_UBO("projectionXview_block", m_camera->Ubo_slot);
        int anim_ubo = Animation_manager::get_Ubo_slot();
        if (anim_ubo >= 0)
            m_animation_shader->bind_UBO("Bone_block", anim_ubo);

        setup_events(input, win);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glfwSetTime(0.0);
        m_time_of_last_frame = glfwGetTime();
    }

    // -----------------------------------------------------------------------
    void onDetach() override
    {
        for (auto& kv : m_tree_map) delete kv.second;
        m_tree_map.clear();

        for (Man* m : m_collective) delete m;
        m_collective.clear();
    }

    // -----------------------------------------------------------------------
    void Update() override
    {
        double now = glfwGetTime();
        float  dt = static_cast<float>(now - m_time_of_last_frame);

        game_object_base::Tick(global_registry);
        m_man_animator->Tick(0.3f * (dt / static_cast<float>(TARGET_FRAME_TIME)));

        Input_Manager* input = m_app.get_input_manager();
        Key_state rmb = input->Get_key_state({ Mouse_input, GLFW_MOUSE_BUTTON_RIGHT });
        m_camera_control = (rmb == Pressed || rmb == Hold);

        processInput(dt);
        m_time_of_last_frame = glfwGetTime();

        m_ui.update();

        m_frame_count++;
        if (glfwGetTime() - m_fps_last_reset >= 1.0)
        {
            m_fps_text = "FPS: " + std::to_string(m_frame_count);
            m_frame_count = 0;
            m_fps_last_reset = glfwGetTime();
            draw_call_count = 0;
            m_reverse_arrow = !m_reverse_arrow;
        }
    }

    // -----------------------------------------------------------------------
    void Render() override
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        m_shader->use();
        m_shader->setVec3("viewPos", m_camera->camera_position);

        Arrow::draw(*m_shader);
        Man::draw(*m_animation_shader);

        Logger::checkGLError("After drawing objects");

        m_printer->render_text(m_fps_text, -1.0f, 0.9f, 2.0f);

        m_pointer_arrow->rotate(glm::vec3(0.0f, 0.005f, 0.0f));
        m_pointer_arrow->move(glm::vec3(0.0f, m_reverse_arrow ? 0.001f : -0.001f, 0.0f));

        m_ui.render();
    }

private:
    App& m_app;

    std::unique_ptr<camera_test>             m_camera;
    std::unique_ptr<TextRenderer>            m_printer;
    std::unique_ptr<Shader>                  m_shader;
    std::unique_ptr<Shader>                  m_ui_shader;
    std::unique_ptr<Shader>                  m_animation_shader;
    std::unique_ptr<Animation_manager>       m_man_animator;

    UI_manager m_ui;

    game_object_basic_model                  m_backpack_model;
    std::unordered_map<unsigned int, Tree*>  m_tree_map;

    std::unique_ptr<game_object_basic_model> m_arrow_model;
    std::unique_ptr<Arrow>                   m_pointer_arrow;

    std::unique_ptr<game_object_basic_model> m_man_model;
    std::vector<Man*>                        m_collective;

    Text_panel* m_info_panel = nullptr;
    unsigned int m_selected_id = 0u;
    entt::entity m_selected_entity = entt::null;

    Event_management::Event_receiver_shared m_click_receiver;
    Event_management::Event_receiver_shared m_camera_trigger;
    Event_management::Event_receiver_shared m_framebuffer_receiver;

    bool         m_camera_control = false;
    bool         m_reverse_arrow = false;
    bool         m_f11_pressable = true;
    double       m_time_of_last_frame = 0.0;
    int          m_frame_count = 0;
    double       m_fps_last_reset = 0.0;
    std::string  m_fps_text;

    // -----------------------------------------------------------------------
    void setup_ui()
    {
        Button* deleteButton = new Button(
            glm::vec2(-0.95f, 0.0f),
            glm::vec2(0.3f, 0.2f),
            "Sil",
            [this]()
            {
                if (m_selected_id == 0u) return;

                auto it = m_tree_map.find(m_selected_id);
                if (it != m_tree_map.end())
                {
                    Tree* t = it->second;
                    auto  region = Tree::get_class_region();
                    int   deleted_slot = t->get_region_slot_index();

                    int last_slot = static_cast<int>(region->object_ptrs.size()) - 1;
                    while (last_slot >= 0 && region->object_ptrs[last_slot] == nullptr)
                        last_slot--;

                    if (last_slot >= 0 && last_slot != deleted_slot)
                    {
                        auto* last_obj = static_cast<game_object_basic<Tree>*>(region->object_ptrs[last_slot]);
                        last_obj->move_to_slot(deleted_slot);
                        glm::mat4 world = last_obj->get_transform_copy().world;
                        m_backpack_model.load_instance_buffer(
                            reinterpret_cast<float*>(&world),
                            1, 3, region,
                            static_cast<unsigned int>(deleted_slot)
                        );
                    }

                    delete t;
                    m_tree_map.erase(it);
                }
                m_selected_entity = entt::null;
                m_selected_id = 0u;
            },
            m_ui_shader.get(),
            m_printer.get()
        );
        m_ui.add_widget(deleteButton);
        deleteButton->set_color({ 0.0f, 0.6f, 5.2f, 1.0f });
        deleteButton->set_text_scale(2.3f);

        Button* addTreeButton = new Button(
            glm::vec2(-0.95f, 0.5f),
            glm::vec2(0.3f, 0.2f),
            "Ekle",
            [this]()
            {
                const auto& cfg = m_app.get_window().get_config_ref();
                glm::vec3 ray_dir = Ray_casting::ScreenToWorldRay(
                    static_cast<float>(cfg.width) / 2.0f,
                    static_cast<float>(cfg.height) / 2.0f,
                    static_cast<unsigned int>(cfg.width),
                    static_cast<unsigned int>(cfg.height),
                    m_camera->projection,
                    m_camera->view
                );

                glm::vec3 pos = Ray_casting::ray_plane_intersection(
                    m_camera->camera_position, ray_dir, 0.0f);

                if (pos == glm::vec3(0.0f)) return;

                unsigned int next_id = static_cast<unsigned int>(m_tree_map.size());
                Tree* t = new Tree(global_registry, "Tree_added_" + std::to_string(next_id));
                t->set_position(pos);
                m_tree_map[t->get_id()] = t;
            },
            m_ui_shader.get(),
            m_printer.get()
        );
        m_ui.add_widget(addTreeButton);
        addTreeButton->set_text_scale(2.3f);
        addTreeButton->set_color({ 0.0f, 5.6f, 0.2f, 1.0f });

        m_info_panel = new Text_panel(
            glm::vec2(0.5f, -0.9f),
            glm::vec2(0.4f, 0.3f),
            m_ui_shader.get(),
            m_printer.get()
        );
        m_info_panel->visible = false;
        m_ui.add_widget(m_info_panel);
        m_info_panel->set_background_color({ 0.15f, 0.15f, 0.15f, 0.9f });
        m_info_panel->set_text_scale(0.75f);
        m_info_panel->set_line_spacing(0.08f);
    }

    // -----------------------------------------------------------------------
    void setup_lights()
    {
        Light sun = {
            false,
            glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(1.0f), glm::vec3(5.0f), glm::vec3(0.5f),
            0, 0, 0, 0, 0
        };

        auto set_light = [&sun](Shader& s)
            {
                s.use();
                s.setInt("num_of_lights", 1);
                s.setBool("lights[0].has_a_source", sun.has_a_source);
                s.setVec3("lights[0].light_pos", sun.light_pos);
                s.setVec3("lights[0].light_target", sun.light_target);
                s.setVec3("lights[0].ambient", sun.ambient);
                s.setVec3("lights[0].diffuse", sun.diffuse);
                s.setVec3("lights[0].specular", sun.specular);
                s.setFloat("lights[0].cos_soft_cut_off_angle", sun.cos_soft_cut_off_angle);
                s.setFloat("lights[0].cos_hard_cut_off_angle", sun.cos_hard_cut_off_angle);
                s.setFloat("lights[0].constant", sun.constant);
                s.setFloat("lights[0].linear", sun.linear);
                s.setFloat("lights[0].quadratic", sun.quadratic);
            };

        set_light(*m_shader);
        set_light(*m_animation_shader);
        Logger::checkGLError("After loading light");
    }

    // -----------------------------------------------------------------------
    void setup_models()
    {
        // Trees
        for (int i = 0; i < GRID_AMOUNT; i++)
        {
            for (int j = 0; j < GRID_AMOUNT; j++)
            {
                Tree* t = new Tree(global_registry,
                    "Tree_" + std::to_string(i) + "_" + std::to_string(j));
                t->set_position({ i * 5.0f, 0.0f, j * 5.0f });
                m_tree_map[t->get_id()] = t;
            }
        }

        // Arrow
        m_arrow_model = std::make_unique<game_object_basic_model>();
        m_arrow_model->import_model_from_file("Models\\Cylinder.obj");
        int cone_root = m_arrow_model->import_model_from_file("Models\\Cone.obj");
        m_arrow_model->add_instance_buffer(16, MODEL_ATRIB_LAST_INDEX + 1);
        m_arrow_model->add_instance_buffer(9, MODEL_ATRIB_LAST_INDEX + 5);

        if (cone_root < 0)
        {
            LOG_ERROR("Failed to load cone model!");
        }
        else
        {
            LOG_INFO("Cone model loaded successfully with root index: %d", cone_root);
            glm::mat4 offset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
            offset = glm::rotate(offset, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            offset = glm::scale(offset, glm::vec3(1.5f, 0.5f, 1.5f));
            m_arrow_model->offset_mesh_vertices(
                m_arrow_model->roots[cone_root].flattened_index_start,
                m_arrow_model->roots[cone_root].flattened_index_end,
                offset);
        }

        Arrow::set_model(m_arrow_model.get(), 1);
        m_pointer_arrow = std::make_unique<Arrow>(global_registry, "Pointer_arrow");
        m_pointer_arrow->set_position({ 0.0f, 4.5f, 0.0f });

        // Man
        m_man_model = std::make_unique<game_object_basic_model>();
        m_man_model->import_model_from_file("Models\\Imported\\Wave Hip Hop Dance.fbx", false);
        m_man_model->add_instance_buffer(16, MODEL_ATRIB_LAST_INDEX + 1);
        m_man_model->add_instance_buffer(9, MODEL_ATRIB_LAST_INDEX + 5);

        m_collective.reserve(static_cast<size_t>(MAN_GRID * MAN_GRID));
        Man::set_model(m_man_model.get(), MAN_GRID * MAN_GRID);
        for (int i = 0; i < MAN_GRID; i++)
        {
            for (int j = 0; j < MAN_GRID; j++)
            {
                Man* m = new Man(global_registry,
                    "My_Man_" + std::to_string(i) + "_" + std::to_string(j));
                m->set_position({ 10.0f * i, 0.0f, 10.0f * j });
                m->set_scale({ 0.1f, 0.1f, 0.1f });
                m_collective.push_back(m);
            }
        }
    }

    // -----------------------------------------------------------------------
    void setup_animations()
    {
        m_man_animator = std::make_unique<Animation_manager>();
        m_man_animator->Extract_bones_with_hierarchy(*m_man_model);
        m_man_animator->Extract_skeletal_animations();
    }

    // -----------------------------------------------------------------------
    void setup_events(Input_Manager* input, Window_Manager& win)
    {
        // Left click -> ray-cast select tree
        m_click_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_button_pressed) return;
                const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);
                if (mouse.key.code != GLFW_MOUSE_BUTTON_LEFT) return;
                if (m_ui.is_hovered_ndc()) return;

                entt::entity closest_entity = entt::null;
                float        closest_distance = -1.0f;

                const auto& cfg = m_app.get_window().get_config_ref();
                auto view = global_registry.view<Transform_component, Tag_component>();
                view.each([&](entt::entity entity, Transform_component& transform, Tag_component& tag)
                    {
                        if (tag.tag.find("Tree") == std::string::npos) return;

                        glm::vec3 rayDir = Ray_casting::ScreenToWorldRay(
                            static_cast<float>(mouse.mouse_x),
                            static_cast<float>(mouse.mouse_y),
                            static_cast<unsigned int>(cfg.width),
                            static_cast<unsigned int>(cfg.height),
                            m_camera->projection, m_camera->view);

                        float dist = Ray_casting::ray_sphere_intersection(
                            m_camera->camera_position, rayDir,
                            glm::vec3(transform.position), 3.0f);

                        if (dist > 0.0f && (closest_distance < 0.0f || dist < closest_distance))
                        {
                            closest_distance = dist;
                            closest_entity = entity;
                        }
                    });

                if (closest_entity != entt::null)
                {
                    auto& tag = global_registry.get<Tag_component>(closest_entity);
                    auto& transform = global_registry.get<Transform_component>(closest_entity);

                    LOG_INFO("Selected: %s pos: %.2f, %.2f, %.2f",
                        tag.tag.c_str(),
                        transform.position.x, transform.position.y, transform.position.z);

                    m_selected_entity = closest_entity;
                    m_selected_id = global_registry.get<Id_component>(closest_entity).id;
                    m_pointer_arrow->set_position(glm::vec3(
                        transform.position.x,
                        m_pointer_arrow->get_position().y,
                        transform.position.z));
                    m_info_panel->clear();
                    m_info_panel->set_line(0, "Tag: " + tag.tag);
                    m_info_panel->set_line(1, "Pos X: " + std::to_string(transform.position.x));
                    m_info_panel->set_line(2, "Pos Y: " + std::to_string(transform.position.y));
                    m_info_panel->set_line(3, "Pos Z: " + std::to_string(transform.position.z));
                    m_info_panel->visible = true;
                }
            });
        input->subscribe(Input_channel_names[Mouse_input],
            Event_management::Event_type::Mouse_button_pressed, m_click_receiver);

        // Mouse move -> camera look
        m_camera_trigger = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_moved) return;
                if (!m_camera_control) return;
                const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);
                m_camera->process_mouse_movement(
                    static_cast<float>(mouse.mouse_x_offset),
                    static_cast<float>(mouse.mouse_y_offset),
                    static_cast<float>(m_app.get_input_manager()->mouse_sensitivity));
            });
        input->subscribe(Input_channel_names[Mouse_input],
            Event_management::Event_type::Mouse_moved, m_camera_trigger);

        // Framebuffer / window resize
        m_framebuffer_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type == Event_management::Event_type::Window_framebuffer_resized)
                {
                    const auto& r = static_cast<const Window_framebuffer_resize_event&>(e);
                    m_camera->update_projection(45.0f, r.new_aspect_ratio, 0.1f, 500.0f);
                    m_printer->change_screen_size(r.new_width, r.new_height);
                    m_ui.on_resize(static_cast<unsigned int>(r.new_width),
                        static_cast<unsigned int>(r.new_height));
                }
                if (e.type == Event_management::Event_type::Window_resized)
                {
                    const auto& r = static_cast<const Window_resize_event&>(e);
                    m_ui.on_resize(static_cast<unsigned int>(r.new_width),
                        static_cast<unsigned int>(r.new_height));
                }
            });
        win.subscribe(Event_management::Event_type::Window_framebuffer_resized, m_framebuffer_receiver);
        win.subscribe(Event_management::Event_type::Window_resized, m_framebuffer_receiver);
    }

    // -----------------------------------------------------------------------
    void processInput(float dt)
    {
        GLFWwindow* window = m_app.get_window().get_handle();
        float speed = 0.1f * (dt / static_cast<float>(TARGET_FRAME_TIME));

        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && m_f11_pressable)
        {
            m_app.get_window().toggle_fullscreen();
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_f11_pressable = false;
        }
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE)
            m_f11_pressable = true;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            speed *= 2.0f;

        bool fwd = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool back = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool rght = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool up = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool down = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) m_camera->camera_tilt(-speed * 5.0f);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_camera->camera_tilt(speed * 5.0f);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        m_camera->camera_move(1.0f, speed, fwd, back, left, rght, up, down);
    }
};

// ---------------------------------------------------------------------------
// Entry point  (call from main.cpp)
// ---------------------------------------------------------------------------
inline int run_game()
{
    App::Config cfg;
    cfg.window_config.title = "Application";
    cfg.window_config.width = 1280;
    cfg.window_config.height = 720;
    cfg.enable_vsync = false;

    App app(cfg);
    app.push_layer(std::make_unique<GameLayer>(app));
    app.run();
    return 0;
}