
#include "device_cache.h"

namespace TANG
{
	DeviceCache::DeviceCache()
	{
		InvalidateCache();
	}

	VkDevice DeviceCache::GetLogicalDevice() const
	{
		return logicalDevice;
	}

	VkPhysicalDevice DeviceCache::GetPhysicalDevice() const
	{
		return physicalDevice;
	}

	VkSampleCountFlagBits DeviceCache::GetMaxMSAA() const
	{
		return msaaSamples;
	}

	VkPhysicalDeviceProperties DeviceCache::GetPhysicalDeviceProperties() const
	{
		return physicalDeviceProperties;
	}

	VkPhysicalDeviceFeatures DeviceCache::GetPhysicalDeviceFeatures() const
	{
		return physicalDeviceFeatures;
	}

	VkPhysicalDeviceMemoryProperties DeviceCache::GetPhysicalDeviceMemoryProperties() const
	{
		return physicalDeviceMemoryProperties;
	}

	void DeviceCache::CacheDevices(VkDevice _logicalDevice, VkPhysicalDevice _physicalDevice)
	{
		logicalDevice = _logicalDevice;
		physicalDevice = _physicalDevice;
	}

	void DeviceCache::CacheLogicalDevice(VkDevice _logicalDevice)
	{
		logicalDevice = _logicalDevice;
	}

	void DeviceCache::CachePhysicalDevice(VkPhysicalDevice _physicalDevice)
	{
		physicalDevice = _physicalDevice;

		// Calculate and cache physical-device-dependent data
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
		msaaSamples = CalculateMaxMSAA();
	}

	void DeviceCache::InvalidateCache()
	{
		logicalDevice = VK_NULL_HANDLE;
		physicalDevice = VK_NULL_HANDLE;
		msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		physicalDeviceProperties = VkPhysicalDeviceProperties();
		physicalDeviceFeatures = VkPhysicalDeviceFeatures();
		physicalDeviceMemoryProperties = VkPhysicalDeviceMemoryProperties();
	}

	VkSampleCountFlagBits DeviceCache::CalculateMaxMSAA()
	{
		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}
}