#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"
#include "utils/GpuStructs.h"

class VulkanUniformBuffer : public QRenderObject
{
public:
	VulkanUniformBuffer(class VulkanDevice* vulkanDeviceRef, class VulkanPhysicalDevice* vulkanPhysicalDeviceRef);
	virtual ~VulkanUniformBuffer() override;

	void create() override;
	void updateSceneData(const SceneData& sceneData);

	const vk::raii::Buffer* getBufferInst() const { return &bufferInst; }
	vk::DeviceSize getBufferSize() const { return sizeof(SceneData); }

private:
	uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

	const class VulkanDevice* vulkanDeviceRef = nullptr;
	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;

	vk::raii::Buffer bufferInst = nullptr;
	vk::raii::DeviceMemory bufferMemoryInst = nullptr;
	void* bufferMappedInst = nullptr;
};
