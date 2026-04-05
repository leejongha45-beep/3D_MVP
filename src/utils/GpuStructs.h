#pragma once

#include <glm/glm.hpp>

struct CameraData
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 target;
	alignas(16) glm::vec3 up;
	float fov;
};

struct PlaneData
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 color;
	float size;
};

struct LightData
{
	alignas(16) glm::vec3 direction;
	alignas(16) glm::vec3 color;
	float intensity;
	float ambient;
};

struct SceneData
{
	CameraData camera;
	PlaneData plane;
	LightData light;
};
