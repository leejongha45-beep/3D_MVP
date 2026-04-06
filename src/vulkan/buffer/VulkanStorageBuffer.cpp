#include "VulkanStorageBuffer.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"

VulkanStorageBuffer::VulkanStorageBuffer(VulkanDevice* vulkanDeviceRef, VulkanPhysicalDevice* vulkanPhysicalDeviceRef)
	: vulkanDeviceRef(vulkanDeviceRef), vulkanPhysicalDeviceRef(vulkanPhysicalDeviceRef)
{
	assert(vulkanDeviceRef);
	assert(vulkanPhysicalDeviceRef);
}

VulkanStorageBuffer::~VulkanStorageBuffer()
{
}

void VulkanStorageBuffer::create()
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

	createBuffer(*pDevice, *pPhysicalDevice, fieldBufferInst, fieldMemoryInst);
	createBuffer(*pDevice, *pPhysicalDevice, velocityBufferInst, velocityMemoryInst);
}

void VulkanStorageBuffer::createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory)
{
	vk::BufferCreateInfo bufferCreateInfo{
		.size        = GRID_TOTAL * sizeof(float),
		.usage       = vk::BufferUsageFlagBits::eStorageBuffer,
		.sharingMode = vk::SharingMode::eExclusive
	};

	buffer = vk::raii::Buffer(device, bufferCreateInfo);

	vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
	uint32_t memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	if (!ENSURE(memoryTypeIndex != UINT32_MAX))
		return;

	vk::MemoryAllocateInfo memoryAllocateInfo{
		.allocationSize  = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	memory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
	buffer.bindMemory(*memory, 0);
}

uint32_t VulkanStorageBuffer::findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
	vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	std::cerr << "[VulkanStorageBuffer] Failed to find suitable memory type" << std::endl;
	return UINT32_MAX;
}
