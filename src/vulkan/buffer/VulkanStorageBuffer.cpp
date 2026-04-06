#include "VulkanStorageBuffer.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"

#include <cstring>

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

	// field: vec4 (xyz=RGB파동, w=반사계수) × 128^3
	createBuffer(*pDevice, *pPhysicalDevice, GRID_TOTAL * sizeof(float) * 4, fieldBufferInst, fieldMemoryInst, fieldMappedInst);
	// velocity: vec3 (RGB 스펙트럼별 속도) × 128^3
	createBuffer(*pDevice, *pPhysicalDevice, GRID_TOTAL * sizeof(float) * 3, velocityBufferInst, velocityMemoryInst, velocityMappedInst);

	clearGrid();
}

void VulkanStorageBuffer::createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory, void*& mapped)
{
	vk::BufferCreateInfo bufferCreateInfo{
		.size        = size,
		.usage       = vk::BufferUsageFlagBits::eStorageBuffer,
		.sharingMode = vk::SharingMode::eExclusive
	};

	buffer = vk::raii::Buffer(device, bufferCreateInfo);

	vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
	uint32_t memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	if (!ENSURE(memoryTypeIndex != UINT32_MAX))
		return;

	vk::MemoryAllocateInfo memoryAllocateInfo{
		.allocationSize  = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	memory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
	buffer.bindMemory(*memory, 0);

	mapped = memory.mapMemory(0, size);
	if (!ENSURE(mapped))
	{
		std::cerr << "[VulkanStorageBuffer] Failed to map buffer memory" << std::endl;
	}
}

void VulkanStorageBuffer::clearGrid()
{
	if (!ENSURE(fieldMappedInst))
		return;
	if (!ENSURE(velocityMappedInst))
		return;

	memset(fieldMappedInst, 0, GRID_TOTAL * sizeof(float) * 4);
	memset(velocityMappedInst, 0, GRID_TOTAL * sizeof(float) * 3);
}

void VulkanStorageBuffer::markStaticObject(const glm::vec3& worldPosition, const glm::vec3& reflectSpectrum)
{
	if (!ENSURE(fieldMappedInst))
		return;

	uint32_t index = worldToGridIndex(worldPosition);
	if (index == UINT32_MAX)
		return;

	// field[index] = vec4(reflectSpectrum.rgb, 반사계수 평균)
	float* fieldData = static_cast<float*>(fieldMappedInst);
	uint32_t offset = index * 4;
	fieldData[offset + 0] = 0.0f;
	fieldData[offset + 1] = 0.0f;
	fieldData[offset + 2] = 0.0f;
	fieldData[offset + 3] = (reflectSpectrum.r + reflectSpectrum.g + reflectSpectrum.b) / 3.0f;
}

uint32_t VulkanStorageBuffer::worldToGridIndex(const glm::vec3& worldPosition) const
{
	int gridX = static_cast<int>(worldPosition.x + GRID_SIZE / 2);
	int gridY = static_cast<int>(worldPosition.y);
	int gridZ = static_cast<int>(worldPosition.z + GRID_SIZE / 2);

	if (gridX < 0 || gridX >= static_cast<int>(GRID_SIZE) ||
		gridY < 0 || gridY >= static_cast<int>(GRID_SIZE) ||
		gridZ < 0 || gridZ >= static_cast<int>(GRID_SIZE))
	{
		return UINT32_MAX;
	}

	return gridZ * GRID_SIZE * GRID_SIZE + gridY * GRID_SIZE + gridX;
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
