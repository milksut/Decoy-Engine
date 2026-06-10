#pragma once

#include "Layer.h"
#include "App.h"
#include "Camera_test.h"
#include "game_object_basic.h"
#include "Globals.h"
#include "Shader.h"
#include "TextRenderer.h"
#include "ray_casting.h"
#include "UI_Manager.h"


class Tree : public game_object_basic<Tree>
{
public:
    Tree(entt::registry& reg, const std::string& tag, game_object_basic* parent = nullptr)
        : game_object_basic(reg, tag, parent) {
    }
};

class Arrow : public game_object_basic<Arrow>
{
public:
    Arrow(entt::registry& reg, const std::string& tag)
        : game_object_basic(reg, tag) {
    }
};


class GameLayer : public Decoy::Layer
{
public:
    int grid_amount = 100;

private:
    App* m_app = nullptr;

    camera_test* m_camera = nullptr;
    TextRenderer* m_printer = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_ui_shader = nullptr;
    game_object_basic_model m_tree_model;
    game_object_basic_model m_arrow_model;

    UI_manager              m_ui_manager;
    Text_panel* m_info_panel = nullptr;

    Arrow* m_pointer_arrow = nullptr;

    std::unordered_map<unsigned int, Tree*> m_tree_map;
    unsigned int            m_selected_id = 0;
    entt::entity            m_selected_entity = entt::null;

    Event_management::Event_receiver_shared m_click_receiver;
    Event_management::Event_receiver_shared m_camera_receiver;
    Event_management::Event_receiver_shared m_resize_receiver;

    bool   m_camera_control = false;
    bool   m_reverse_arrow_anim = false;
    double m_fps_timer = 0.0;
    int    m_frame_count = 0;
    int    m_last_fps = 0;
    std::string m_fps_text = "";

    bool m_pressable = true;

    void delete_selected_tree()
    {
        if (m_selected_id == 0) return;

        auto it = m_tree_map.find(m_selected_id);
        if (it == m_tree_map.end()) return;

        Tree* t = it->second;
        auto  region = Tree::get_class_region();
        int   deleted_slot = t->get_region_slot_index();
        int   last_slot = (int)region->object_ptrs.size() - 1;

        while (last_slot >= 0 && region->object_ptrs[last_slot] == nullptr)
            last_slot--;

        if (last_slot >= 0 && last_slot != deleted_slot)
        {
            game_object_basic<Tree>* last_obj =
                static_cast<game_object_basic<Tree>*>(region->object_ptrs[last_slot]);
            last_obj->move_to_slot(deleted_slot);
            glm::mat4 world = last_obj->get_transform_copy().world;
            m_tree_model.load_instance_buffer(
                reinterpret_cast<float*>(&world), 1, 3,
                region, (unsigned int)deleted_slot);
        }

        delete t;
        m_tree_map.erase(it);
        m_selected_entity = entt::null;
        m_selected_id = 0;
    }

    void add_tree_at_crosshair()
    {
        auto& cfg = m_app->get_window().get_config_ref();

        glm::vec3 ray_dir = Ray_casting::ScreenToWorldRay(
            (float)cfg.width / 2.0f, (float)cfg.height / 2.0f,
            cfg.width, cfg.height,
            m_camera->projection, m_camera->view);

        glm::vec3 pos = Ray_casting::ray_plane_intersection(
            m_camera->camera_position, ray_dir, 0.0f);

        if (pos == glm::vec3(0.0f)) return;

        Tree* t = new Tree(global_registry,
            "Tree_added_" + std::to_string(m_tree_map.size()));
        t->set_position(pos);
        m_tree_map[t->get_id()] = t;
    }

    void process_keyboard_input(float camera_speed)
    {
        GLFWwindow* win = m_app->get_window().get_handle();

        if (glfwGetKey(win, GLFW_KEY_F11) == GLFW_PRESS && m_pressable)
        {
            m_app->get_window().toggle_fullscreen();
            m_pressable = false;
        }
        if (glfwGetKey(win, GLFW_KEY_F11) == GLFW_RELEASE)
            m_pressable = true;

        if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera_speed *= 2.0f;

        bool fwd = glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS;
        bool back = glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS;
        bool left = glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS;
        bool right = glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS;
        bool up = glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool down = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
            m_camera->camera_tilt(-camera_speed * 5.0f);
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
            m_camera->camera_tilt(camera_speed * 5.0f);

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            m_app->get_window().set_mouse_input_mode(GLFW_CURSOR_NORMAL);
        if (glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS)
            m_app->get_window().set_mouse_input_mode(GLFW_CURSOR_DISABLED);

        m_camera->camera_move(1.0f, camera_speed, fwd, back, left, right, up, down);
    }

public:
    explicit GameLayer(App* app)
        : Decoy::Layer("GameLayer"), m_app(app) {}

    ~GameLayer() override
    {
        delete m_camera;
        delete m_printer;
        delete m_shader;
        delete m_ui_shader;
    }

    void onAttach() override
    {
        auto& win_cfg = m_app->get_window().get_config_ref();
        Input_Manager* input = m_app->get_input_manager();

        m_camera = new camera_test(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(270.0f, 0.0f, 0.0f));
        m_camera->update_projection(45.0f, win_cfg.aspect_ratio, 0.1f, 500.0f);

        m_printer = new TextRenderer(
            "Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.png",
            "Textures/Font_texture_Atlas/DejaVu Sans Mono_512X256_16x32.txt",
            win_cfg.width, win_cfg.height, 16, 32,
            "Shaders/Vertex_shaders/Text_render_vertex.vert",
            "Shaders/Fragment_shaders/Text_render_fragment.frag",
            "Shaders/Geometry_shaders/Text_render_geometry.geom",
            0.005f);

        m_printer->change_deleted_colors(0,
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.5f, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
        m_printer->change_deleted_colors(1,
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        m_printer->push_deleted_colors();

        // ---- Shaders ----
        m_shader = new Shader(
            "Shaders/Vertex_shaders/Loaded_model_vertex.vert",
            "Shaders/Fragment_shaders/Loaded_model_fragment.frag");

        m_ui_shader = new Shader(
            "Shaders/Vertex_shaders/Ui_vertex.vert",
            "Shaders/Fragment_shaders/Ui_fragment.frag");

        // ---- Light ----
        Light sun = { false,
            glm::vec3(0.0f), glm::vec3(0.0f,-1.0f,0.0f),
            glm::vec3(1.0f,1.0f,1.0f), glm::vec3(5.0f,5.0f,5.0f), glm::vec3(0.5f,0.5f,0.5f),
            0,0,0,0,0 };

        m_shader->use();
        m_shader->setInt("num_of_lights", 1);
        m_shader->setBool("lights[0].has_a_source", sun.has_a_source);
        m_shader->setVec3("lights[0].light_pos", sun.light_pos);
        m_shader->setVec3("lights[0].light_target", sun.light_target);
        m_shader->setVec3("lights[0].ambient", sun.ambient);
        m_shader->setVec3("lights[0].diffuse", sun.diffuse);
        m_shader->setVec3("lights[0].specular", sun.specular);
        m_shader->setFloat("lights[0].cos_soft_cut_off_angle", sun.cos_soft_cut_off_angle);
        m_shader->setFloat("lights[0].cos_hard_cut_off_angle", sun.cos_hard_cut_off_angle);
        m_shader->setFloat("lights[0].constant", sun.constant);
        m_shader->setFloat("lights[0].linear", sun.linear);
        m_shader->setFloat("lights[0].quadratic", sun.quadratic);
        m_shader->bind_UBO("projectionXview_block", m_camera->Ubo_slot);

        // ---- Models ----
        m_tree_model.import_model_from_file("Models\\Tree1.obj");
        m_tree_model.add_instance_buffer(16, 3);
        m_tree_model.add_instance_buffer(9, 7);

        Tree::set_model(&m_tree_model, grid_amount * grid_amount, 3);

        for (int i = 0; i < grid_amount; i++)
            for (int j = 0; j < grid_amount; j++)
            {
                Tree* t = new Tree(global_registry,
                    "Tree_" + std::to_string(i) + "_" + std::to_string(j));
                t->set_position({ i * 5.0f, 0.0f, j * 5.0f });
                m_tree_map[t->get_id()] = t;
            }

        m_arrow_model.import_model_from_file("Models\\Cylinder.obj");
        int root_index = m_arrow_model.import_model_from_file("Models\\Cone.obj");
        m_arrow_model.add_instance_buffer(16, 3);
        m_arrow_model.add_instance_buffer(9, 7);

        if (root_index >= 0)
        {
            glm::mat4 offset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
            offset = glm::rotate(offset, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            offset = glm::scale(offset, glm::vec3(1.5f, 0.5f, 1.5f));
            m_arrow_model.offset_mesh_vertices(
                m_arrow_model.roots[root_index].flattened_index_start,
                m_arrow_model.roots[root_index].flattened_index_end, offset);
        }

        Arrow::set_model(&m_arrow_model, 1);
        m_pointer_arrow = new Arrow(global_registry, "Pointer_arrow");
        m_pointer_arrow->set_position({ 0.0f, 4.5f, 0.0f });

        // ---- UI ----
        UI_manager::Config ui_cfg;
        ui_cfg.screen_width = win_cfg.width;
        ui_cfg.screen_height = win_cfg.height;
        m_ui_manager.init(input, ui_cfg);

        Button* del_btn = new Button(
            glm::vec2(-0.95f, 0.0f), glm::vec2(0.3f, 0.2f), "Sil",
            [this]() { delete_selected_tree(); },
            m_ui_shader, m_printer);
        del_btn->set_color({ 0.0f, 0.6f, 5.2f, 1.0f });
        del_btn->set_text_scale(2.3f);
        m_ui_manager.add_widget(del_btn);

        Button* add_btn = new Button(
            glm::vec2(-0.95f, 0.5f), glm::vec2(0.3f, 0.2f), "Ekle",
            [this]() { add_tree_at_crosshair(); },
            m_ui_shader, m_printer);
        add_btn->set_text_scale(2.3f);
        add_btn->set_color({ 0.0f, 5.6f, 0.2f, 1.0f });
        m_ui_manager.add_widget(add_btn);

        m_info_panel = new Text_panel(
            glm::vec2(0.5f, -0.9f), glm::vec2(0.4f, 0.3f),
            m_ui_shader, m_printer);
        m_info_panel->visible = false;
        m_info_panel->set_background_color({ 0.15f, 0.15f, 0.15f, 0.9f });
        m_info_panel->set_text_scale(0.75f);
        m_info_panel->set_line_spacing(0.08f);
        m_ui_manager.add_widget(m_info_panel);


        m_click_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_button_pressed) return;
                const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);
                if (mouse.key.code != GLFW_MOUSE_BUTTON_LEFT) return;
                if (m_ui_manager.is_hovered_ndc()) return;

                auto& cfg = m_app->get_window().get_config_ref();
                entt::entity closest_entity = entt::null;
                float closest_dist = -1.0f;

                auto view = global_registry.view<Transform_component, Tag_component>();
                view.each([&](entt::entity entity,
                    Transform_component& transform, Tag_component& tag)
                    {
                        if (tag.tag.find("Tree") == std::string::npos) return;

                        glm::vec3 ray_dir = Ray_casting::ScreenToWorldRay(
                            (float)mouse.mouse_x, (float)mouse.mouse_y,
                            cfg.width, cfg.height,
                            m_camera->projection, m_camera->view);

                        float dist = Ray_casting::ray_sphere_intersection(
                            m_camera->camera_position, ray_dir,
                            glm::vec3(transform.position), 3.0f);

                        if (dist > 0 && (closest_dist < 0 || dist < closest_dist))
                        {
                            closest_dist = dist;
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

        m_camera_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_moved) return;
                if (!m_camera_control) return;
                const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);
                m_camera->process_mouse_movement(
                    (float)mouse.mouse_x_offset,
                    (float)mouse.mouse_y_offset,
                    (float)m_app->get_input_manager()->mouse_sensitivity);
            });
        input->subscribe(Input_channel_names[Mouse_input],
            Event_management::Event_type::Mouse_moved, m_camera_receiver);

        m_resize_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type == Event_management::Event_type::Window_framebuffer_resized)
                {
                    const auto& r = static_cast<const Window_framebuffer_resize_event&>(e);
                    m_camera->update_projection(45.0f, r.new_aspect_ratio, 0.1f, 500.0f);
                    m_printer->change_screen_size(r.new_width, r.new_height);
                    m_ui_manager.on_resize(r.new_width, r.new_height);
                }
                if (e.type == Event_management::Event_type::Window_resized)
                {
                    const auto& r = static_cast<const Window_resize_event&>(e);
                    m_ui_manager.on_resize(r.new_width, r.new_height);
                }
            });
        m_app->get_window().subscribe(
            Event_management::Event_type::Window_framebuffer_resized, m_resize_receiver);
        m_app->get_window().subscribe(
            Event_management::Event_type::Window_resized, m_resize_receiver);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    void onDetach() override
    {
        for (auto& [id, tree] : m_tree_map)
            delete tree;
        m_tree_map.clear();

        delete m_pointer_arrow;
        m_pointer_arrow = nullptr;
    }

    void onUpdate(float dt) override
    {
        Input_Manager* input = m_app->get_input_manager();
        Key_state rmb = input->Get_key_state({ Mouse_input, GLFW_MOUSE_BUTTON_RIGHT });
        m_camera_control = (rmb == Pressed || rmb == Hold);

        process_keyboard_input(0.1f * dt * 144.0f);

        m_ui_manager.update();

        m_fps_timer += dt;
        m_frame_count++;
        if (m_fps_timer >= 1.0)
        {
            m_last_fps = m_frame_count;
            m_frame_count = 0;
            m_fps_timer -= 1.0;
            m_fps_text = "FPS: " + std::to_string(m_last_fps);
            draw_call_count = 0;
            m_reverse_arrow_anim = !m_reverse_arrow_anim;
        }

        m_pointer_arrow->rotate(glm::vec3(0.0f, 0.005f, 0.0f));
        m_pointer_arrow->move(glm::vec3(0.0f, m_reverse_arrow_anim ? 0.001f : -0.001f, 0.0f));

        Frustum frustum = Ray_casting::extract_frustum(*m_camera);

        std::vector<unsigned int> visible_list;
        std::vector<int>          available_indices;

        std::vector<void*>& region = Tree::get_class_region()->object_ptrs;
        unsigned int        region_size = (unsigned int)region.size();

        visible_list.reserve(region_size * 10);
        available_indices.reserve(region_size);

        auto view = global_registry.view<
            Transform_component, Id_component, Tag_component, World_AABB_component>();

        view.each([&](auto, Transform_component&, Id_component& id_c,
            Tag_component& tag_c, World_AABB_component& aabb_c)
            {
                if (tag_c.tag.find("Tree") == std::string::npos) return;
                if (Ray_casting::aabb_in_frustum(frustum, aabb_c.aabb))
                    visible_list.push_back(id_c.id);
            });

        unsigned int visible_amount = (unsigned int)visible_list.size();

        for (unsigned int i = 0; i < visible_amount && i < region_size; i++)
        {
            game_object_base* ptr = static_cast<game_object_base*>(region[i]);
            if (ptr == nullptr) { available_indices.push_back(i); continue; }

            unsigned int temp_id = ptr->get_id();
            auto index = std::find(visible_list.begin(), visible_list.end(), temp_id);
            if (index != visible_list.end()) { *index = 0; continue; }
            else { available_indices.push_back(i); continue; }
        }

        int idx = 0;
        for (unsigned int id : visible_list)
        {
            if (idx >= (int)available_indices.size()) break;
            if (id == 0) continue;

            int new_index = available_indices[idx++];
            void* obj_ptr = region[new_index];

            if (obj_ptr == nullptr)
                Global_object_map::get_object(id)->use_null_region_pos(new_index);
            else
                static_cast<game_object_base*>(obj_ptr)->swap_region_pos(id);
        }

        Tree::max_region_upload_index = visible_amount;
        game_object_base::Tick(global_registry);

        m_visible_amount = visible_amount;
    }

    void onRender() override
    {
        m_shader->use();
        m_shader->setVec3("viewPos", m_camera->camera_position);

        if (m_visible_amount > 0)
            Tree::draw(*m_shader, m_visible_amount);
        Arrow::draw(*m_shader);

        Logger::checkGLError("After drawing objects");

        m_printer->render_text(m_fps_text, -1.0f, 0.9f, 2.0f);

        m_ui_manager.render();
    }

private:
    unsigned int m_visible_amount = 0;
};