#pragma once

#include "Layer_manager.h"
#include "Layers/App.h"
#include "Camera_test.h"
#include "game_object_basic.h"
#include "Physics_manager.h"
#include "Block_word.h"
#include "Globals.h"
#include "Shader.h"
#include "UI_Manager.h"
#include "TextRenderer.h"
#include "Audio_manager.h"

// ---------------------------------------------------------------------------
class Block : public game_object_basic<Block>
{
public:
    Block(entt::registry& reg, const std::string& tag)
        : game_object_basic(reg, tag)
    {
    }
};

// ---------------------------------------------------------------------------
class MinecraftLayer : public Layer
{
    static constexpr float  PLAYER_HEIGHT = 1.8f;
    static constexpr float  PLAYER_RADIUS = 0.3f;
    static constexpr float  PLAYER_SPEED = 5.0f;
    static constexpr float  JUMP_VELOCITY = 4.43f;
    static constexpr float  MOUSE_SENSITIVITY = 0.1f;
    static constexpr float  REACH = 10.0f;
    static constexpr double TARGET_FPS = 144.0;
    static constexpr double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

public:

    MinecraftLayer(App& app)
        : Layer("MinecraftLayer")
        , m_app(app)
    {
    }

    void onAttach() override
    {
        Window_Manager& win = m_app.get_window();
        Input_Manager* input = m_app.get_input_manager();
        const auto& cfg = win.get_config_ref();

        glfwSetInputMode(win.get_handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        m_camera = std::make_unique<camera_test>(
            glm::vec3(5.0f, 5.0f, 5.0f),
            glm::vec3(225.0f, -30.0f, 0.0f)
        );
        m_camera->update_projection(70.0f, cfg.aspect_ratio, 0.05f, 500.0f);

        // Shader
        m_shader = std::make_unique<Shader>(
            "Shaders/Vertex_shaders/Loaded_model_vertex.vert",
            "Shaders/Fragment_shaders/Loaded_model_fragment.frag"
        );

        m_ui_shader = std::make_unique<Shader>(
            "Shaders/Vertex_shaders/Ui_vertex.vert",
            "Shaders/Fragment_shaders/Ui_fragment.frag"
        );

        m_physics = std::make_unique<Physics_manager>(nullptr, &global_registry);

        Audio_manager::Config audio_cfg;
        audio_cfg.master_volume = 1.0f;
        audio_cfg.max_sources = 32;
        m_audio = std::make_unique<Audio_manager>(audio_cfg);

        m_break_sound_buffer = m_audio->load_wav("Sounds\\click.wav");
        m_place_sound_buffer = m_audio->load_wav("Sounds\\click.wav");

        // BlockWorld
        m_world = std::make_unique<BlockWorld<Block>>(global_registry, m_physics.get());

        m_block_model = std::make_unique<game_object_basic_model>();
        m_block_model->import_model_from_file("Models\\Imported\\Grass_Block.obj", false);
        m_block_model->add_instance_buffer(16, MODEL_ATRIB_LAST_INDEX + 1);
        m_block_model->add_instance_buffer(9, MODEL_ATRIB_LAST_INDEX + 5);

        Block::set_model(m_block_model.get(), 4096);

        generate_flat_world(25, 25);

        setup_player();

        // UBO
        m_shader->bind_UBO("projectionXview_block", m_camera->Ubo_slot);
        setup_light();

        setup_events(input, win);

        m_ui.init(m_app.get_input_manager());

        m_crosshair = new Button(
            glm::vec2(-0.01f, -0.01f),
            glm::vec2(0.01f, 0.01f),
            "",
            []() {},
            m_ui_shader.get(),
            &m_text_renderer
        );

        m_crosshair->set_text_scale(1.5f);
        m_ui.add_widget(m_crosshair);

        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glfwSetTime(0.0);
        m_last_frame = glfwGetTime();
    }

    void onDetach() override
    {
        delete m_crosshair;
        m_crosshair = nullptr;

        m_world->clear();
        if (m_physics && !m_player_body.IsInvalid())
            m_physics->delete_body(m_player_body);
    }

    void Update() override
    {
        m_ui.update();
        double now = glfwGetTime();
        float  dt = static_cast<float>(now - m_last_frame);
        m_last_frame = now;

        game_object_base::Tick(global_registry);

        process_input(dt);
        m_physics->Tick(dt);
        sync_camera_to_player();

        m_audio->update_listener(
            m_camera->camera_position,
            m_camera->camera_front,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        m_audio->cleanup_finished_sources();

        m_frame_count++;
        if (now - m_fps_timer >= 1.0)
        {
            m_fps = m_frame_count;
            m_frame_count = 0;
            m_fps_timer = now;
        }
    }

    void Render() override
    {
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
        m_shader->use();
        m_shader->setVec3("viewPos", m_camera->camera_position);

        auto region = Block::get_class_region();
        unsigned int draw_count = 0;
        if (region)
        {
            for (int i = (int)region->object_ptrs.size() - 1; i >= 0; i--)
            {
                if (region->object_ptrs[i] != nullptr)
                {
                    draw_count = (unsigned int)(i + 1);
                    break;
                }
            }
        }

        Block::draw(*m_shader, draw_count);
        m_ui.render();
    }

private:
    App& m_app;

    std::unique_ptr<camera_test>             m_camera;
    std::unique_ptr<Shader>                  m_shader;
    std::unique_ptr<Physics_manager>         m_physics;
    std::unique_ptr<BlockWorld<Block>>       m_world;
    std::unique_ptr<game_object_basic_model> m_block_model;
    std::unique_ptr<Shader>                  m_ui_shader;
    std::unique_ptr<Audio_manager>           m_audio;

    ALuint m_break_sound_buffer = 0;
    ALuint m_place_sound_buffer = 0;

    JPH::BodyID m_player_body;

    TextRenderer m_text_renderer{
    "Textures/Font_texture_Atlas/letter.png",
    "Textures/Font_texture_Atlas/letter.txt",
    1280, 720,
    16, 16,
    "Shaders/Vertex_shaders/Text_render_vertex.vert",
    "Shaders/Fragment_shaders/Text_render_fragment.frag",
    "Shaders/Geometry_shaders/Text_render_geometry.geom"
    };

    UI_manager m_ui;
    Button* m_crosshair = nullptr;

    double m_last_frame = 0.0;
    int    m_frame_count = 0;
    int    m_fps = 0;
    double m_fps_timer = 0.0;

    bool m_left_clicked = false;
    bool m_right_clicked = false;

    bool m_f11_pressable = true;
    int  m_placed_count = 0;

    Event_management::Event_receiver_shared m_mouse_move_receiver;
    Event_management::Event_receiver_shared m_mouse_click_receiver;
    Event_management::Event_receiver_shared m_resize_receiver;

    // -----------------------------------------------------------------------
    void generate_flat_world(int size_x, int size_z)
    {
        int idx = 0;
        for (int x = 0; x < size_x; x++)
        {
            for (int z = 0; z < size_z; z++)
            {
                glm::ivec3 pos(x, 0, z);
                m_world->place_block(pos, BlockType::Dirt,
                    [&](const glm::ivec3& p) -> Block*
                    {
                        Block* b = new Block(global_registry, "Block_" + std::to_string(idx));
                        b->set_position(glm::vec3(p.x + 0.5f, p.y + 0.5f, p.z + 0.5f));
                        b->set_scale(glm::vec3(0.5f));
                        return b;
                    });
                idx++;
            }
        }
        LOG_INFO("Generated %zu blocks", m_world->block_count());
    }

    // -----------------------------------------------------------------------
    void setup_player()
    {
        JPH::BodyInterface* bi = m_physics->get_body_interface();

        JPH::CapsuleShapeSettings* capsule_settings = new JPH::CapsuleShapeSettings(
            PLAYER_HEIGHT * 0.5f - PLAYER_RADIUS, PLAYER_RADIUS);

        JPH::ShapeSettings::ShapeResult shape_result = capsule_settings->Create();
        if (shape_result.HasError())
        {
            LOG_ERROR("Capsule shape creation failed!");
            capsule_settings->Release();
            return;
        }

        JPH::RefConst<JPH::Shape> shape = shape_result.Get();
        capsule_settings->Release();

        JPH::BodyCreationSettings body_settings(
            shape,
            JPH::RVec3(5.0f, 5.0f, 5.0f),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            Object_layers::MOVING
        );

        body_settings.mAngularDamping = 1.0f;
        body_settings.mLinearDamping = 0.0f;
        body_settings.mGravityFactor = 1.0f;
        body_settings.mAllowedDOFs =
            JPH::EAllowedDOFs::TranslationX |
            JPH::EAllowedDOFs::TranslationY |
            JPH::EAllowedDOFs::TranslationZ;

        m_player_body = bi->CreateAndAddBody(body_settings, JPH::EActivation::Activate);

        if (m_player_body.IsInvalid())
            LOG_ERROR("Player body creation failed!");
        else
            LOG_INFO("Player body created successfully.");
    }

    // -----------------------------------------------------------------------
    void setup_light()
    {
        m_shader->use();
        m_shader->setInt("num_of_lights", 1);
        m_shader->setBool("lights[0].has_a_source", false);
        m_shader->setVec3("lights[0].light_pos", glm::vec3(0.0f));
        m_shader->setVec3("lights[0].light_target", glm::vec3(0.0f, -1.0f, 0.0f));
        m_shader->setVec3("lights[0].ambient", glm::vec3(0.8f));
        m_shader->setVec3("lights[0].diffuse", glm::vec3(1.0f));
        m_shader->setVec3("lights[0].specular", glm::vec3(0.3f));
        m_shader->setFloat("lights[0].cos_soft_cut_off_angle", 0.0f);
        m_shader->setFloat("lights[0].cos_hard_cut_off_angle", 0.0f);
        m_shader->setFloat("lights[0].constant", 1.0f);
        m_shader->setFloat("lights[0].linear", 0.0f);
        m_shader->setFloat("lights[0].quadratic", 0.0f);
    }

    // -----------------------------------------------------------------------
    void setup_events(Input_Manager* input, Window_Manager& win)
    {
        m_mouse_move_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_moved) return;
                const auto& mouse = dynamic_cast<const Mouse_move_event&>(e);
                m_camera->process_mouse_movement(
                    static_cast<float>(mouse.mouse_x_offset),
                    static_cast<float>(mouse.mouse_y_offset),
                    MOUSE_SENSITIVITY);
            });
        input->subscribe(Input_channel_names[Mouse_input],
            Event_management::Event_type::Mouse_moved, m_mouse_move_receiver);

        m_mouse_click_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Mouse_button_pressed) return;
                const auto& mouse = dynamic_cast<const Mouse_button_press_event&>(e);
                if (mouse.key.code == GLFW_MOUSE_BUTTON_LEFT)  m_left_clicked = true;
                if (mouse.key.code == GLFW_MOUSE_BUTTON_RIGHT) m_right_clicked = true;
            });
        input->subscribe(Input_channel_names[Mouse_input],
            Event_management::Event_type::Mouse_button_pressed, m_mouse_click_receiver);

        // Resize
        m_resize_receiver = Event_management::make_receiver(
            [this](const Event_management::Event& e)
            {
                if (e.type != Event_management::Event_type::Window_framebuffer_resized) return;
                const auto& r = static_cast<const Window_framebuffer_resize_event&>(e);
                m_camera->update_projection(70.0f, r.new_aspect_ratio, 0.05f, 500.0f);

                m_text_renderer.change_screen_size(
                    r.new_width,
                    r.new_height
                );

                m_ui.on_resize(
                    r.new_width,
                    r.new_height
                );
            });

        win.subscribe(Event_management::Event_type::Window_framebuffer_resized, m_resize_receiver);
    }

    // -----------------------------------------------------------------------
    void process_input(float dt)
    {
        GLFWwindow* window = m_app.get_window().get_handle();
        JPH::BodyInterface* bi = m_physics->get_body_interface();

        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += m_camera->camera_front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= m_camera->camera_front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= m_camera->camera_right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += m_camera->camera_right;
        move.y = 0.0f;
        if (glm::length(move) > 0.001f)
            move = glm::normalize(move) * PLAYER_SPEED;

        JPH::Vec3 cur_vel = bi->GetLinearVelocity(m_player_body);
        bi->SetLinearVelocity(m_player_body, JPH::Vec3(move.x, cur_vel.GetY(), move.z));

        bool on_ground = false;
        {
            JPH::RVec3 player_rvec = bi->GetCenterOfMassPosition(m_player_body);
            JPH::Vec3 player_pos_jolt((float)player_rvec.GetX(), (float)player_rvec.GetY(), (float)player_rvec.GetZ());
            JPH::RRayCast ground_ray(
                JPH::RVec3(player_pos_jolt.GetX(), player_pos_jolt.GetY(), player_pos_jolt.GetZ()),
                JPH::Vec3(0.0f, -(PLAYER_HEIGHT * 0.5f + 0.15f), 0.0f)
            );
            AllBroadPhaseFilter bp;
            AllObjectLayerFilter ol;
            std::vector<JPH::RayCastResult> ground_hits;
            if (m_physics->cast_ray_all(ground_ray, ground_hits, bp, ol))
            {
                for (auto& gh : ground_hits)
                    if (!(gh.mBodyID == m_player_body)) { on_ground = true; break; }
            }
        }

        static bool space_was_pressed = false;
        bool space_now = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space_now && !space_was_pressed && on_ground)
            bi->SetLinearVelocity(m_player_body, JPH::Vec3(cur_vel.GetX(), JUMP_VELOCITY, cur_vel.GetZ()));
        space_was_pressed = space_now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && m_f11_pressable)
        {
            m_app.get_window().toggle_fullscreen();
            m_f11_pressable = false;
        }
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE)
            m_f11_pressable = true;


        if (m_left_clicked)
        {
            m_left_clicked = false;

            auto hit = m_world->raycast(m_camera->camera_position, m_camera->camera_front, REACH, m_player_body);
            if (hit)
            {
                bool removed = m_world->remove_block(hit->block_pos);
                if (removed)
                {
                    LOG_INFO("Block removed at %d %d %d",
                        hit->block_pos.x, hit->block_pos.y, hit->block_pos.z);

                    glm::vec3 sound_pos = glm::vec3(hit->block_pos) + glm::vec3(0.5f);
                    m_audio->play_oneshot_3d("Sounds/click.wav", sound_pos);
                }
                else
                    LOG_WARNING("remove_block failed at %d %d %d",
                        hit->block_pos.x, hit->block_pos.y, hit->block_pos.z);
            }
            else
            {
                LOG_INFO("Left click: no block hit");
            }
        }


        if (m_right_clicked)
        {
            m_right_clicked = false;

            auto hit = m_world->raycast(m_camera->camera_position, m_camera->camera_front, REACH, m_player_body);
            if (hit)
            {
                glm::ivec3 new_pos = hit->block_pos + hit->normal;

                glm::vec3 player_center = m_camera->camera_position - glm::vec3(0, PLAYER_HEIGHT * 0.4f, 0);
                glm::ivec3 player_grid = glm::ivec3(glm::floor(player_center));

                if (new_pos == player_grid || new_pos == player_grid + glm::ivec3(0, 1, 0))
                {
                    LOG_INFO("Right click: block placement blocked (inside player)");
                }
                else
                {
                    bool placed = m_world->place_block(new_pos, BlockType::Dirt,
                        [&](const glm::ivec3& p) -> Block*
                        {
                            Block* b = new Block(global_registry,
                                "Block_placed_" + std::to_string(m_placed_count++));
                            b->set_position(glm::vec3(p.x + 0.5f, p.y + 0.5f, p.z + 0.5f));
                            b->set_scale(glm::vec3(0.5f));
                            return b;
                        });

                    if (placed)
                    {
                        LOG_INFO("Block placed at %d %d %d", new_pos.x, new_pos.y, new_pos.z);

                        glm::vec3 sound_pos = glm::vec3(new_pos) + glm::vec3(0.5f);
                        m_audio->play_oneshot_3d("Sounds/click.wav", sound_pos);
                    }
                    else
                        LOG_INFO("Right click: position already occupied at %d %d %d",
                            new_pos.x, new_pos.y, new_pos.z);
                }
            }
            else
            {
                LOG_INFO("Right click: no block hit");
            }
        }
    }

    // -----------------------------------------------------------------------
    void sync_camera_to_player()
    {
        JPH::BodyInterface* bi = m_physics->get_body_interface();
        JPH::RVec3          pos = bi->GetCenterOfMassPosition(m_player_body);
        m_camera->update_camera_position(glm::vec3(
            static_cast<float>(pos.GetX()),
            static_cast<float>(pos.GetY()) + PLAYER_HEIGHT * 0.4f,
            static_cast<float>(pos.GetZ())
        ));
    }
};

// ---------------------------------------------------------------------------
inline int run_minecraft()
{
    App::Config cfg;
    cfg.window_config.title = "Minecraft";
    cfg.window_config.width = 1280;
    cfg.window_config.height = 720;
    cfg.enable_vsync = false;

    App app(cfg);
    app.push_layer(std::make_unique<MinecraftLayer>(app));
    app.run();
    return 0;
}