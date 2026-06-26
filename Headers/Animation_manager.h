#pragma once

#include "Globals.h"
#include "game_object_basic.h"

#include "Components/Animation_components.h"


//works with 1 model per manager, so it can track bone ids and their corresponding animation data, and upload them to shader with UBO
//if you want to have different animation with same model, you again need another manager, this also means you can't draw them in same call
//you can manage this by giving each a different class region
class Animation_manager
{
private:
	static inline entt::registry animation_registry;
	static inline unsigned int Bone_UBO = 0;
	static inline int Ubo_slot = -1;

	game_object_basic_model* current_model = nullptr;

public:
	static int get_Ubo_slot()
	{
		return Ubo_slot;
	}

	class Bone : public game_object_basic<Bone>
	{
	public:
		int bone_id = -1;
		std::string bone_name = "";
		glm::mat4 offset = glm::mat4(1.0f);

		Bone(const int id, const std::string& name,const glm::mat4& offset_in, Bone* parent = nullptr)
			: game_object_basic(animation_registry, name, parent), bone_id(id), bone_name(name), offset(offset_in)
		{}
		
	};

private:
	std::map<std::string, std::shared_ptr<Bone>> bone_map;

	Bone* root_bone = nullptr;
	glm::vec3 last_root_pos = glm::vec3(0.0f);
	glm::vec3 pending_root_delta = glm::vec3(0.0f);

public:

	bool root_motion_enabled = false;

	void set_root_bone(const std::string& name)
	{
		root_bone = get_bone_by_name(name);
		if (root_bone)
		{
			last_root_pos = root_bone->get_transform_copy().position;
		}
	}

	glm::vec3 consume_root_motion_delta()
	{
		if (!root_motion_enabled || root_bone == nullptr)
			return glm::vec3(0.0f);

		glm::vec3 delta = pending_root_delta;
		pending_root_delta = glm::vec3(0.0f);
		return delta;
	}

	Bone* get_bone_by_name(const std::string& name)
	{
		auto it = bone_map.find(name);
		if (it != bone_map.end())
			return it->second.get();
		return nullptr;
	}

	void Extract_bones_with_hierarchy(game_object_basic_model& model, Bone* parent = nullptr, const aiNode* node = nullptr)
	{
		if (node == nullptr)
			node = model.last_scene_pointer->mRootNode;

		if (node == nullptr)
			return;

		std::string name = node->mName.C_Str();
		Bone* current = parent;

		// Only create a Bone for nodes that are actual skeleton bones
		if (model.Bone_info_map.count(name))
		{
			auto b = std::make_shared<Bone>(model.Bone_info_map[name].id, name + "_" + std::to_string(reinterpret_cast<uintptr_t>(this)),
				model.Bone_info_map[name].offset, parent);
			current = b.get();
			bone_map[name] = b;
			
			if (parent == nullptr)
				set_root_bone(name);
		}

		

		for (unsigned int i = 0; i < node->mNumChildren; i++)
			Extract_bones_with_hierarchy( model, current, node->mChildren[i]);

		current_model = &model;
	}

	void upload_bone_transforms()
	{
		std::vector<glm::mat4> mats(MAX_BONES, glm::mat4(1.0f));

		for (auto& [name, bone] : bone_map)
		{
			int id = bone->bone_id;
			if (id < 0 || id >= MAX_BONES) continue;

			glm::mat4 world = bone->get_transform_copy().world;
			glm::mat4 offset = bone->offset;

			mats[id] = current_model->global_inverse_transform * world * offset;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, Bone_UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, MAX_BONES * sizeof(glm::mat4), mats.data());
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}



	class Bone_animation
	{
	public:
		double animation_length = 0.0;
		Bone* bone = nullptr;
		entt::entity this_object = entt::null;

		Bone_animation(Bone* effected_bone, std::map<double, glm::vec3>& positions,
			std::map<double, glm::quat>& rotations, std::map<double, glm::vec3>& scales)
			: bone(effected_bone)
		{
			this_object = animation_registry.create();
			animation_registry.emplace<Position_frames_component>(this_object, positions);
			animation_registry.emplace<Rotation_frames_component>(this_object, rotations);
			animation_registry.emplace<Scale_frames_component>(this_object, scales);

			if (!positions.empty()) animation_length = std::max(animation_length, positions.rbegin()->first);
			if (!rotations.empty()) animation_length = std::max(animation_length, rotations.rbegin()->first);
			if (!scales.empty())    animation_length = std::max(animation_length, scales.rbegin()->first);
		}

		Bone_animation(Bone* effected_bone, const aiNodeAnim* channel)
			: bone(effected_bone)
		{

			this_object = animation_registry.create();

			std::map<double, glm::vec3> positions;
			for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i)
			{
				positions[channel->mPositionKeys[i].mTime] = glm::vec3(channel->mPositionKeys[i].mValue.x, 
					channel->mPositionKeys[i].mValue.y, channel->mPositionKeys[i].mValue.z);
			}
				
			std::map<double, glm::quat> rotations;
			for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i)
			{
				rotations[channel->mRotationKeys[i].mTime] = glm::quat(channel->mRotationKeys[i].mValue.w, 
					channel->mRotationKeys[i].mValue.x, channel->mRotationKeys[i].mValue.y, channel->mRotationKeys[i].mValue.z);
			}
				
			std::map<double, glm::vec3> scales;
			for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i)
			{
				scales[channel->mScalingKeys[i].mTime] = glm::vec3(channel->mScalingKeys[i].mValue.x, 
					channel->mScalingKeys[i].mValue.y, channel->mScalingKeys[i].mValue.z);
			}
				
			animation_registry.emplace<Position_frames_component>(this_object, positions);
			animation_registry.emplace<Rotation_frames_component>(this_object, rotations);
			animation_registry.emplace<Scale_frames_component>(this_object, scales);

			if (!positions.empty()) animation_length = std::max(animation_length, positions.rbegin()->first);
			if (!rotations.empty()) animation_length = std::max(animation_length, rotations.rbegin()->first);
			if (!scales.empty())    animation_length = std::max(animation_length, scales.rbegin()->first);
		}

		std::map<double, glm::vec3> get_position_frames() const
		{
			if (animation_registry.valid(this_object))
				return animation_registry.get<Position_frames_component>(this_object).positions;
			return {};
		}

		std::map<double, glm::quat> get_rotation_frames() const
		{
			if (animation_registry.valid(this_object))
				return animation_registry.get<Rotation_frames_component>(this_object).rotations;
			return {};
		}

		std::map<double, glm::vec3> get_scale_frames() const
		{
			if (animation_registry.valid(this_object))
				return animation_registry.get<Scale_frames_component>(this_object).scales;
			return {};
		}

		~Bone_animation()
		{
			if (animation_registry.valid(this_object))
				animation_registry.destroy(this_object);
		}
		Bone_animation(const Bone_animation&) = delete;
		Bone_animation& operator=(const Bone_animation&) = delete;
		Bone_animation(Bone_animation&&) = default;
		Bone_animation& operator=(Bone_animation&&) = default;
	};
private:
	std::vector<std::unique_ptr<Bone_animation>> bone_animations;
public:

	struct Bone_animation_entry
	{
	public:
		Bone_animation* animation = nullptr;
		double start_time = 0.0;
		double speed = 1.0;
		bool loop = false;
	};



	class Skeletal_animation
	{
	public:
		std::multimap<double, Bone_animation_entry> animations;
		double	animation_length = 0.0;
		double	current_time = 0.0;

		void add_animation(Bone_animation* animation, double start_time, double speed = 1.0, bool loop = false)
		{
			animations.insert({ start_time, {animation, start_time, speed, loop} });
			animation_length = std::max(animation_length, start_time + animation->animation_length);
		}

		void add_animation(Bone_animation_entry entry)
		{
			animations.insert({ entry.start_time, entry });
			animation_length = std::max(animation_length, entry.start_time + entry.animation->animation_length);
		}

		bool update(double delta_seconds, bool loop_clip = true)
		{
			bool did_loop = false;
			current_time += delta_seconds;
			if (loop_clip && animation_length > 0.0)
			{
				if (current_time >= animation_length)
					did_loop = true;
				current_time = std::fmod(current_time, animation_length);
			}
			else
				current_time = std::min(current_time, animation_length);

			// upper_bound(current_time) stops at the first track that hasn't
			// started yet — everything before it is active or past its start
			auto end = animations.upper_bound(current_time);
			for (auto it = animations.begin(); it != end; ++it)
				evaluate(it->second);

			return did_loop;
		}

	private:
		void evaluate(const Bone_animation_entry& entry)
		{
			if (!entry.animation || !entry.animation->bone) return;
			if (current_time < entry.start_time) return;

			// Convert parent time to this track's local time
			double local_time = (current_time - entry.start_time) * entry.speed;

			if (entry.loop && entry.animation->animation_length > 0.0)
				local_time = std::fmod(local_time, entry.animation->animation_length);
			else
				local_time = std::min(local_time, entry.animation->animation_length);

			Bone_animation& ba = *entry.animation;

			glm::vec3 pos = sample_vec3(ba.get_position_frames(), local_time);
			glm::quat rot = sample_quat(ba.get_rotation_frames(), local_time);
			glm::vec3 sca = sample_vec3(ba.get_scale_frames(), local_time);

			ba.bone->set_position(pos);
			ba.bone->set_rotation_quat(rot);
			ba.bone->set_is_using_quat(true);
			ba.bone->set_scale(sca);

		}

		static glm::vec3 sample_vec3(const std::map<double, glm::vec3>& frames, double t)
		{
			if (frames.empty())  return glm::vec3(0.0f);
			auto next = frames.lower_bound(t);
			if (next == frames.end())   return frames.rbegin()->second;
			if (next == frames.begin()) return next->second;
			auto   prev = std::prev(next);
			double dt = next->first - prev->first;
			float  fac = (dt > 0.0) ? (float)((t - prev->first) / dt) : 0.0f;
			return glm::mix(prev->second, next->second, fac);
		}

		static glm::quat sample_quat(const std::map<double, glm::quat>& frames, double t)
		{
			if (frames.empty())  return glm::identity<glm::quat>();
			auto next = frames.lower_bound(t);
			if (next == frames.end())   return frames.rbegin()->second;
			if (next == frames.begin()) return next->second;
			auto   prev = std::prev(next);
			double dt = next->first - prev->first;
			float  fac = (dt > 0.0) ? (float)((t - prev->first) / dt) : 0.0f;
			return glm::normalize(glm::slerp(prev->second, next->second, fac));
		}
	};

	std::map<std::string, std::unique_ptr<Skeletal_animation>> skeletal_animations;
	std::vector<std::string> active_animations;

	
	void Extract_skeletal_animations(std::string name = "")
	{

		extract_animation(current_model->last_scene_pointer, name);

	}


	//TODO:add param
	void Add_skeletal_animation_from_file(std::string path, std::string name = "", bool flip_uvs = false)
	{
		Assimp::Importer scene_importer;

		const aiScene* scene = scene_importer.ReadFile(path, aiProcess_Triangulate
			| (flip_uvs ? aiProcess_FlipUVs : 0u) | aiProcess_CalcTangentSpace);


		extract_animation(scene, name);

	}
private:
	//to avoid duplication
	//TODO: extrall al animation, not the first one
	void extract_animation(const aiScene* scene_pointer, std::string name = "")
	{
		if (scene_pointer->mNumAnimations > 0)
		{
			const aiAnimation* anim = scene_pointer->mAnimations[0];

			// Create a skeletal animation clip and add it to the manager
			auto clip = std::make_unique<Skeletal_animation>();

			for (unsigned int i = 0; i < anim->mNumChannels; i++)
			{
				const aiNodeAnim* channel = anim->mChannels[i];
				std::string bone_name = channel->mNodeName.C_Str();

				// inside the for loop over anim->mNumChannels, before the `if (get_bone_by_name(bone_name) == nullptr) continue;`
				LOG_INFO("Animation channel[%u] name='%s' (checking bone map)...", i, bone_name.c_str());
				auto* found = get_bone_by_name(bone_name);
				if (!found) {
					LOG_WARNING("  Channel '%s' has no matching bone in Bone_info_map", bone_name.c_str());
				}
				else {
					LOG_DEBUG("  Channel '%s' -> bone id=%d", bone_name.c_str(), found->bone_id);
				}

				// Only process channels that map to actual bones we extracted
				if (get_bone_by_name(bone_name) == nullptr)
					continue;

				// Bone_animation's second constructor does the aiNodeAnim parsing for you
				auto bone_anim = std::make_unique<Bone_animation>(found, channel);

				clip->add_animation(bone_anim.get(), 0.0, 1.0, true); // start=0, speed=1, loop=true

				// Store it so it doesn't get destroyed
				bone_animations.push_back(std::move(bone_anim));

			}

			std::string clip_name = name.empty() ? anim->mName.C_Str() : name;
			if (clip_name.empty()) clip_name = "clip_0";

			skeletal_animations[clip_name] = std::move(clip);
			active_animations.push_back(clip_name);

			LOG_INFO("Loaded animation: %s", clip_name.c_str());
		}
		else
		{
			LOG_WARNING("No animations found in model!");
		}
	}
public:

	//TODO: make registery tick seperatet from this tick so it didnt get called every time this one called and onlly caled 1 per game tick
	void Tick(double delta_seconds, bool loop_clip = true)
	{

		bool any_looped = false;
		for (const std::string& name : active_animations)
			if (skeletal_animations[name]->update(delta_seconds, loop_clip))  // now returns bool
				any_looped = true;

		// Root motion: read the keyframe position BEFORE zeroing,
		// accumulate delta, then zero BEFORE hierarchy propagation.
		if (root_motion_enabled && root_bone)
		{
			glm::vec3 current_pos = root_bone->get_transform_copy().position;  // raw keyframe value

			if (!any_looped)
				pending_root_delta += current_pos - last_root_pos;
			// On loop: skip accumulation — avoids the backward teleport jump

			last_root_pos = current_pos;  // track keyframe value for next frame's delta

			// Zero XZ so the mesh stays at the physics body's origin
			glm::vec3 zeroed = current_pos;
			zeroed.x = 0.0f;
			zeroed.z = 0.0f;
			root_bone->set_position(zeroed);
		}

		Tick_animation_registry();   // propagates hierarchy WITH the zeroed root

	}

	//use this if you change animation time or bone position, to update the animation transforms
	//otherwise, this will be called with Tick() function anyway, so you can just change animation time or bone position and it will be updated in the next frame
	static void Tick_animation_registry()
	{
		game_object_base::Tick(animation_registry);

	}

	Animation_manager()
	{
		if (Ubo_slot == -1)
		{
			glGenBuffers(1, &Bone_UBO);
			glBindBuffer(GL_UNIFORM_BUFFER, Bone_UBO);
			glBufferData(GL_UNIFORM_BUFFER,
				MAX_BONES * sizeof(glm::mat4),
				nullptr, GL_DYNAMIC_DRAW);

			Ubo_slot = Ubo_slots::get_first_empty_slot();
			Ubo_slots::bind_ubo_to_slot(Bone_UBO, Ubo_slot);
			LOG_INFO("Animation_manager initialized. UBO slot: %d", Ubo_slot);
		}
	}

	~Animation_manager()
	{
		if (Bone_UBO != 0)
		{
			glDeleteBuffers(1, &Bone_UBO);
			Bone_UBO = 0;
			LOG_INFO("Animation_manager destroyed. Bone UBO deleted.");
		}
	}

	//TODO: ai code - reformat
	void PlayAnimation(const std::string& name, bool loop = true, double speed = 1.0)
	{
		auto it = skeletal_animations.find(name);
		if (it == skeletal_animations.end())
		{
			LOG_WARNING("Animation_manager::PlayAnimation - clip '%s' not found", name.c_str());
			return;
		}

		// If already playing exactly this clip, just update speed/loop and return
		if (!active_animations.empty() && active_animations[0] == name)
		{
			return;
		}

		// Replace active animations with this single clip
		active_animations.clear();
		active_animations.push_back(name);

		// Reset clip time so it starts from beginning
		it->second->current_time = 0.0;
		it->second->animation_length = it->second->animation_length; // no-op but keeps semantics

		if (root_bone)
			last_root_pos = root_bone->get_transform_copy().position;

		pending_root_delta = glm::vec3(0.0f);
	}

	void StopAnimation(const std::string& name)
	{
		active_animations.erase(std::remove(active_animations.begin(), active_animations.end(), name),
			active_animations.end());
	}

	bool IsPlaying(const std::string& name) const
	{
		return !active_animations.empty() && active_animations[0] == name;
	}

	bool IsFinished(const std::string& name) const
	{
		if (!IsPlaying(name)) 
			return false;

		auto it = skeletal_animations.find(name);
		if (it == skeletal_animations.end())
			return false;
		return it->second->current_time >= it->second->animation_length;
	}
};