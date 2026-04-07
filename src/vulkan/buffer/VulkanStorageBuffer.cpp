#include "VulkanStorageBuffer.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"

#include <cstring>
#include <cmath>

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

	createBuffer(*pDevice, *pPhysicalDevice, getFieldXYZBufferSize(), fieldXYZBufferInst, fieldXYZMemoryInst, fieldXYZMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, getFieldWBufferSize(), fieldWBufferInst, fieldWMemoryInst, fieldWMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, getVelocityXYZBufferSize(), velocityXYZBufferInst, velocityXYZMemoryInst, velocityXYZMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, getVelocityWBufferSize(), velocityWBufferInst, velocityWMemoryInst, velocityWMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, getAccumCounterBufferSize(), accumCounterBufferInst, accumCounterMemoryInst, accumCounterMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, getNormalBufferSize(), normalBufferInst, normalMemoryInst, normalMappedInst);
	createBuffer(*pDevice, *pPhysicalDevice, getReflectedFieldXYZBufferSize(), reflectedFieldXYZBufferInst, reflectedFieldXYZMemoryInst, reflectedFieldXYZMappedInst);

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
	if (ENSURE(fieldXYZMappedInst))
		memset(fieldXYZMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4);
	if (ENSURE(fieldWMappedInst))
		memset(fieldWMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float));
	if (ENSURE(velocityXYZMappedInst))
		memset(velocityXYZMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4);
	if (ENSURE(velocityWMappedInst))
		memset(velocityWMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float));
	if (ENSURE(accumCounterMappedInst))
		memset(accumCounterMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float));
	if (ENSURE(normalMappedInst))
		memset(normalMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4);
	if (ENSURE(reflectedFieldXYZMappedInst))
		memset(reflectedFieldXYZMappedInst, 0, static_cast<size_t>(GRID_TOTAL) * PLANE_COUNT * sizeof(float) * 4);
}

void VulkanStorageBuffer::markStaticObject(const glm::vec3& worldPosition, const glm::vec3& reflectSpectrum, float roughness, const glm::vec3& normal)
{
	if (!ENSURE(fieldWMappedInst))
		return;
	if (!ENSURE(velocityWMappedInst))
		return;
	if (!ENSURE(normalMappedInst))
		return;

	float reflectCoeff = (reflectSpectrum.r + reflectSpectrum.g + reflectSpectrum.b) / 3.0f;
	float* fieldWData = static_cast<float*>(fieldWMappedInst);
	float* velocityWData = static_cast<float*>(velocityWMappedInst);
	float* normalData = static_cast<float*>(normalMappedInst);

	int half = static_cast<int>(GRID_SIZE / 2);
	float absNx = std::abs(normal.x);
	float absNy = std::abs(normal.y);
	float absNz = std::abs(normal.z);

	// 평면 0: xy — 법선이 z 방향이면 이 평면에 평행 → 마킹 안 함
	if (absNz < 0.9f)
	{
		int xy_x = static_cast<int>(worldPosition.x / CELL_SIZE) + half;
		int xy_y = static_cast<int>(worldPosition.y / CELL_SIZE) + half;
		if (xy_x >= 0 && xy_x < static_cast<int>(GRID_SIZE) && xy_y >= 0 && xy_y < static_cast<int>(GRID_SIZE))
		{
			uint32_t idx = 0 * GRID_TOTAL + xy_y * GRID_SIZE + xy_x;
			fieldWData[idx] = reflectCoeff;
			velocityWData[idx] = roughness;
			normalData[idx * 4 + 0] = normal.x;
			normalData[idx * 4 + 1] = normal.y;
			normalData[idx * 4 + 2] = normal.z;
			normalData[idx * 4 + 3] = 0.0f;
		}
	}

	// 평면 1: xz — 법선이 y 방향이면 마킹 안 함 (수학 좌표계: coord.y 큰 = worldZ 큰)
	if (absNy < 0.9f)
	{
		int xz_x = static_cast<int>(worldPosition.x / CELL_SIZE) + half;
		int xz_y = static_cast<int>(worldPosition.z / CELL_SIZE) + half;
		if (xz_x >= 0 && xz_x < static_cast<int>(GRID_SIZE) && xz_y >= 0 && xz_y < static_cast<int>(GRID_SIZE))
		{
			uint32_t idx = 1 * GRID_TOTAL + xz_y * GRID_SIZE + xz_x;
			fieldWData[idx] = reflectCoeff;
			velocityWData[idx] = roughness;
			normalData[idx * 4 + 0] = normal.x;
			normalData[idx * 4 + 1] = normal.y;
			normalData[idx * 4 + 2] = normal.z;
			normalData[idx * 4 + 3] = 0.0f;
		}
	}

	// 평면 2: yz — 법선이 x 방향이면 마킹 안 함 (수학 좌표계)
	if (absNx < 0.9f)
	{
		int yz_x = static_cast<int>(worldPosition.y / CELL_SIZE) + half;
		int yz_y = static_cast<int>(worldPosition.z / CELL_SIZE) + half;
		if (yz_x >= 0 && yz_x < static_cast<int>(GRID_SIZE) && yz_y >= 0 && yz_y < static_cast<int>(GRID_SIZE))
		{
			uint32_t idx = 2 * GRID_TOTAL + yz_y * GRID_SIZE + yz_x;
			fieldWData[idx] = reflectCoeff;
			velocityWData[idx] = roughness;
			normalData[idx * 4 + 0] = normal.x;
			normalData[idx * 4 + 1] = normal.y;
			normalData[idx * 4 + 2] = normal.z;
			normalData[idx * 4 + 3] = 0.0f;
		}
	}
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
