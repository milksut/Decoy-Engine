#pragma once

#include <Globals.h>

struct Position_frames_component
{
	std::map<double, glm::vec3> positions;
};

struct Rotation_frames_component
{
	std::map<double, glm::quat> rotations;
};

struct Scale_frames_component
{
	std::map<double, glm::vec3> scales;
};
