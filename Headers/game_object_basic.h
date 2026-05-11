#pragma once

#include "Globals.h"

#include "game_object_basic_model.h"

#include "Components/HierarchyComponents.h"
#include "Components/TagComponent.h"
#include "Components/TransformComponent.h"



//this class is for calling transform upload with each derived classes own static class region
class game_object_base
{
public:

	virtual void set_is_pos_changed_flag(bool val) = 0;
	virtual bool get_is_pos_changed_flag() = 0;
	virtual void set_is_any_child_pos_changed_flag(bool val) = 0;
	virtual bool get_is_any_child_pos_changed_flag() = 0;
	virtual void tick_transforms(const glm::mat4) = 0;
	virtual void trigger_child_pos_changed_flag() = 0;

void static Tick(entt::registry& registry)
{
	auto group = registry.group<TransformComponent, Self_component>(entt::get<>, entt::exclude<ParentComponent>);

	group.each([](auto entity, TransformComponent& transform, Self_component& self)
	{
		static_cast<game_object_base*>(self.this_object)->tick_transforms(glm::mat4(1.0f));
	});

	upload_all_transforms();

}

protected:
	static std::vector<std::function<void()>>& get_registry()
	{
		static std::vector<std::function<void()>> registry;
		return registry;
	}

	static void register_upload_transform(std::function<void()> fn)
	{
		get_registry().push_back(std::move(fn));
	}

	static void upload_all_transforms()
	{
		for (auto& fn : get_registry())
			fn();
	}
};

template <typename Derived>
class game_object_basic : public game_object_base
{
private:
	struct auto_register
	{
		auto_register()
		{
			game_object_base::register_upload_transform([]() { Derived::tick_upload_transforms();});
		}
	};

	static inline auto_register registerer{};
	
	static inline game_object_basic_model* model = nullptr;
	static inline std::shared_ptr<class_region> region = nullptr;
	static inline int attrib_index = -1;   // VAO attrib slot for world mat4 (e.g. 3)

	static void tick_upload_transforms()
	{
		if (!model || !region || attrib_index < 0)
			return;

		const int region_size = static_cast<int>(region->object_ptrs.size());

		std::vector<glm::mat4> staging;
		staging.reserve(region_size); // avoid reallocations mid-loop

		int batch_start = -1; // index in object_ptrs where current batch began

		auto flush_batch = [&]()
		{
			if (batch_start == -1 || staging.empty())
				return;

			// offset_in_numbers is the region's start in the full VBO
			// batch_start is our local index within the region
			int vbo_offset = region->offset_in_numbers + batch_start;

			model->load_instance_buffer(
				reinterpret_cast<float*>(staging.data()),  // mat4 data
				static_cast<unsigned int>(staging.size()), // number of mat4s
				attrib_index,
				region,
				static_cast<unsigned int>(batch_start)           // offset within region
			);

			staging.clear();
			batch_start = -1;
		};

		for (int i = 0; i < region_size; i++)
		{
			void* ptr = region->object_ptrs[i];

			if (ptr == nullptr)
			{
				flush_batch();
				continue;
			}

			// Cast void* back to game_object_basic — safe because only
			// game_object_basic instances ever write into object_ptrs
			game_object_basic* obj = static_cast<game_object_basic*>(ptr);

			if (!obj->is_pos_changed_flag)
			{
				flush_batch();
				continue;
			}

			// Pull world matrix from EnTT
			TransformComponent& tc = obj->registry.get<TransformComponent>(obj->this_object);

			if (batch_start == -1)
				batch_start = i; // start a new batch here

			staging.push_back(tc.world);
			obj->is_pos_changed_flag = false; // reset flag after queuing for upload
		}

		flush_batch(); // flush any trailing batch
	}

	//-----------------------------------------------------------------------------------------

	entt::entity this_object;
	entt::registry& registry;

	int region_slot_index = -1; // this objects slot in region->object_ptrs

	TransformComponent& get_transform_ref()
	{
		return registry.get<TransformComponent>(this_object);
	}

	static game_object_basic* from_region_ptr(void* ptr)
	{
		return static_cast<game_object_basic<Derived>*>(ptr);
	}

protected:

	bool is_pos_changed_flag = false;
	bool is_any_child_pos_changed_flag = false;

	//TODO: renamke this to prevent miss use
	void trigger_child_pos_changed_flag() override
	{
		is_any_child_pos_changed_flag = true;
		ParentComponent* comp = registry.try_get<ParentComponent>(this_object);

		if(comp != nullptr)
		{
			if(static_cast<game_object_base*>(comp->parent)->get_is_any_child_pos_changed_flag())
				return;
			else
				static_cast<game_object_base*>(comp->parent)->trigger_child_pos_changed_flag();
		}
	}

	void trigger_pos_changed_flags()
	{
		is_pos_changed_flag = true;
		trigger_child_pos_changed_flag();
	}

	void set_is_pos_changed_flag(bool val) override
	{
		is_pos_changed_flag = val;
	}

	bool get_is_pos_changed_flag() override
	{
		return is_pos_changed_flag;
	}

	void set_is_any_child_pos_changed_flag(bool val) override
	{
		is_any_child_pos_changed_flag = val;
	}

	bool get_is_any_child_pos_changed_flag() override
	{
		return is_any_child_pos_changed_flag;
	}


	void tick_transforms(const glm::mat4 parent_transform) override
	{
		TransformComponent& this_transform = get_transform_ref();

		if(is_pos_changed_flag)
		{
		
			// Local transform
			glm::mat4 local = glm::mat4(1.0f);
			local = glm::translate(local, this_transform.position);
			local = glm::rotate(local, glm::radians(this_transform.rotation.x), glm::vec3(1, 0, 0));
			local = glm::rotate(local, glm::radians(this_transform.rotation.y), glm::vec3(0, 1, 0));
			local = glm::rotate(local, glm::radians(this_transform.rotation.z), glm::vec3(0, 0, 1));
			local = glm::scale(local, this_transform.scale);

			// World Transform
			this_transform.world = parent_transform * local;

		}

		ChildComponent* childs = registry.try_get<ChildComponent>(this_object);

		if (childs != nullptr)
		{
			for (void* child_ptr : childs->children)
			{
				game_object_base* child = static_cast<game_object_base*>(child_ptr);

				if (is_pos_changed_flag)
					child->set_is_pos_changed_flag(true);

				else if (!child->get_is_any_child_pos_changed_flag())
					continue;

				child->tick_transforms(this_transform.world);
			}
		}
		
		set_is_any_child_pos_changed_flag(false);

	}
	
	game_object_basic(entt::registry& registry, const std::string& tag = "Undefined tag",
		game_object_basic* parent_object = nullptr, TransformComponent& transform = TransformComponent())
		: registry(registry)
	{
		this_object = registry.create();
		
		registry.emplace<Self_component>(this_object, static_cast<void*>(this));
		
		registry.emplace<TagComponent>(this_object, tag);

		registry.emplace<TransformComponent>(this_object) = transform;
		
		if(parent_object != nullptr)
		{
			registry.emplace<ParentComponent>(this_object, static_cast<void*>(parent_object));
			registry.get_or_emplace<ChildComponent>(parent_object->this_object).children.push_back(static_cast<void*>(this));
		}

		if(region != nullptr)
		{
			for(int i=0; i<region->object_ptrs.size(); ++i)
			{
				if(region->object_ptrs[i] == nullptr)
				{
					region->object_ptrs[i] = this;
					region_slot_index = i;
					break;
				}
			}
			if(region_slot_index < 0)
			{
				if(region->object_ptrs.size() < region->size_in_number)
				{
					region->object_ptrs.push_back(this);
					region_slot_index = region->object_ptrs.size() - 1;
				}
				else
				{
					LOG_ERROR("Game_object_basic, There is not enougf space in region, object Cant be Created.");
				}
			}
		}

		trigger_pos_changed_flags();
	};

	~game_object_basic()
	{
		if (region && region_slot_index >= 0)
			region->object_ptrs[region_slot_index] = nullptr;
	}

public:

	TransformComponent get_transform_copy()
	{
		return get_transform_ref();
	}

	//---set transforms-----------------------------------------------------------------------
	void set_position(const glm::vec3& new_pos)
	{
		get_transform_ref().position = new_pos;
		trigger_pos_changed_flags();
	}

	void set_rotation(const glm::vec3& new_rot)
	{
		get_transform_ref().rotation = new_rot;
		trigger_pos_changed_flags();
	}

	void set_scale(const glm::vec3& new_scale)
	{
		get_transform_ref().scale = new_scale;
		trigger_pos_changed_flags();
	}

	//---update transforms------------------------------------------------------------------------
	void move(const glm::vec3& delta_pos)
	{
		get_transform_ref().position += delta_pos;
		trigger_pos_changed_flags();
	}

	void rotate(const glm::vec3& delta_rot)
	{
		get_transform_ref().rotation += delta_rot;
		trigger_pos_changed_flags();
	}

	void scale(const glm::vec3& delta_scale)
	{
		get_transform_ref().scale *= delta_scale;
		trigger_pos_changed_flags();
	}

	//--- upload/change model--------------------------------------------------------------------

	static void set_model(game_object_basic_model* new_model ,const unsigned int region_size ,const int transform_attrib_index = 3)
	{
		model = new_model;

		if (new_model != nullptr)
		{
			region = model->reserve_class_region(region_size);
			region->object_ptrs.assign(region_size, nullptr);
			attrib_index = transform_attrib_index;
		}
		else
		{
			region = nullptr;
			attrib_index = -1;
		}
	}

	//you can call it with a negative to make it smaller
	static int expand_region(const int additional_size)
	{
		if (model == nullptr || region == nullptr)
		{
			LOG_WARNING("Game_object_basic: cant expand region, there is no defined model or region!");
			return -1;
		}
		if(additional_size + region->size_in_number < 0)
		{
			LOG_WARNING("Game_object_basic: cant shrink region below 0!");
			return -1;
		}
			
		model->reserve_additional_region(additional_size + region->size_in_number, region);
		return additional_size + region->size_in_number;
	}

	//--- draw -----------------------------------------------------------------------------------
	static int draw(Shader& shader, int amount = -1)
	{
		if (model == nullptr)
		{
			LOG_WARNING("Game_object_basic: cant draw, there is no defined model!");
			return -1;
		}

		if(amount < 0)
			amount = region->size_in_number;

		else if (amount > region->size_in_number)
		{
			LOG_WARNING("Game_object_basic: draw amount is bigger than region size, drawing only region size!");
			amount = region->size_in_number;
		}

		model->draw(shader, region, amount);
		return 0;
	}
};