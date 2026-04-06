#include "VulkanUniformBuffer.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"

#include <cstring>

VulkanUniformBuffer::VulkanUniformBuffer(VulkanDevice* vulkanDeviceRef, VulkanPhysicalDevice* vulkanPhysicalDeviceRef)
	: vulkanDeviceRef(vulkanDeviceRef), vulkanPhysicalDeviceRef(vulkanPhysicalDeviceRef)
{
	assert(vulkanDeviceRef);
	assert(vulkanPhysicalDeviceRef);
}

VulkanUniformBuffer::~VulkanUniformBuffer()
{
}

void VulkanUniformBuffer::create()
{
	if (!ENSURE(vulkanDeviceRef))
		return;
	if (!ENSURE(vulkanPhysicalDeviceRef))
		return;

	const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
	if (!ENSURE(pDevice))
		return;

	const vk::raii::PhysicalDevice* pPhysicalDevice = vulkanPhysicalDeviceRef->getPhysicalDeviceInst();
	if (!ENSURE(pPhysicalDevice))
		return;

	vk::BufferCreateInfo bufferCreateInfo{
		.size        = sizeof(SceneData),
		.usage       = vk::BufferUsageFlagBits::eUniformBuffer,
		.sharingMode = vk::SharingMode::eExclusive
	};

	bufferInst = vk::raii::Buffer(*pDevice, bufferCreateInfo);

	vk::MemoryRequirements memoryRequirements = bufferInst.getMemoryRequirements();
	uint32_t memoryTypeIndex = findMemoryType(
		*pPhysicalDevice,
		memoryRequirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	if (!ENSURE(memoryTypeIndex != UINT32_MAX))
		return;

	vk::MemoryAllocateInfo memoryAllocateInfo{
		.allocationSize  = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	bufferMemoryInst = vk::raii::DeviceMemory(*pDevice, memoryAllocateInfo);
	bufferInst.bindMemory(*bufferMemoryInst, 0);

	bufferMappedInst = bufferMemoryInst.mapMemory(0, sizeof(SceneData));
	if (!ENSURE(bufferMappedInst))
	{
		std::cerr << "[VulkanUniformBuffer] Failed to map uniform buffer memory" << std::endl;
	}
}

void VulkanUniformBuffer::updateSceneData(const SceneData& sceneData)
{
	if (!ENSURE(bufferMappedInst))
		return;

	memcpy(bufferMappedInst, &sceneData, sizeof(SceneData));
}

uint32_t VulkanUniformBuffer::findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
	vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	std::cerr << "[VulkanUniformBuffer] Failed to find suitable memory type" << std::endl;
	return UINT32_MAX;
}
