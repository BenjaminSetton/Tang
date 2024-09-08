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
	struct Transform;

	class PBRPass : public BasePass
	{
	public:

		PBRPass();
		~PBRPass();
		PBRPass(PBRPass&& other) noexcept;

		PBRPass(const PBRPass& other) = delete;
		PBRPass& operator=(const PBRPass& other) = delete;

		void SetData(const DescriptorPool* descriptorPool, const HDRRenderPass* hdrRenderPass, VkExtent2D swapChainExtent);

		void UpdateTransformUniformBuffer(uint32_t frameIndex, Transform& transform);
		void UpdateViewUniformBuffer(uint32_t frameIndex, const glm::mat4& viewMatrix);
		void UpdateProjUniformBuffer(uint32_t frameIndex, const glm::mat4& projMatrix);
		void UpdateCameraUniformBuffer(uint32_t frameIndex, const glm::vec3& position);

		void UpdateDescriptorSets(uint32_t frameIndex, std::array<const TextureResource*, 8>& textures);

		void Create() override;
		void Destroy() override;

		void Draw(uint32_t frameIndex, const DrawData& data);

	private:

		void CreatePipelines() override;
		void CreateSetLayoutCaches() override;
		void CreateDescriptorSets() override;
		void CreateUniformBuffers() override;

		void ResetBorrowedData() override;

		PBRPipeline pbrPipeline;
		SetLayoutCache pbrSetLayoutCache;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> transformUBO;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> viewUBO;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> projUBO;
		std::array<UniformBuffer, CONFIG::MaxFramesInFlight> cameraDataUBO;
		std::array<std::array<DescriptorSet, 3>, CONFIG::MaxFramesInFlight> pbrDescriptorSets;
		SecondaryCommandBuffer cmdBuffer;

		struct
		{
			const DescriptorPool* descriptorPool;
			const HDRRenderPass* hdrRenderPass;
			VkExtent2D swapChainExtent;
		} borrowedData;
	};
}

#endif