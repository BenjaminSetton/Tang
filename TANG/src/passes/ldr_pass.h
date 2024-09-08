#ifndef LDR_PASS_H
#define LDR_PASS_H

#include <array>

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/ldr_pipeline.h"
#include "base_pass.h"
#include "../config.h"

namespace TANG
{
	class LDRPass : public BasePass
	{
	public:

		LDRPass();
		~LDRPass();
		LDRPass(LDRPass&& other) noexcept;

		LDRPass(const LDRPass& other) = delete;
		LDRPass& operator=(const LDRPass& other) = delete;

		void SetData(const DescriptorPool* descriptorPool, const LDRRenderPass* hdrRenderPass, VkExtent2D swapChainExtent);

		void UpdateExposureUniformBuffer(uint32_t frameIndex, float exposure);
		void UpdateDescriptorSets(uint32_t frameIndex, const TextureResource* hdrTexture);

		void Create() override;
		void Destroy() override;

		void Draw(uint32_t frameIndex, const DrawData& data);

	private:

		void CreatePipelines() override;
		void CreateSetLayoutCaches() override;
		void CreateDescriptorSets() override;
		void CreateUniformBuffers() override;

		void ResetBorrowedData() override;

		LDRPipeline ldrPipeline;
		SetLayoutCache ldrSetLayoutCache;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> ldrExposureUBO;
		std::array<DescriptorSet, CONFIG::MaxFramesInFlight> ldrDescriptorSet;

		struct
		{
			const DescriptorPool* descriptorPool;
			const LDRRenderPass* hdrRenderPass;
			VkExtent2D swapChainExtent;
		} borrowedData;
	};
}

#endif