#ifndef PBR_PASS_H
#define PBR_PASS_H

#include <array>

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/pbr_pipeline.h"
#include "base_pass.h"
#include "../config.h"

namespace TANG
{
	class PBRPass : public BasePass
	{
	public:

		PBRPass();
		~PBRPass();
		PBRPass(PBRPass&& other) noexcept;

		PBRPass(const PBRPass& other) = delete;
		PBRPass& operator=(const PBRPass& other) = delete;

		void SetData(const DescriptorPool* descriptorPool, const HDRRenderPass* hdrRenderPass, VkExtent2D swapChainExtent);

		void UpdatePBRTextureDescriptors(const TextureResource* skyboxCubemap[8]);
		//void UpdateCameraMatricesShaderParameters(uint32_t frameIndex, const UniformBuffer* view, const UniformBuffer* proj);

		void Create() override;
		void Destroy() override;

		void Draw(uint32_t currentFrame, const DrawData& data);

	private:

		void CreatePipelines() override;
		void CreateSetLayoutCaches() override;
		void CreateDescriptorSets() override;

		void ResetBorrowedData() override;

		PBRPipeline pbrPipeline;
		SetLayoutCache pbrSetLayoutCache;
		std::array<std::array<DescriptorSet, 3>, CONFIG::MaxFramesInFlight> pbrDescriptorSets;

		struct
		{
			const DescriptorPool* descriptorPool;
			const HDRRenderPass* hdrRenderPass;
			VkExtent2D swapChainExtent;
		} borrowedData;
	};
}

#endif