#ifndef SKYBOX_PASS_H
#define SKYBOX_PASS_H

#include <array>

#include "../data_buffer/uniform_buffer.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/skybox_pipeline.h"
#include "base_pass.h" // DrawData
#include "../config.h"

class SkyboxPass
{
public:

	SkyboxPass();
	~SkyboxPass();
	SkyboxPass(SkyboxPass&& other) noexcept;

	SkyboxPass(const SkyboxPass& other) = delete;
	SkyboxPass& operator=(const SkyboxPass& other) = delete;

	void UpdateSkyboxCubemap(const TANG::TextureResource* skyboxCubemap);
	void UpdateViewProjUniformBuffers(uint32_t frameIndex, const glm::mat4& view, const glm::mat4 proj);

	void UpdateDescriptorSets(uint32_t frameIndex);

	void Create(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
	void Destroy();

	void Draw(uint32_t currentFrame, const TANG::DrawData& data);

private:

	void CreatePipelines(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight);
	void CreateSetLayoutCaches();
	void CreateDescriptorSets();
	void CreateUniformBuffers();

	TANG::SkyboxPipeline skyboxPipeline;
	TANG::SetLayoutCache skyboxSetLayoutCache;
	std::array<TANG::UniformBuffer, TANG::CONFIG::MaxFramesInFlight> viewUBO;
	std::array<TANG::UniformBuffer, TANG::CONFIG::MaxFramesInFlight> projUBO;
	std::array<std::array<TANG::DescriptorSet, 2>, TANG::CONFIG::MaxFramesInFlight> skyboxDescriptorSets;

	bool wasCreated;
};

#endif