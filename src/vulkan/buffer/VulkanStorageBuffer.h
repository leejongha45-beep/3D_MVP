#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

class VulkanStorageBuffer : public QRenderObject
{
public:
	VulkanStorageBuffer(class VulkanDevice* vulkanDeviceRef, class VulkanPhysicalDevice* vulkanPhysicalDeviceRef);
	virtual ~VulkanStorageBuffer() override;

	void create() override;

	const vk::raii::Buffer* getFieldBufferInst() const { return &fieldBufferInst; }
	const vk::raii::Buffer* getVelocityBufferInst() const { return &velocityBufferInst; }
	vk::DeviceSize getBufferSize() const { return GRID_TOTAL * sizeof(float); }
	uint32_t getGridSize() const { return GRID_SIZE; }

private:
	uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
	void createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory);

	const class VulkanDevice* vulkanDeviceRef = nullptr;
	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;

	static constexpr uint32_t GRID_SIZE = 128;
	static constexpr uint32_t GRID_TOTAL = GRID_SIZE * GRID_SIZE * GRID_SIZE;

	vk::raii::Buffer fieldBufferInst = nullptr;
	vk::raii::DeviceMemory fieldMemoryInst = nullptr;
	vk::raii::Buffer velocityBufferInst = nullptr;
	vk::raii::DeviceMemory velocityMemoryInst = nullptr;
};
