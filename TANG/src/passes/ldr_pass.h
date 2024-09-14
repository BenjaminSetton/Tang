#ifndef LDR_PASS_H
#define LDR_PASS_H

#include <array>

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/ldr_pipeline.h"
#include "base_pass.h" // DrawData
#include "../config.h"

namespace TANG
{
	class LDRPass
	{
	public:

		LDRPass();
		~LDRPass();
		LDRPass(LDRPass&& other) noexcept;

		LDRPass(const LDRPass& other) = delete;
		LDRPass& operator=(const LDRPass& other) = delete;

		void UpdateExposureUniformBuffer(uint32_t frameIndex, float exposure);
		void UpdateDescriptorSets(uint32_t frameIndex, const TextureResource* hdrTexture);

		void Create(const LDRRenderPass* ldrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
		void Destroy();

		void Draw(uint32_t frameIndex, const DrawData& data);

	private:

		void CreatePipelines(const LDRRenderPass* ldrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
		void CreateSetLayoutCaches();
		void CreateDescriptorSets();
		void CreateUniformBuffers();

		LDRPipeline ldrPipeline;
		SetLayoutCache ldrSetLayoutCache;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> ldrExposureUBO;
		std::array<DescriptorSet, CONFIG::MaxFramesInFlight> ldrDescriptorSet;

		bool wasCreated;

	};
}

#endif