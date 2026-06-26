#pragma once

#include "Layer_manager.h"
#include "Camera_test.h"
#include "game_object_basic.h"
#include "Physics_manager.h"
#include "Animation_manager.h"
#include "Globals.h"
#include "Shader.h"
#include "Input_Manager.h"
#include "Audio_manager.h"
#include "UI_Manager.h"

class Player : public game_object_basic<Player>
{
public:
    Player(entt::registry& reg, const std::string& tag)
        : game_object_basic(reg, tag)
    {
    }
};

class Enemy : public game_object_basic<Enemy>
{
public:
    Enemy(entt::registry& reg, const std::string& tag)
        : game_object_basic(reg, tag)
    {
    }
};

class camera_wrapper : public game_object_basic<camera_wrapper>
{
private:
    camera_test* camera = nullptr;
    Player* man = nullptr;

    float follow_distance = 5.0f;   // How far behind the player
    float height_offset = 2.0f;     // How high above the player
    float look_height = 1.3f;       // height above player feet to look at
    float right_offset = 1.0f;      // How far to the right (over the shoulder)
    float look_ahead = 0.0f;        // How far in front of the player to look
    float min_pitch = -15.0f;
    float max_pitch = 60.0f;

public:
    float yaw = 180.0f;  // start behind the player
    float pitch = 15.0f;  // slight downward angle

    camera_wrapper(entt::registry& reg, const std::string& tag, Player* fallowed_man, camera_test* fallowing_camera)
        : game_object_basic(reg, tag, fallowed_man), camera(fallowing_camera), man(fallowed_man)
    {}

    void rotate(float delta_yaw, float delta_pitch, float sensitivity = 0.15f)
    {
        yaw -= delta_yaw * sensitivity;
        pitch -= delta_pitch * sensitivity;
        pitch = glm::clamp(pitch, min_pitch, max_pitch);
    }

    void user_function_per_tick() override
    {
        if (camera == nullptr || man == nullptr)
            return;

        glm::vec3 player_pos = man->get_position_world();
        glm::vec3 look_target = player_pos + glm::vec3(0.0f, look_height, 0.0f);

        float ry = glm::radians(yaw);
        float rp = glm::radians(pitch);
        
        glm::vec3 offset(
            cos(rp) * cos(ry) * follow_distance,
            sin(rp) * follow_distance,
            cos(rp) * sin(ry) * follow_distance);

        glm::vec3 to_player = glm::normalize(-offset);
        glm::vec3 right = glm::normalize(glm::cross(to_player, glm::vec3(0, 1, 0)));
        glm::vec3 cam_pos = player_pos + offset + right * right_offset;

        camera->update_camera_position(cam_pos);
        camera->look_at(look_target);
    }
};

class Terrain_object : public game_object_basic<Terrain_object>
{
public:
    Terrain_object(entt::registry& reg, const std::string& tag)
        : game_object_basic(reg, tag)
    {}
};

class Guard_demo_layer : public Layer
{
private:
    static constexpr double TARGET_FPS = 144.0;
    static constexpr double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;
    static constexpr double MOUSE_SENSITIVITY = 1.0;

    Event_manager* event_mng = &event_manager;//derived from Layer class

    Physics_manager* phys_mng;
    Window_Manager* window_mng;
    Input_Manager* input_mng;

    Audio_manager* audio_mng = new Audio_manager();
    UI_manager* ui_mng = new UI_manager();

    TextRenderer* printer;

    Shader* shader_base;
    Shader* shader_anim;
    Shader* shader_ui;

    camera_test* camera;//camera need at least 1 window manager to be initlazed,
    //it tr^y's to use glfw contex which is initlized when any window manager crated.
    
    Player* player = new Player(global_registry, "My_Man");
    Enemy* enemy = new Enemy(global_registry, "Other_man");

    camera_wrapper* wrapper;

    Animation_manager* player_anim_mng;//same thing with camera, it needs a glfw contex
    Animation_manager* enemy_anim_mng;

    game_object_basic_model terrain_model;
    Terrain_object* terrain_obj = nullptr;
    JPH::BodyID terrain_body;

    JPH::BodyID player_body;
    JPH::BodyID enemy_body;

    enum class MoveState { Idle, Walk, Run, back, left, right, attack, Jump };
    MoveState current_move_state = MoveState::Idle;

#ifdef JPH_DEBUG_RENDERER
    Jolt_debug_renderer* m_debug_renderer = nullptr;
#endif // JPH_DEBUG_RENDERER

    double time_of_last_frame = 0.0;
    double time_of_last_reset = 0.0;
    unsigned int frame_counter = 0;
    std::string fps_text = "";
    bool punch_connected = false;

    void setup_light(Shader* shader)
    {
        shader->use();
        shader->setInt("num_of_lights", 1);
        shader->setBool("lights[0].has_a_source", false);
        shader->setVec3("lights[0].light_pos", glm::vec3(0.0f));
        shader->setVec3("lights[0].light_target", glm::vec3(0.0f, -1.0f, 0.0f));
        shader->setVec3("lights[0].ambient", glm::vec3(0.6f));
        shader->setVec3("lights[0].diffuse", glm::vec3(1.0f));
        shader->setVec3("lights[0].specular", glm::vec3(0.3f));
        shader->setFloat("lights[0].cos_soft_cut_off_angle", 0.0f);
        shader->setFloat("lights[0].cos_hard_cut_off_angle", 0.0f);
        shader->setFloat("lights[0].constant", 1.0f);
        shader->setFloat("lights[0].linear", 0.0f);
        shader->setFloat("lights[0].quadratic", 0.0f);
    }
public:
    Guard_demo_layer()
    {
        phys_mng = new Physics_manager(event_mng, &global_registry);
        window_mng = new Window_Manager(*event_mng);
        input_mng = new Input_Manager(*event_mng, window_mng->get_handle());
        camera = new camera_test();
        player_anim_mng = new Animation_manager();
        enemy_anim_mng = new Animation_manager();

        window_mng->focus_window();
        window_mng->set_mouse_input_mode(GLFW_CURSOR_DISABLED);

        phys_mng->set_gravity({ 0.0f, -5.0f, 0.0f });

#ifdef JPH_DEBUG_RENDERER
        m_debug_renderer = new Jolt_debug_renderer(camera->Ubo_slot);
#endif // JPH_DEBUG_RENDERER

        printer = new TextRenderer(
            "Textures/Font_texture_Atlas/letter.png",
            "Textures/Font_texture_Atlas/letter.txt",
            window_mng->get_config_ref().width, window_mng->get_config_ref().height, 16, 32,
            "Shaders/Vertex_shaders/Text_render_vertex.vert",
            "Shaders/Fragment_shaders/Text_render_fragment.frag",
            "Shaders/Geometry_shaders/Text_render_geometry.geom",
            0.005f);

        printer->change_deleted_colors(0, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.5f, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
        printer->change_deleted_colors(1, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        printer->push_deleted_colors();

        shader_base = new Shader(
            "Shaders/Vertex_shaders/Loaded_model_vertex.vert",
            "Shaders/Fragment_shaders/Loaded_model_fragment.frag");
        setup_light(shader_base);

        shader_ui = new Shader(
            "Shaders/Vertex_shaders/Ui_vertex.vert",
            "Shaders/Fragment_shaders/Ui_fragment.frag");
        setup_light(shader_ui);

        shader_anim = new Shader(
            "Shaders/Vertex_shaders/Loaded_animation_vertex.vert",
            "Shaders/Fragment_shaders/Loaded_model_fragment.frag");
        setup_light(shader_anim);

        UI_manager::Config ui_config;
        ui_config.screen_width = static_cast<unsigned int>(window_mng->get_config_ref().width);
        ui_config.screen_height = static_cast<unsigned int>(window_mng->get_config_ref().height);
        ui_mng->init(input_mng, ui_config);

        setup_models();
        Player::Tick(global_registry);
        Enemy::Tick(global_registry);
        setup_animations();
        setup_movement_events();
        setup_terrain();
        player_body = create_character_body(phys_mng,player);
        enemy_body = create_character_body(phys_mng,enemy);

        shader_base->bind_UBO("projectionXview_block", camera->Ubo_slot);
        shader_anim->bind_UBO("projectionXview_block", camera->Ubo_slot);
        int anim_ubo = Animation_manager::get_Ubo_slot();
        if (anim_ubo >= 0)
            shader_anim->bind_UBO("Bone_block", anim_ubo);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glfwSetTime(0.0);
        time_of_last_frame = glfwGetTime();
    }

    void onAttach() override {}
    void onDetach() override {}
    void Update() override
    {
        double now = glfwGetTime();
        float  dt = static_cast<float>(now - time_of_last_frame);

        game_object_base::Tick(global_registry);

        input_mng->Poll_keys();
        if (phys_mng->get_linear_velocity(player_body).IsNearZero() && current_move_state != MoveState::attack)
            current_move_state = MoveState::Idle;

        switch (current_move_state)
        {
        case Guard_demo_layer::MoveState::Idle:
            if (!player_anim_mng->IsPlaying("Idle"))
            {
                player_anim_mng->PlayAnimation("Idle");
            }
            break;
        case Guard_demo_layer::MoveState::Walk:
            if (!player_anim_mng->IsPlaying("Slow Run"))
            {
                player_anim_mng->PlayAnimation("Slow Run", false);
            }
            break;
        case Guard_demo_layer::MoveState::Run:
            if (!player_anim_mng->IsPlaying("Fast Run"))
            {
                player_anim_mng->PlayAnimation("Fast Run", false);
            }
            break;
        case Guard_demo_layer::MoveState::Jump:
            if (!player_anim_mng->IsPlaying("Jumping"))
            {
                player_anim_mng->PlayAnimation("Jumping", false);
            }
            break;
        case Guard_demo_layer::MoveState::back:
            if (!player_anim_mng->IsPlaying("Running Backward"))
            {
                player_anim_mng->PlayAnimation("Running Backward", false);
            }
            break;
        case Guard_demo_layer::MoveState::left:
            if (!player_anim_mng->IsPlaying("Standing Run Left"))
            {
                player_anim_mng->PlayAnimation("Standing Run Left", false);
            }
            break;
        case Guard_demo_layer::MoveState::right:
            if (!player_anim_mng->IsPlaying("Standing Run Right"))
            {
                player_anim_mng->PlayAnimation("Standing Run Right", false);
            }
            break;
        case Guard_demo_layer::MoveState::attack:
            if (!player_anim_mng->IsPlaying("Standing Melee Punch"))
            {
                player_anim_mng->PlayAnimation("Standing Melee Punch", false);
            }
            break;
        default:
            break;
        }

        if (current_move_state == MoveState::attack && !punch_connected)
        {
            try_punch();
            punch_connected = true;
        }

        if (current_move_state != MoveState::attack)
            punch_connected = false;


        player_anim_mng->Tick(0.3f * (dt / static_cast<float>(TARGET_FRAME_TIME)));
        enemy_anim_mng->Tick(0.3f * (dt / static_cast<float>(TARGET_FRAME_TIME)));

        glm::vec3 root_delta = player_anim_mng->consume_root_motion_delta();
        root_delta = enemy_anim_mng->consume_root_motion_delta();

        ui_mng->update();

        phys_mng->Tick((dt / static_cast<float>(TARGET_FRAME_TIME)));

        time_of_last_frame = glfwGetTime();

        frame_counter++;
        if (glfwGetTime() - time_of_last_reset >= 1.0)
        {
            glm::vec3 pos = player->get_position_local();
            fps_text = "FPS: " + std::to_string(frame_counter) + "      player pos x:" +
                std::to_string(pos.x) + " y:" + std::to_string(pos.y) + " z:" + std::to_string(pos.z);
            frame_counter = 0;
            time_of_last_reset = glfwGetTime();
            draw_call_count = 0;
        }

    }
    void Render() override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        shader_base->use();
        shader_base->setVec3("viewPos", camera->camera_position);
        Terrain_object::draw(*shader_base);
        
        shader_anim->use();
        shader_anim->setVec3("viewPos", camera->camera_position);

        player_anim_mng->upload_bone_transforms();
        Player::draw(*shader_anim);

        enemy_anim_mng->upload_bone_transforms();
        Enemy::draw(*shader_anim);

#ifdef JPH_DEBUG_RENDERER
        // in Render(), after drawing everything else, before window Tick
        phys_mng->draw_debug(*m_debug_renderer);
#endif // JPH_DEBUG_RENDERER

        Logger::checkGLError("After drawing objects");

        printer->render_text(fps_text, -1.0f, 0.9f, 2.0f);

        ui_mng->render();
        
        window_mng->Tick();
    }

private:

    game_object_basic_model character_model;
    game_object_basic_model enemy_model;
    void setup_models()
    {
        // Player
        character_model.import_model_from_file("Models\\Imported\\Wave Hip Hop Dance.fbx", false);
        character_model.add_instance_buffer(16, MODEL_ATRIB_LAST_INDEX + 1);
        character_model.add_instance_buffer(9, MODEL_ATRIB_LAST_INDEX + 5);

        Player::set_model(&character_model, 1);
        LOG_DEBUG("trying to give region slot %d", player->try_assign_region_slot());

        player->set_position({ 10.0f , 10.0f, 10.0f });
        player->set_scale({ 0.01f, 0.01f, 0.01f });

        wrapper = new camera_wrapper(global_registry, "camera_wrapper", player, camera);

        //Enemy
        enemy_model.import_model_from_file("Models\\Imported\\Bouncing Fight Idle.fbx", false);
        enemy_model.add_instance_buffer(16, MODEL_ATRIB_LAST_INDEX + 1);
        enemy_model.add_instance_buffer(9, MODEL_ATRIB_LAST_INDEX + 5);

        Enemy::set_model(&enemy_model, 1);
        LOG_DEBUG("trying to give region slot %d", enemy->try_assign_region_slot());

        enemy->set_position({ 20.0f , 10.0f, 10.0f });
        enemy->set_scale({ 0.01f, 0.01f, 0.01f });

    }

    void setup_animations()
    {
        player_anim_mng->Extract_bones_with_hierarchy(character_model);
        enemy_anim_mng->Extract_bones_with_hierarchy(enemy_model);

        player_anim_mng->Extract_skeletal_animations("dance");
        enemy_anim_mng->Extract_skeletal_animations("idle");


        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Idle.fbx", "Idle");
        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Slow Run.fbx", "Slow Run");
        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Standing Run Left.fbx", "Standing Run Left");
        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Standing Run Right.fbx", "Standing Run Right");
        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Jumping.fbx", "Jumping");
        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Running Backward.fbx", "Running Backward");
        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Fast Run.fbx", "Fast Run");

        player_anim_mng->Add_skeletal_animation_from_file("Models\\Imported\\Standing Melee Punch.fbx", "Standing Melee Punch");

        player_anim_mng->root_motion_enabled = true;
    }

    Event_management::Event_receiver_shared player_movment_reciver;
    Event_management::Event_receiver_shared mouse_move_receiver;

    void setup_movement_events()
    {
        player_movment_reciver = Event_management::make_receiver([this](const Event_management::Event& e)
        {
                bool relase = false;
                Input_key key;
                if (e.type == Event_management::Keyboard_button_pressed)
                {
                    key = static_cast<const Key_press_event&>(e).key;
                }
                else if (e.type == Event_management::Keyboard_button_hold)
                {
                    key = static_cast<const Key_hold_event&>(e).key;
                }
                else if (e.type == Event_management::Mouse_button_pressed)
                {
                    key = static_cast<const Mouse_button_press_event&>(e).key;
                }
                else if (e.type == Event_management::Mouse_button_hold)
                {
                    key = static_cast<const Mouse_button_hold_event&>(e).key;
                }
                else if (e.type == Event_management::Mouse_button_released)
                {
                    key = static_cast<const Mouse_button_release_event&>(e).key;
                    relase = true;
                }
                else { return; }

                // Derive movement axes from the camera's actual look direction
                glm::vec3 cam_fwd = camera->camera_front;
                cam_fwd.y = 0.0f; //flatten to XZ plane     
                
                if (glm::length(cam_fwd) > 0.001f)
                    cam_fwd = glm::normalize(cam_fwd);

                glm::vec3 cam_right = glm::normalize(glm::cross(cam_fwd, glm::vec3(0.0f, 1.0f, 0.0f)));

                const float move_speed = 0.50f;
                const float jump_speed = 2.0f;

                // Always read current Y so gravity is never cancelled by horizontal input
                float cur_y = phys_mng->get_linear_velocity(player_body).GetY();

                switch (key.code)
                {   
                    case GLFW_MOUSE_BUTTON_1:
                    {
                        current_move_state = relase ? MoveState::Idle : MoveState::attack;
                        break;
                    }
                    case GLFW_KEY_W:
                    {
                        if (current_move_state == MoveState::attack) break;
                        phys_mng->set_linear_velocity(player_body, JPH::Vec3(cam_fwd.x * move_speed, cur_y, cam_fwd.z * move_speed));
                        current_move_state = MoveState::Walk;
                        break;
                    }
                    case GLFW_KEY_S:
                    {
                        if (current_move_state == MoveState::attack) break;
                        phys_mng->set_linear_velocity(player_body, JPH::Vec3(-cam_fwd.x * move_speed, cur_y, -cam_fwd.z * move_speed));
                        current_move_state = MoveState::back;
                        break;
                    }
                    case GLFW_KEY_A:
                    {
                        if (current_move_state == MoveState::attack) break;
                        phys_mng->set_linear_velocity(player_body, JPH::Vec3(-cam_right.x * move_speed, cur_y, -cam_right.z * move_speed));
                        current_move_state = MoveState::left;
                        break;
                    }
                    case GLFW_KEY_D:
                    {
                        if (current_move_state == MoveState::attack) break;
                        phys_mng->set_linear_velocity(player_body, JPH::Vec3(cam_right.x * move_speed, cur_y, cam_right.z * move_speed));
                        current_move_state = MoveState::right;
                        break;
                    }
                    case GLFW_KEY_SPACE:
                    {
                        if (current_move_state == MoveState::attack) break;
                        if (!check_grounded()) break;

                        phys_mng->set_linear_velocity(player_body, JPH::Vec3(phys_mng->get_linear_velocity(player_body).GetX(),
                            jump_speed, phys_mng->get_linear_velocity(player_body).GetZ()));
                        current_move_state = MoveState::Jump;
                        break;
                    }
                    case GLFW_KEY_LEFT_CONTROL:
                    {
                        if (current_move_state == MoveState::attack) break;
                        phys_mng->set_linear_velocity(player_body, { 0.0f, -1.0f, 0.0f });
                        current_move_state = MoveState::Run;
                        break;
                    }
                    case GLFW_KEY_ESCAPE:
                    {
                        window_mng->set_mouse_input_mode(GLFW_CURSOR_NORMAL);
                        break;
                    }
                    case GLFW_KEY_TAB:
                    {   
                        window_mng->focus_window();
                        window_mng->set_mouse_input_mode(GLFW_CURSOR_DISABLED);
                        break;
                    }
                        
                    
                    default:
                        break;
                }
        });

        input_mng->subscribe(Input_channel_names[Keyboard_input], Event_management::Keyboard_button_pressed, player_movment_reciver);
        input_mng->subscribe(Input_channel_names[Keyboard_input], Event_management::Keyboard_button_hold, player_movment_reciver);
        input_mng->subscribe(Input_channel_names[Mouse_input], Event_management::Mouse_button_pressed, player_movment_reciver);
        input_mng->subscribe(Input_channel_names[Mouse_input], Event_management::Mouse_button_hold, player_movment_reciver);
        input_mng->subscribe(Input_channel_names[Mouse_input], Event_management::Mouse_button_released, player_movment_reciver);

        mouse_move_receiver = Event_management::make_receiver([this](const Event_management::Event& e)
        {

            if (e.type != Event_management::Event_type::Mouse_moved)
                return;

            const auto& mouse = static_cast<const Mouse_move_event&>(e);

            wrapper->rotate(
                static_cast<float>(-mouse.mouse_x_offset),
                static_cast<float>(-mouse.mouse_y_offset));
        });
        input_mng->subscribe(Input_channel_names[Mouse_input], Event_management::Event_type::Mouse_moved, mouse_move_receiver);
    }

    void setup_terrain()
    {

        terrain_model.import_model_from_file("Models/Basic/big_plane.fbx", true);
        terrain_model.add_instance_buffer(16, MODEL_ATRIB_LAST_INDEX + 1);
        terrain_model.add_instance_buffer(9, MODEL_ATRIB_LAST_INDEX + 5);

        Terrain_object::set_model(&terrain_model, 1);

        terrain_obj = new Terrain_object(global_registry, "Terrain");
        terrain_obj->try_assign_region_slot();
        // adjust scale/position to match your mesh
        terrain_obj->set_scale(glm::vec3(100.0f));
        terrain_obj->set_position(glm::vec3(0.0f));
        terrain_obj->set_is_using_quat(true);

        // physics
        terrain_body = create_terrain_body();

        JPH::BodyInterface* bi = phys_mng->get_body_interface();
        if (bi && terrain_body != JPH::BodyID())
        {
            // Rotate -90 degrees around the X axis to lay the mesh flat (Z-up to Y-up)
            JPH::Quat rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(-90.0f));

            // Get the current position so we don't move it
            JPH::RVec3 currentPos = bi->GetPosition(terrain_body);

            // Apply the rotation (keeping the position the same)
            bi->SetPositionAndRotation(terrain_body, currentPos, rotation, JPH::EActivation::DontActivate);
        }

        auto& rb = global_registry.emplace<RigidBody_component>(
            terrain_obj->get_entity(),
            terrain_body,
            nullptr,  // event_reciver
            true,     // is_static
            false     // is_trigger
        );
    }

    JPH::BodyID create_terrain_body()
    {
        JPH::TriangleList triangles;

        for (auto& mesh_ptr : terrain_model.Meshes)
        {
            auto& verts = mesh_ptr->main_vertices;
            auto& inds = mesh_ptr->main_indices;

            for (size_t i = 0; i + 2 < inds.size(); i += 3)
            {
                auto& v0 = verts[inds[i]];
                auto& v1 = verts[inds[i + 1]];
                auto& v2 = verts[inds[i + 2]];
                triangles.push_back(JPH::Triangle(
                    JPH::Float3(v0.position[0], v0.position[1], v0.position[2]),
                    JPH::Float3(v1.position[0], v1.position[1], v1.position[2]),
                    JPH::Float3(v2.position[0], v2.position[1], v2.position[2])
                ));
            }
        }

        JPH::MeshShapeSettings settings(std::move(triangles));
        auto result = settings.Create();
        if (result.HasError()) { LOG_ERROR("Terrain: %s", result.GetError().c_str()); return {}; }

        JPH::Shape* scaledShape = new JPH::ScaledShape(result.Get(), JPH::Vec3(100.0f, 100.0f, 100.0f));
        JPH::BodyCreationSettings body_settings(
            scaledShape, JPH::RVec3(0, 0, 0),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static, Object_layers::NON_MOVING);

        return phys_mng->create_body(0, body_settings, JPH::EActivation::DontActivate);
    }

    inline JPH::RVec3 glm_to_rvec3(const glm::vec3& v) {
        return JPH::RVec3(v.x, v.y, v.z);
    }

    JPH::BodyID create_character_body(Physics_manager* phys_mng, game_object_base* player)
    {
        if (!phys_mng || !player) return JPH::BodyID();

        // Read transform from ECS
        Transform_component tf = *(player->get_component<Transform_component>());

        // Choose capsule parameters (tweak to fit your mesh)
        // Use player's scale to approximate world size
        float uniform_scale = (tf.scale.x + tf.scale.y + tf.scale.z) / 3.0f;
        //float capsule_radius = 0.25f * uniform_scale;   // ~25 cm
        //float capsule_height = 1.6f * uniform_scale;    // total height excluding the hemispheres
        //float half_height = capsule_height * 0.5f;      // Jolt expects half-height for many shapes

        // Use fixed capsule size in world units (meters)
        float capsule_radius = 0.25f;   // 25 cm
        float capsule_height = 1.6f;    // 1.6 m total height (excluding hemispheres)
        float half_height = capsule_height * 0.5f;

        // Create capsule shape (half height, radius)
        JPH::Shape* capsule = new JPH::CapsuleShape(half_height, capsule_radius);

        // Position: use the object's world position (so the capsule sits where the mesh is)
        glm::vec3 world_pos = player->get_position_world();
        JPH::RVec3 jpos = glm_to_rvec3(world_pos);

        // Rotation: use object's quaternion if available, otherwise identity
        glm::quat q = player->get_rotation_quat();
        JPH::Quat jrot(q.x, q.y, q.z, q.w); // Jolt uses (x,y,z,w) constructor

        // Body creation settings: dynamic body on MOVING layer
        JPH::BodyCreationSettings settings(
            capsule,
            jpos,
            jrot,
            JPH::EMotionType::Dynamic,
            Object_layers::MOVING
        );

        // Optional: tune friction/restitution via settings.mOverrideMassProperties or contact settings
        // settings.mFriction = 0.6f;
        // settings.mRestitution = 0.0f;

        // Create the body and add it to the physics system (activate so SyncPhysicsToECS will copy transform)
        settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX
            | JPH::EAllowedDOFs::TranslationY
            | JPH::EAllowedDOFs::TranslationZ
            | JPH::EAllowedDOFs::RotationY;

        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = 70.0f;  // realistic 70 kg human

        JPH::BodyID body_id = phys_mng->create_body(player->get_id(), settings, JPH::EActivation::Activate);

        if (body_id == JPH::BodyID())
        {
            LOG_ERROR("Failed to create character body for id %d", player->get_id());
            delete capsule; // cleanup on failure
            return JPH::BodyID();
        }

        // Attach a RigidBody_component to the ECS (match how you did terrain)
        // event receiver nullptr for now; set if you want collision callbacks per-character
        global_registry.emplace<RigidBody_component>(
            player->get_entity(),
            body_id,
            nullptr,   // event_reciver
            false,     // is_static
            false      // is_trigger
        );

        return body_id;
    }

    bool check_grounded() const
    {
        glm::vec3 pos = player->get_position_world();
        const float check_dist = 2.0f;

        JPH::RRayCast ray{
            JPH::RVec3(pos.x, pos.y, pos.z),
            JPH::Vec3(0.0f, -check_dist, 0.0f)  // direction * length baked in
        };

        std::vector<JPH::RayCastResult> hits;
        phys_mng->cast_ray_all(ray, hits);

        for (const auto& hit : hits)
        {
            if (hit.mBodyID != player_body)  // mBodyID, not body_id
                return true;
        }
        return false;
    }

    void try_punch()
    {
        glm::vec3 origin = player->get_position_world() + glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 dir = camera->camera_front;
        dir.y = 0.0f;
        if (glm::length(dir) > 0.001f) dir = glm::normalize(dir);

        const float punch_range = 2.0f;

        JPH::RRayCast ray{
            JPH::RVec3(origin.x, origin.y, origin.z),
            JPH::Vec3(dir.x * punch_range, dir.y * punch_range, dir.z * punch_range)
        };

        std::vector<JPH::RayCastResult> hits;
        phys_mng->cast_ray_all(ray, hits);

        for (const auto& hit : hits)
        {
            if (hit.mBodyID == enemy_body)
            {
                LOG_DEBUG("punch!!");
                glm::vec3 push = glm::normalize(dir + glm::vec3(0.0f, 0.4f, 0.0f)) * 350.0f;
                phys_mng->add_impulse(enemy_body, JPH::Vec3(push.x, push.y, push.z));
                break;
            }
        }
    }
};
