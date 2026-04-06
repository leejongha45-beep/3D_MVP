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

	const vk::raii::Buffer* getFieldBufferInst() const { return &fieldBufferInst; }
	const vk::raii::Buffer* getVelocityBufferInst() const { return &velocityBufferInst; }
	static constexpr uint32_t PLANE_COUNT = 3;
	vk::DeviceSize getFieldBufferSize() const { return GRID_TOTAL * PLANE_COUNT * sizeof(float) * 4; }
	vk::DeviceSize getVelocityBufferSize() const { return GRID_TOTAL * PLANE_COUNT * sizeof(float) * 4; }
	uint32_t getGridSize() const { return GRID_SIZE; }

	void markStaticObject(const glm::vec3& worldPosition, const glm::vec3& reflectSpectrum, float roughness);
	void clearGrid();

private:
	uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
	void createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory, void*& mapped);
	uint32_t worldToGridIndex(const glm::vec3& worldPosition) const;

	const class VulkanDevice* vulkanDeviceRef = nullptr;
	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;

	static constexpr uint32_t GRID_SIZE = 1024;
	static constexpr uint32_t GRID_TOTAL = GRID_SIZE * GRID_SIZE;
	static constexpr uint32_t PLANE_COUNT_INTERNAL = 3; // xz, xy, yz

	vk::raii::Buffer fieldBufferInst = nullptr;
	vk::raii::DeviceMemory fieldMemoryInst = nullptr;
	void* fieldMappedInst = nullptr;

	vk::raii::Buffer velocityBufferInst = nullptr;
	vk::raii::DeviceMemory velocityMemoryInst = nullptr;
	void* velocityMappedInst = nullptr;
};
