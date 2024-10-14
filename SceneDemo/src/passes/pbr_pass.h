#ifndef PBR_PASS_H
#define PBR_PASS_H

#include <array>

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/pbr_pipeline.h"
#include "base_pass.h" // DrawData
#include "../config.h"

namespace TANG
{
	struct Transform;
}

class PBRPass
{
public:

	PBRPass();
	~PBRPass();
	PBRPass(PBRPass&& other) noexcept;

	PBRPass(const PBRPass& other) = delete;
	PBRPass& operator=(const PBRPass& other) = delete;

	void UpdateTransformUniformBuffer(uint32_t frameIndex, TANG::Transform& transform);
	void UpdateViewUniformBuffer(uint32_t frameIndex, const glm::mat4& viewMatrix);
	void UpdateProjUniformBuffer(uint32_t frameIndex, const glm::mat4& projMatrix);
	void UpdateCameraUniformBuffer(uint32_t frameIndex, const glm::vec3& position);

	void UpdateDescriptorSets(uint32_t frameIndex, std::array<const TANG::TextureResource*, 8>& textures);

	void Create(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
	void Destroy();

	void Draw(uint32_t frameIndex, const TANG::DrawData& data);

private:

	void CreatePipelines(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
	void CreateSetLayoutCaches();
	void CreateDescriptorSets();
	void CreateUniformBuffers();

	TANG::PBRPipeline pbrPipeline;
	TANG::SetLayoutCache pbrSetLayoutCache;
	std::array<TANG::UniformBuffer, TANG::CONFIG::MaxFramesInFlight> transformUBO;
	std::array<TANG::UniformBuffer, TANG::CONFIG::MaxFramesInFlight> viewUBO;
	std::array<TANG::UniformBuffer, TANG::CONFIG::MaxFramesInFlight> projUBO;
	std::array<TANG::UniformBuffer, TANG::CONFIG::MaxFramesInFlight> cameraDataUBO;
	std::array<std::array<TANG::DescriptorSet, 3>, TANG::CONFIG::MaxFramesInFlight> pbrDescriptorSets;
	TANG::SecondaryCommandBuffer cmdBuffer;

	bool wasCreated;
};


#endif