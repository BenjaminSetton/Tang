#ifndef SKYBOX_PASS_H
#define SKYBOX_PASS_H

#include <array>

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/skybox_pipeline.h"
#include "base_pass.h" // DrawData
#include "../config.h"

namespace TANG
{
	class SkyboxPass
	{
	public:

		SkyboxPass();
		~SkyboxPass();
		SkyboxPass(SkyboxPass&& other) noexcept;

		SkyboxPass(const SkyboxPass& other) = delete;
		SkyboxPass& operator=(const SkyboxPass& other) = delete;

		void UpdateSkyboxCubemap(const TextureResource* skyboxCubemap);
		void UpdateViewProjUniformBuffers(uint32_t frameIndex, const glm::mat4& view, const glm::mat4 proj);

		void UpdateDescriptorSets(uint32_t frameIndex);

		void Create(const DescriptorPool* descriptorPool, const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
		void Destroy();

		void Draw(uint32_t currentFrame, const DrawData& data);

	private:

		void CreatePipelines(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
		void CreateSetLayoutCaches();
		void CreateDescriptorSets(const DescriptorPool* descriptorPool);
		void CreateUniformBuffers();

		SkyboxPipeline skyboxPipeline;
		SetLayoutCache skyboxSetLayoutCache;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> viewUBO;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> projUBO;
		std::array<std::array<DescriptorSet, 2>, CONFIG::MaxFramesInFlight> skyboxDescriptorSets;

		bool wasCreated;
	};
}

#endif