#ifndef BLOOM_PASS_H
#define BLOOM_PASS_H

#include <array>

#include "../config.h"
#include "../descriptors/descriptor_set.h"
#include "../pipelines/bloom_prefilter_pipeline.h"
#include "../pipelines/bloom_downscaling_pipeline.h"
#include "../pipelines/bloom_upscaling_pipeline.h"
#include "base_pass.h"

namespace TANG
{
	class DescriptorPool;
	class TextureResource;

	// I don't like the derive-from-BasePass thing. I feel like this inheritance does not make any sense or provide
	// any useful functionality, since all passes need different data and function signatures to work correctly.
	// IMO the best functionality of creating virtual methods is virtual dispatch. 
	// We were manually calling all the functions in the renderer's code anyway, so we weren't taking advantage of this.
	// If that changes in the future, it might be worth figuring out a good way to setup virtual methods for passes
	class BloomPass
	{
	public:

		BloomPass();
		~BloomPass();
		BloomPass(BloomPass&& other) noexcept;

		BloomPass(const BloomPass& other) = delete;
		BloomPass& operator=(const BloomPass& other) = delete;

		void Create(const DescriptorPool* descriptorPool, uint32_t baseTextureWidth, uint32_t baseTextureHeight);
		void Destroy();

		// Input texture cannot be const because we might have to transition it's layout to
		// SRC_OPTIMAL to copy mip level 0 to the downscale texture resource
		void Draw(uint32_t currentFrame, CommandBuffer* cmdBuffer, TextureResource* inputTexture);

	private:

		void PrefilterInputTexture(CommandBuffer* cmdBuffer, uint32_t currentFrame, TextureResource* inputTexture);
		void DownscaleTexture(CommandBuffer* cmdBuffer, uint32_t currentFrame, uint32_t startingWidth, uint32_t startingHeight);
		void UpscaleTexture(CommandBuffer* cmdBuffer, uint32_t currentFrame, uint32_t startingWidth, uint32_t startingHeight);

		void CreatePipelines();
		void CreateSetLayoutCaches();
		void CreateDescriptorSets(const DescriptorPool* descriptorPool);
		void CreateTextures(uint32_t width, uint32_t height);

		SetLayoutCache bloomFilteringSetLayoutCache;

		BloomPrefilterPipeline bloomPrefilterPipeline;
		SetLayoutCache bloomPrefilterSetLayoutCache;
		std::array<DescriptorSet, CONFIG::MaxFramesInFlight> bloomPrefilterDescriptorSets;

		BloomDownscalingPipeline bloomDownscalingPipeline;
		TextureResource bloomDownscalingTexture;
		std::array<std::array<DescriptorSet, CONFIG::BloomMaxMips - 1>, CONFIG::MaxFramesInFlight> bloomDownscalingDescriptorSets;

		BloomUpscalingPipeline bloomUpscalingPipeline;
		TextureResource bloomUpscalingTexture;
		std::array<std::array<DescriptorSet, CONFIG::BloomMaxMips>, CONFIG::MaxFramesInFlight> bloomUpscalingDescriptorSets;

		bool wasCreated;
	};
}

#endif