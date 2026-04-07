#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

#include <glm/glm.hpp>

class VulkanStorageBuffer : public QRenderObject
{
public:
	VulkanStorageBuffer(class VulkanDevice* vulkanDeviceRef, class VulkanPhysicalDevice* vulkanPhysicalDeviceRef);
	virtual ~VulkanStorageBuffer() override;

	void create() override;

	const vk::raii::Buffer* getFieldXYZBufferInst() const { return &fieldXYZBufferInst; }
	const vk::raii::Buffer* getFieldWBufferInst() const { return &fieldWBufferInst; }
	const vk::raii::Buffer* getVelocityXYZBufferInst() const { return &velocityXYZBufferInst; }
	const vk::raii::Buffer* getVelocityWBufferInst() const { return &velocityWBufferInst; }
	const vk::raii::Buffer* getAccumCounterBufferInst() const { return &accumCounterBufferInst; }
	const vk::raii::Buffer* getNormalBufferInst() const { return &normalBufferInst; }
	const vk::raii::Buffer* getReflectedFieldXYZBufferInst() const { return &reflectedFieldXYZBufferInst; }

	static constexpr uint32_t PLANE_COUNT = 3;
	vk::DeviceSize getFieldXYZBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4; } // float3은 std430에서 16바이트 정렬
	vk::DeviceSize getFieldWBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float); }
	vk::DeviceSize getVelocityXYZBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4; }
	vk::DeviceSize getVelocityWBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float); }
	vk::DeviceSize getAccumCounterBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float); }
	vk::DeviceSize getNormalBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4; } // float4 (vec3 + padding)
	vk::DeviceSize getReflectedFieldXYZBufferSize() const { return static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4; } // float3 std430 16바이트 정렬
	uint32_t getGridSize() const { return GRID_SIZE; }

	void markStaticObject(const glm::vec3& worldPosition, const glm::vec3& reflectSpectrum, float roughness, const glm::vec3& normal);
	void clearGrid();

private:
	uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
	void createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory, void*& mapped);

	const class VulkanDevice* vulkanDeviceRef = nullptr;
	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;

	static constexpr uint32_t GRID_SIZE = 256;
	static constexpr uint32_t GRID_TOTAL = GRID_SIZE * GRID_SIZE;
	static constexpr float CELL_SIZE = 1.0f;

	vk::raii::Buffer fieldXYZBufferInst = nullptr;
	vk::raii::DeviceMemory fieldXYZMemoryInst = nullptr;
	void* fieldXYZMappedInst = nullptr;

	vk::raii::Buffer fieldWBufferInst = nullptr;
	vk::raii::DeviceMemory fieldWMemoryInst = nullptr;
	void* fieldWMappedInst = nullptr;

	vk::raii::Buffer velocityXYZBufferInst = nullptr;
	vk::raii::DeviceMemory velocityXYZMemoryInst = nullptr;
	void* velocityXYZMappedInst = nullptr;

	vk::raii::Buffer velocityWBufferInst = nullptr;
	vk::raii::DeviceMemory velocityWMemoryInst = nullptr;
	void* velocityWMappedInst = nullptr;

	vk::raii::Buffer accumCounterBufferInst = nullptr;
	vk::raii::DeviceMemory accumCounterMemoryInst = nullptr;
	void* accumCounterMappedInst = nullptr;

	vk::raii::Buffer normalBufferInst = nullptr;
	vk::raii::DeviceMemory normalMemoryInst = nullptr;
	void* normalMappedInst = nullptr;

	vk::raii::Buffer reflectedFieldXYZBufferInst = nullptr;
	vk::raii::DeviceMemory reflectedFieldXYZMemoryInst = nullptr;
	void* reflectedFieldXYZMappedInst = nullptr;
};
