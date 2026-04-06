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
	// 3평면(xz, xy, yz) × 1024x1024
	createBuffer(*pDevice, *pPhysicalDevice, static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT_INTERNAL * sizeof(float) * 4, fieldBufferInst, fieldMemoryInst, fieldMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, static_cast<vk::DeviceSize>(GRID_TOTAL) * PLANE_COUNT_INTERNAL * sizeof(float) * 4, velocityBufferInst, velocityMemoryInst, velocityMappedInst);

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

	memset(fieldMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT_INTERNAL * sizeof(float) * 4);
	memset(velocityMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT_INTERNAL * sizeof(float) * 4);
}

void VulkanStorageBuffer::markStaticObject(const glm::vec3& worldPosition, const glm::vec3& reflectSpectrum, float roughness)
{
	if (!ENSURE(fieldMappedInst))
		return;
	if (!ENSURE(velocityMappedInst))
		return;

	float reflectCoeff = (reflectSpectrum.r + reflectSpectrum.g + reflectSpectrum.b) / 3.0f;
	float* fieldData = static_cast<float*>(fieldMappedInst);
	float* velocityData = static_cast<float*>(velocityMappedInst);

	int half = static_cast<int>(GRID_SIZE / 2);

	// 평면 0: xy (위에서 아래, z축이 높이)
	int xy_x = static_cast<int>(worldPosition.x) + half;
	int xy_y = static_cast<int>(worldPosition.y) + half;
	if (xy_x >= 0 && xy_x < static_cast<int>(GRID_SIZE) && xy_y >= 0 && xy_y < static_cast<int>(GRID_SIZE))
	{
		uint32_t idx = (0 * GRID_TOTAL + xy_y * GRID_SIZE + xy_x) * 4;
		fieldData[idx + 3] = reflectCoeff;
		velocityData[idx + 3] = roughness;
	}

	// 평면 1: xz (앞에서 뒤)
	int xz_x = static_cast<int>(worldPosition.x) + half;
	int xz_y = static_cast<int>(worldPosition.z) + half;
	if (xz_x >= 0 && xz_x < static_cast<int>(GRID_SIZE) && xz_y >= 0 && xz_y < static_cast<int>(GRID_SIZE))
	{
		uint32_t idx = (1 * GRID_TOTAL + xz_y * GRID_SIZE + xz_x) * 4;
		fieldData[idx + 3] = reflectCoeff;
		velocityData[idx + 3] = roughness;
	}

	// 평면 2: yz (옆에서)
	int yz_x = static_cast<int>(worldPosition.z) + half;
	int yz_y = static_cast<int>(worldPosition.y) + half;
	if (yz_x >= 0 && yz_x < static_cast<int>(GRID_SIZE) && yz_y >= 0 && yz_y < static_cast<int>(GRID_SIZE))
	{
		uint32_t idx = (2 * GRID_TOTAL + yz_y * GRID_SIZE + yz_x) * 4;
		fieldData[idx + 3] = reflectCoeff;
		velocityData[idx + 3] = roughness;
	}
}

uint32_t VulkanStorageBuffer::worldToGridIndex(const glm::vec3& worldPosition) const
{
	// 태양 시점 (위에서 아래) xz 평면으로 투영
	int gridX = static_cast<int>(worldPosition.x + GRID_SIZE / 2);
	int gridZ = static_cast<int>(worldPosition.z + GRID_SIZE / 2);

	if (gridX < 0 || gridX >= static_cast<int>(GRID_SIZE) ||
		gridZ < 0 || gridZ >= static_cast<int>(GRID_SIZE))
	{
		return UINT32_MAX;
	}

	return gridZ * GRID_SIZE + gridX;
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
