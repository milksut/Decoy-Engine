#pragma once

#include "Globals.h"

#include "Components/HierarchyComponents.h"
#include "Components/TagComponent.h"
#include "Components/TransformComponent.h"

class game_object_basic
{
private:
	entt::entity this_object;
	entt::registry& registry;

	TransformComponent& get_transform_ref()
	{
		return registry.get<TransformComponent>(this_object);
	}

	bool is_pos_changed_flag = false;
	bool is_any_child_pos_changed_flag = false;

	//TODO: renamke this to prevent miss use
	void trigger_child_pos_changed_flag()
	{
		is_any_child_pos_changed_flag = true;
		ParentComponent* comp = registry.try_get<ParentComponent>(this_object);

		if(comp != nullptr)
		{
			if(comp->parent->is_any_child_pos_changed_flag)
				return;
			else
				comp->parent->trigger_child_pos_changed_flag();
		}
	}

	void trigger_pos_changed_flags()
	{
		is_pos_changed_flag = true;
		trigger_child_pos_changed_flag();
	}

	std::shared_ptr<class_region> Transform_region;

	void tick_transforms(const glm::mat4 parent_transform)
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

		ChildComponent& childs = registry.get<ChildComponent>(this_object);

		for (game_object_basic* child : childs.children)
		{
			if (is_pos_changed_flag)
				child->is_pos_changed_flag = true;

			else if (!child->is_any_child_pos_changed_flag)
				continue;

			child->tick_transforms(this_transform.world);
		}

	}
public:

	game_object_basic(entt::registry& registry, const std::string& tag = "Undefined tag", const std::shared_ptr<class_region> Transform_region = nullptr,
		 TransformComponent& transform = TransformComponent(), const game_object_basic* parent_object = nullptr)
		: registry(registry), Transform_region(Transform_region)
	{
		this_object = registry.create();
		
		registry.emplace<TagComponent>(this_object, tag);

		if(parent_object != nullptr)
		{
			registry.emplace<ParentComponent>(this_object, parent_object->this_object);
			registry.get_or_emplace<ChildComponent>(parent_object->this_object).children.push_back(this);
		}

		registry.emplace<Self_component>(this_object, this);

		registry.emplace<TransformComponent>(this_object, transform);
	};

	TransformComponent get_transform_copy()
	{
		return get_transform_ref();
	}

	void static Tick(entt::registry& registry)
	{
		auto group = registry.group<TransformComponent, Self_component>(entt::get<>, entt::exclude<ParentComponent>);

		group.each([](auto entity, TransformComponent& transform, Self_component& self)
		{
			self.this_object->tick_transforms(glm::mat4(1.0f));
		});

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

};