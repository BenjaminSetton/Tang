
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/descriptor_pool.h"
#include "../descriptors/write_descriptor_set.h"
#include "../texture_resource.h"
#include "../utils/logger.h"
#include "bloom_pass.h"


namespace TANG
{
	BloomPass::BloomPass()
	{ }

	BloomPass::~BloomPass()
	{ }

	BloomPass::BloomPass(BloomPass&& other) noexcept
	{
		TNG_TODO();
	}

	void BloomPass::Create(const DescriptorPool* descriptorPool, uint32_t baseTextureWidth, uint32_t baseTextureHeight)
	{
		if (wasCreated)
		{
			LogWarning("Attempting to create bloom pass more than once!");
			return;
		}

		CreateSetLayoutCaches();
		CreateDescriptorSets(descriptorPool);
		CreatePipelines();
		CreateTextures(baseTextureWidth >> 1, baseTextureHeight >> 1); // We start the bloom pass at a quarter of the base resolution

		// Update the descriptor sets with the downscaling image view for every mip level. We're reusing the images so we can update the desc sets only once
		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			// Bloom downscaling
			for (uint32_t j = 0; j < CONFIG::BloomMaxMips - 1; j++)
			{
				WriteDescriptorSets bloomDownsamplingWriteDescSets(0, 2);
				bloomDownsamplingWriteDescSets.AddImage(bloomDownscalingDescriptorSets[i][j].GetDescriptorSet(), 0, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, j);	// Input image
				bloomDownsamplingWriteDescSets.AddImage(bloomDownscalingDescriptorSets[i][j].GetDescriptorSet(), 1, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, j + 1);	// Output image
				bloomDownscalingDescriptorSets[i][j].Update(bloomDownsamplingWriteDescSets);

			}

			// Bloom upsampling
			uint32_t maxMipIndex = CONFIG::BloomMaxMips - 1;
			for (uint32_t j = 0; j < CONFIG::BloomMaxMips - 1; j++)
			{
				WriteDescriptorSets bloomUpscalingWriteDescSets(0, 3);
				bloomUpscalingWriteDescSets.AddImage(bloomUpscalingDescriptorSets[i][j].GetDescriptorSet(), 0, &bloomUpscalingTexture  , VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxMipIndex - j    );	// Input previous upscale mip (blur upsample)
				bloomUpscalingWriteDescSets.AddImage(bloomUpscalingDescriptorSets[i][j].GetDescriptorSet(), 1, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxMipIndex - j - 1);	// Input current downscale mip (direct sample)
				bloomUpscalingWriteDescSets.AddImage(bloomUpscalingDescriptorSets[i][j].GetDescriptorSet(), 2, &bloomUpscalingTexture  , VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxMipIndex - j - 1);	// Output image
				bloomUpscalingDescriptorSets[i][j].Update(bloomUpscalingWriteDescSets);
			}
		}

		wasCreated = true;
	}

	void BloomPass::Destroy()
	{
		bloomUpscalingPipeline.Destroy();
		bloomDownscalingPipeline.Destroy();

		bloomUpscalingTexture.Destroy();
		bloomDownscalingTexture.Destroy();

		bloomUpscalingSetLayoutCache.DestroyLayouts();
		bloomDownscalingSetLayoutCache.DestroyLayouts();

		bloomPrefilterPipeline.Destroy();
		bloomPrefilterSetLayoutCache.DestroyLayouts();
	}

	void BloomPass::Draw(uint32_t currentFrame, CommandBuffer* cmdBuffer, TextureResource* inputTexture)
	{
		if (inputTexture == nullptr)
		{
			LogError("Failed to execute bloom pass, no input texture was bound!");
			return;
		}

		if (inputTexture->CalculateMipLevelsFromSize() < CONFIG::BloomMaxMips)
		{
			LogError("Size of input texture (%u, %u) is insufficient to perform a bloom pass on %u mips!",
				inputTexture->GetWidth(),
				inputTexture->GetHeight(),
				CONFIG::BloomMaxMips
			);
			return;
		}

		PrefilterInputTexture(cmdBuffer, currentFrame, inputTexture);

		bloomDownscalingTexture.InsertPipelineBarrier(cmdBuffer,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			1
		);

		// The starting width and height are actually mip level 1 because of how we set up the descriptor sets
		DownscaleTexture(cmdBuffer, currentFrame, bloomDownscalingTexture.GetWidth(), bloomDownscalingTexture.GetHeight());

		// Finish writing to the last mip before we use it as input for upscaling
		bloomDownscalingTexture.InsertPipelineBarrier(cmdBuffer,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			CONFIG::BloomMaxMips - 1,
			1
		);

		// Copy the last mip from the downscale texture to the upscale texture to begin upsampling process
		bloomUpscalingTexture.CopyFromTexture(cmdBuffer, &bloomDownscalingTexture, CONFIG::BloomMaxMips - 1, 1);

		// Finish copying the last mip level to the upscale texture before reading/writing from/to it in the upscaling pass
		// TODO - The garbage TransitionLayout function actually transitions all mips, so we must wait for the layout
		//        transition to happen in all mips regardless of how many we write to. This must be reworked ASAP
		bloomUpscalingTexture.InsertPipelineBarrier(cmdBuffer,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			CONFIG::BloomMaxMips
		);

		// The first mip we output to is actually the second to last mip, since we copied the last mip directly from the downsample texture
		UpscaleTexture(cmdBuffer, currentFrame, bloomUpscalingTexture.GetWidth() >> (CONFIG::BloomMaxMips - 2), bloomUpscalingTexture.GetHeight() >> (CONFIG::BloomMaxMips - 2));
	}

	void BloomPass::PrefilterInputTexture(CommandBuffer* cmdBuffer, uint32_t currentFrame, TextureResource* inputTexture)
	{
		uint32_t numDispatchesX = inputTexture->GetWidth() >> 1;
		uint32_t numDispatchesY = inputTexture->GetHeight() >> 1;

		// Bloom prefilter
		{
			WriteDescriptorSets bloomPrefilterWriteDescSets(0, 2);
			bloomPrefilterWriteDescSets.AddImage(bloomPrefilterDescriptorSets[currentFrame].GetDescriptorSet(), 0, inputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);		// Input image
			bloomPrefilterWriteDescSets.AddImage(bloomPrefilterDescriptorSets[currentFrame].GetDescriptorSet(), 1, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);	// Output image
			bloomPrefilterDescriptorSets[currentFrame].Update(bloomPrefilterWriteDescSets);
		}

		float brightnessThreshold = CONFIG::BloomBrightnessThreshold;
		cmdBuffer->CMD_BindPipeline(&bloomPrefilterPipeline);
		cmdBuffer->CMD_PushConstants(&bloomPrefilterPipeline, static_cast<void*>(&brightnessThreshold), sizeof(brightnessThreshold), VK_SHADER_STAGE_COMPUTE_BIT);
		cmdBuffer->CMD_BindDescriptorSets(&bloomPrefilterPipeline, 1, reinterpret_cast<VkDescriptorSet*>(&bloomPrefilterDescriptorSets[currentFrame]));
		cmdBuffer->CMD_Dispatch(static_cast<uint32_t>(ceil(numDispatchesX / 16.0)), static_cast<uint32_t>(ceil(numDispatchesY / 16.0)), 1);
	}

	void BloomPass::UpscaleTexture(CommandBuffer* cmdBuffer, uint32_t currentFrame, uint32_t startingWidth, uint32_t startingHeight)
	{
		cmdBuffer->CMD_BindPipeline(&bloomUpscalingPipeline);

		uint32_t currentWidth = startingWidth;
		uint32_t currentHeight = startingHeight;

		for (uint32_t i = 0; i < CONFIG::BloomMaxMips - 1; i++)
		{
			cmdBuffer->CMD_BindDescriptorSets(&bloomUpscalingPipeline, 1, reinterpret_cast<VkDescriptorSet*>(&bloomUpscalingDescriptorSets[currentFrame][i]));

			// Dispatch as many work groups as the first mip level of the input texture, divided by the number of invocations (local_size in compute shader)
			// We starting sampling from mip level 0 (N - 1) and write to mip level 1 (N), all the way down to N = CONFIG::BloomMaxMips
			cmdBuffer->CMD_Dispatch(static_cast<uint32_t>(ceil(currentWidth / 16.0)), static_cast<uint32_t>(ceil(currentHeight / 16.0)), 1);

			bloomUpscalingTexture.InsertPipelineBarrier(cmdBuffer,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				CONFIG::BloomMaxMips - i - 2,
				1
			);

			// Go up a mip level
			currentWidth <<= 1;
			currentHeight <<= 1;
		}
	}

	void BloomPass::DownscaleTexture(CommandBuffer* cmdBuffer, uint32_t currentFrame, uint32_t startingWidth, uint32_t startingHeight)
	{
		cmdBuffer->CMD_BindPipeline(&bloomDownscalingPipeline);

		uint32_t currentWidth = startingWidth;
		uint32_t currentHeight = startingHeight;
		for (uint32_t mipLevel = 1; mipLevel < CONFIG::BloomMaxMips; mipLevel++)
		{
			cmdBuffer->CMD_BindDescriptorSets(&bloomDownscalingPipeline, 1, reinterpret_cast<VkDescriptorSet*>(&bloomDownscalingDescriptorSets[currentFrame][mipLevel - 1]));

			// Finish writing to mip N - 1 before we can read from it
			bloomDownscalingTexture.InsertPipelineBarrier(cmdBuffer,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				mipLevel - 1, 
				1
			);

			// Dispatch as many work groups as the first mip level of the input texture, divided by the number of invocations (local_size in compute shader)
			// We starting sampling from mip level 0 (N - 1) and write to mip level 1 (N), all the way down to N = CONFIG::BloomMaxMips
			cmdBuffer->CMD_Dispatch(static_cast<uint32_t>(ceil(currentWidth / 16.0)), static_cast<uint32_t>(ceil(currentHeight / 16.0)), 1);

			// Go down a mip level
			currentWidth >>= 1;
			currentHeight >>= 1;
		}
	}

	void BloomPass::CreatePipelines()
	{
		// Bloom prefilter
		bloomPrefilterPipeline.SetData(&bloomPrefilterSetLayoutCache);
		bloomPrefilterPipeline.Create();

		// Bloom downscaling
		bloomDownscalingPipeline.SetData(&bloomDownscalingSetLayoutCache);
		bloomDownscalingPipeline.Create();

		// Bloom upscaling
		bloomUpscalingPipeline.SetData(&bloomUpscalingSetLayoutCache);
		bloomUpscalingPipeline.Create();
	}

	void BloomPass::CreateSetLayoutCaches()
	{
		// Bloom prefilter
		{
			SetLayoutSummary volatileLayout(0);
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER	, VK_SHADER_STAGE_COMPUTE_BIT);	// Input image (sampler)
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE			, VK_SHADER_STAGE_COMPUTE_BIT);	// Output image (writeonly)
			bloomPrefilterSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}

		// Bloom downscaling
		{
			SetLayoutSummary volatileLayout(0);
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Input image (readonly)
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Output image (writeonly)
			bloomDownscalingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}

		// Bloom upscaling
		{
			SetLayoutSummary volatileLayout(0);
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Input previous upscale mip (blur upsample)
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Input current downscale mip (direct sample)
			volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Output image (writeonly)
			bloomUpscalingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}
	}

	void BloomPass::CreateDescriptorSets(const DescriptorPool* descriptorPool)
	{
		// Bloom prefilter
		{
			if (bloomPrefilterSetLayoutCache.GetLayoutCount() != 1)
			{
				LogError("Failed to create bloom prefilter pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, bloomPrefilterSetLayoutCache.GetLayoutCount());
				return;
			}

			std::optional<DescriptorSetLayout> bloomPrefilterSetLayout = bloomPrefilterSetLayoutCache.GetSetLayout(0);
			if (!bloomPrefilterSetLayout.has_value())
			{
				LogError("Failed to create bloom prefilter descriptor sets! Descriptor set layout is null");
				return;
			}

			for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
			{
				bloomPrefilterDescriptorSets[i].Create(*descriptorPool, bloomPrefilterSetLayout.value());
			}
		}

		// Bloom downscaling
		{
			if (bloomDownscalingSetLayoutCache.GetLayoutCount() != 1)
			{
				LogError("Failed to create bloom downscaling pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, bloomDownscalingSetLayoutCache.GetLayoutCount());
				return;
			}

			std::optional<DescriptorSetLayout> bloomDownscalingSetLayout = bloomDownscalingSetLayoutCache.GetSetLayout(0);
			if (!bloomDownscalingSetLayout.has_value())
			{
				LogError("Failed to create bloom downscaling descriptor sets! Descriptor set layout is null");
				return;
			}

			for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
			{
				for (uint32_t j = 0; j < CONFIG::BloomMaxMips - 1; j++)
				{
					bloomDownscalingDescriptorSets[i][j].Create(*descriptorPool, bloomDownscalingSetLayout.value());
				}
			}
		}

		// Bloom upscaling
		{
			if (bloomUpscalingSetLayoutCache.GetLayoutCount() != 1)
			{
				LogError("Failed to create bloom upscaling pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, bloomUpscalingSetLayoutCache.GetLayoutCount());
				return;
			}

			std::optional<DescriptorSetLayout> bloomUpscalingSetLayout = bloomUpscalingSetLayoutCache.GetSetLayout(0);
			if (!bloomUpscalingSetLayout.has_value())
			{
				LogError("Failed to create bloom upscaling descriptor sets! Descriptor set layout is null");
				return;
			}

			for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
			{
				for (uint32_t j = 0; j < CONFIG::BloomMaxMips - 1; j++)
				{
					bloomUpscalingDescriptorSets[i][j].Create(*descriptorPool, bloomUpscalingSetLayout.value());
				}
			}
		}
	}

	void BloomPass::CreateTextures(uint32_t width, uint32_t height)
	{
		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = width;
		baseImageInfo.height = height;
		baseImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		baseImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		baseImageInfo.mipLevels = CONFIG::BloomMaxMips;
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		baseImageInfo.generateMipMaps = false;

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.viewScope = ImageViewScope::PER_MIP_LEVEL;

		// Bloom downscaling
		{
			// Transition to general layout. This is required to bind the image views of this texture to the descriptor sets
			bloomDownscalingTexture.Create(&baseImageInfo, &viewCreateInfo);
			bloomDownscalingTexture.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		}

		// Bloom upscaling
		{
			// Transition to general layout. This is required to bind the image views of this texture to the descriptor sets
			bloomUpscalingTexture.Create(&baseImageInfo, &viewCreateInfo);
			bloomUpscalingTexture.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		}
	}
}