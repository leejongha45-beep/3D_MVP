#pragma once

#include <glm/glm.hpp>

struct CameraData
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 forward;
	alignas(16) glm::vec3 up;
	float fov;
	float _pad0[3];
};

struct PlaneData
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 color;
	float size;
	float _pad0[3];
};

struct LightData
{
	alignas(16) glm::vec3 direction;
	float frequency;
	float propagationSpeed;
	float damping;
	float time;
	float _pad0;
};

struct SceneData
{
	CameraData camera;
	PlaneData plane;
	LightData light;
};

struct VertexData
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 reflectSpectrum;
	float roughness;
	float _pad0[3];
};

static_assert(sizeof(CameraData) == 64, "CameraData size mismatch");
static_assert(sizeof(PlaneData) == 64, "PlaneData size mismatch");
static_assert(sizeof(LightData) == 32, "LightData size mismatch");
static_assert(sizeof(SceneData) == 160, "SceneData size mismatch");
static_assert(sizeof(VertexData) == 48, "VertexData size mismatch");
