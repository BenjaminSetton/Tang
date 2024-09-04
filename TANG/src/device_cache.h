#ifndef DEVICE_CACHE_H
#define DEVICE_CACHE_H

#include "vulkan/vulkan.h"

namespace TANG
{
	// A device cache containing functionality for caching, invalidating and retrieving
	// the logical and physical Vulkan devices. Note that VkDevice and VkPhysicalDevice
	// are Vulkan handles, which makes them pointer types. This is why they're passed 
	// around by value everything as opposed to const references.
	class DeviceCache
	{
	public:

		friend class Renderer;

		DeviceCache(const DeviceCache& other) = delete;
		DeviceCache& operator=(const DeviceCache& other) = delete;

		static DeviceCache& Get()
		{
			static DeviceCache instance;
			return instance;
		}

		// Getters / utility
		VkDevice GetLogicalDevice() const;
		VkPhysicalDevice GetPhysicalDevice() const;
		VkSampleCountFlagBits GetMaxMSAA() const;
		VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const;
		VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures() const;
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const;

	private:

		// Singleton
		DeviceCache();

	private:

		// NOTE - Only the main renderer class is allowed to update or invalidate the cache
		void CacheDevices(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);
		void CacheLogicalDevice(VkDevice logicalDevice);
		void CachePhysicalDevice(VkPhysicalDevice physicalDevice);

		void InvalidateCache();

		// Utilizes cached physical device properties to calculate max MSAA samples
		VkSampleCountFlagBits CalculateMaxMSAA();

		VkDevice logicalDevice;
		VkPhysicalDevice physicalDevice;

		VkSampleCountFlagBits msaaSamples;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	};

	// Helper function for getting physical/logical devices, since they're needed in a ton of places
	inline VkDevice GetLogicalDevice()
	{
		return DeviceCache::Get().GetLogicalDevice();
	}

	inline VkPhysicalDevice GetPhysicalDevice()
	{
		return DeviceCache::Get().GetPhysicalDevice();
	}
}

#endif