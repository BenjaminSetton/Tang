#ifndef BLOOM_PASS_H
#define BLOOM_PASS_H

#include <array>

#include "TANG/config.h"
#include "TANG/descriptor_set.h"
#include "TANG/set_layout_cache.h"
#include "TANG/texture_resource.h"

#include "pipelines/bloom_composition_pipeline.h"
#include "pipelines/bloom_downscaling_pipeline.h"
#include "pipelines/bloom_upscaling_pipeline.h"

namespace TANG
{
	class DescriptorPool;
	class TextureResource;
}

class BloomPass
{
public:

	BloomPass();
	~BloomPass();

	BloomPass(BloomPass&& other) = delete;
	BloomPass(const BloomPass& other) = delete;
	BloomPass& operator=(const BloomPass& other) = delete;

	void Create(uint32_t baseTextureWidth, uint32_t baseTextureHeight);
	void Destroy();

	// Input texture cannot be const because we might have to transition it's layout to
	// SRC_OPTIMAL to copy mip level 0 to the downscale texture resource
	void Draw(uint32_t currentFrame, TANG::CommandBuffer* cmdBuffer, TANG::TextureResource* inputTexture);

	const TANG::TextureResource* GetOutputTexture() const;

private:

	void DownscaleTexture(TANG::CommandBuffer* cmdBuffer, uint32_t currentFrame);
	void UpscaleTexture(TANG::CommandBuffer* cmdBuffer, uint32_t currentFrame);
	void PerformComposition(TANG::CommandBuffer* cmdBuffer, uint32_t currentFrame, TANG::TextureResource* inputTexture);

	void CreatePipelines();
	void CreateSetLayoutCaches();
	void CreateDescriptorSets();
	void CreateTextures(uint32_t width, uint32_t height);

	TANG::BloomDownscalingPipeline bloomDownscalingPipeline;
	TANG::TextureResource bloomDownscalingTexture;
	TANG::SetLayoutCache bloomDownscalingSetLayoutCache;
	std::array<std::array<TANG::DescriptorSet, TANG::CONFIG::BloomMaxMips>, TANG::CONFIG::MaxFramesInFlight> bloomDownscalingDescriptorSets;

	TANG::BloomUpscalingPipeline bloomUpscalingPipeline;
	TANG::TextureResource bloomUpscalingTexture;
	TANG::SetLayoutCache bloomUpscalingSetLayoutCache;
	std::array<std::array<TANG::DescriptorSet, TANG::CONFIG::BloomMaxMips>, TANG::CONFIG::MaxFramesInFlight> bloomUpscalingDescriptorSets;

	TANG::BloomCompositionPipeline bloomCompositionPipeline;
	TANG::TextureResource bloomCompositionTexture;
	TANG::SetLayoutCache bloomCompositionSetLayoutCache;
	std::array<TANG::DescriptorSet, TANG::CONFIG::MaxFramesInFlight> bloomCompositionDescriptorSets;

	bool wasCreated;
};


#endif