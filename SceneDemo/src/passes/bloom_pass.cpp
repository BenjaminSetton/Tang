
#include "TANG/cmd_buffer/secondary_command_buffer.h"
#include "TANG/descriptors/descriptor_pool.h"
#include "TANG//descriptors/write_descriptor_set.h"
#include "TANG/texture_resource.h"
#include "TANG/utils/logger.h"
#include "bloom_pass.h"

#include "TANG/tang.h"

BloomPass::BloomPass()
{ }

BloomPass::~BloomPass()
{ }

void BloomPass::Create(uint32_t baseTextureWidth, uint32_t baseTextureHeight)
{
	if (wasCreated)
	{
		TANG::LogWarning("Attempting to create bloom pass more than once!");
		return;
	}

	CreateSetLayoutCaches();
	CreateDescriptorSets();
	CreatePipelines();
	CreateTextures(baseTextureWidth >> 1, baseTextureHeight >> 1); // We start the bloom pass at a quarter of the base resolution

	// Update the descriptor sets with the downscaling image view for every mip level. We're reusing the images so we can update the desc sets only once
	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		// Bloom downscaling 
		// NOTE - descriptors on first iteration are updated BEFORE starting downscaling pass, since we need to use the input texture and write to mip 0
		for (uint32_t j = 1; j < TANG::CONFIG::BloomMaxMips - 1; j++)
		{
			TANG::WriteDescriptorSets bloomDownsamplingWriteDescSets(0, 2);
			bloomDownsamplingWriteDescSets.AddImage(bloomDownscalingDescriptorSets[i][j].GetDescriptorSet(), 0, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, j - 1);	// Input image
			bloomDownsamplingWriteDescSets.AddImage(bloomDownscalingDescriptorSets[i][j].GetDescriptorSet(), 1, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, j);	// Output image
			bloomDownscalingDescriptorSets[i][j].Update(bloomDownsamplingWriteDescSets);

		}

		// Bloom upsampling
		uint32_t maxMipIndex = TANG::CONFIG::BloomMaxMips - 1;
		for (uint32_t j = 0; j < TANG::CONFIG::BloomMaxMips - 1; j++)
		{
			TANG::WriteDescriptorSets bloomUpscalingWriteDescSets(0, 3);
			bloomUpscalingWriteDescSets.AddImage(bloomUpscalingDescriptorSets[i][j].GetDescriptorSet(), 0, &bloomUpscalingTexture  , VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxMipIndex - j    );	// Input previous upscale mip (blur upsample)
			bloomUpscalingWriteDescSets.AddImage(bloomUpscalingDescriptorSets[i][j].GetDescriptorSet(), 1, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxMipIndex - j - 1);	// Input current downscale mip (direct sample)
			bloomUpscalingWriteDescSets.AddImage(bloomUpscalingDescriptorSets[i][j].GetDescriptorSet(), 2, &bloomUpscalingTexture  , VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxMipIndex - j - 1);	// Output image
			bloomUpscalingDescriptorSets[i][j].Update(bloomUpscalingWriteDescSets);
		}

		// Bloom composition
		{
			TANG::WriteDescriptorSets bloomCompositionWriteDescSets(0, 2);
			bloomCompositionWriteDescSets.AddImage(bloomCompositionDescriptorSets[i].GetDescriptorSet(), 0, &bloomUpscalingTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);	// Input bloom texture
			// Binding 2 - Input scene texture, updated before composition pass
			bloomCompositionWriteDescSets.AddImage(bloomCompositionDescriptorSets[i].GetDescriptorSet(), 2, &bloomCompositionTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);	// Output image
			bloomCompositionDescriptorSets[i].Update(bloomCompositionWriteDescSets);
		}
	}

	wasCreated = true;
}

void BloomPass::Destroy()
{
	bloomCompositionPipeline.Destroy();
	bloomUpscalingPipeline.Destroy();
	bloomDownscalingPipeline.Destroy();

	bloomCompositionTexture.Destroy();
	bloomUpscalingTexture.Destroy();
	bloomDownscalingTexture.Destroy();

	bloomCompositionSetLayoutCache.DestroyLayouts();
	bloomUpscalingSetLayoutCache.DestroyLayouts();
	bloomDownscalingSetLayoutCache.DestroyLayouts();
}

void BloomPass::Draw(uint32_t currentFrame, TANG::CommandBuffer* cmdBuffer, TANG::TextureResource* inputTexture)
{
	if (inputTexture == nullptr)
	{
		TANG::LogError("Failed to execute bloom pass, no input texture was bound!");
		return;
	}

	if (inputTexture->CalculateMipLevelsFromSize() < TANG::CONFIG::BloomMaxMips)
	{
		TANG::LogError("Size of input texture (%u, %u) is insufficient to perform a bloom pass on %u mips!",
			inputTexture->GetWidth(),
			inputTexture->GetHeight(),
			TANG::CONFIG::BloomMaxMips
		);
		return;
	}

	VkImageLayout oldLayout = inputTexture->GetLayout();
	inputTexture->TransitionLayout(cmdBuffer, oldLayout, VK_IMAGE_LAYOUT_GENERAL);
	inputTexture->InsertPipelineBarrier(cmdBuffer,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		inputTexture->GetAllocatedMipLevels()
	);

	// Update descriptor for first downscale pass to include input texture
	{
		TANG::WriteDescriptorSets bloomDownsamplingWriteDescSets(0, 2);
		bloomDownsamplingWriteDescSets.AddImage(bloomDownscalingDescriptorSets[currentFrame][0].GetDescriptorSet(), 0, inputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0); // Input image
		bloomDownsamplingWriteDescSets.AddImage(bloomDownscalingDescriptorSets[currentFrame][0].GetDescriptorSet(), 1, &bloomDownscalingTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0); // Output image
		bloomDownscalingDescriptorSets[currentFrame][0].Update(bloomDownsamplingWriteDescSets);
	}

	// The starting width and height are actually mip level 1 because of how we set up the descriptor sets
	DownscaleTexture(cmdBuffer, currentFrame);

	// Finish writing to the last mip before we use it as input for upscaling
	bloomDownscalingTexture.InsertPipelineBarrier(cmdBuffer,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		TANG::CONFIG::BloomMaxMips - 1,
		1
	);

	// Copy the last mip from the downscale texture to the upscale texture to begin upsampling process
	bloomUpscalingTexture.CopyFromTexture(cmdBuffer, &bloomDownscalingTexture, TANG::CONFIG::BloomMaxMips - 1, 1);

	// Finish copying the last mip level to the upscale texture before reading/writing from/to it in the upscaling pass
	// TODO - The garbage TransitionLayout function actually transitions all mips, so we must wait for the layout
	//        transition to happen in all mips regardless of how many we write to. This must be reworked ASAP
	bloomUpscalingTexture.InsertPipelineBarrier(cmdBuffer,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		TANG::CONFIG::BloomMaxMips
	);

	// The first mip we output to is actually the second to last mip, since we copied the last mip directly from the downsample texture
	UpscaleTexture(cmdBuffer, currentFrame);

	// Pipeline barrier to sync mip 0 of the upscale texture is inserted during upscale pass; no extra synchronization needed here
	PerformComposition(cmdBuffer, currentFrame, inputTexture);

	inputTexture->TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL, oldLayout);
}

const TANG::TextureResource* BloomPass::GetOutputTexture() const
{
	return &bloomCompositionTexture;
}

void BloomPass::DownscaleTexture(TANG::CommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	cmdBuffer->CMD_BindPipeline(&bloomDownscalingPipeline);

	float currentWidth = static_cast<float>(bloomDownscalingTexture.GetWidth());
	float currentHeight = static_cast<float>(bloomDownscalingTexture.GetHeight());
	for (uint32_t mipLevel = 0; mipLevel < TANG::CONFIG::BloomMaxMips - 1; mipLevel++)
	{
		cmdBuffer->CMD_BindDescriptorSets(&bloomDownscalingPipeline, 1, &bloomDownscalingDescriptorSets[currentFrame][mipLevel]);
		cmdBuffer->CMD_PushConstants(&bloomDownscalingPipeline, static_cast<void*>(&mipLevel), sizeof(mipLevel), VK_SHADER_STAGE_COMPUTE_BIT);

		// Dispatch as many work groups as the first mip level of the input texture, divided by the number of invocations (local_size in compute shader)
		// We starting sampling from mip level 0 (N - 1) and write to mip level 1 (N), all the way down to N = CONFIG::BloomMaxMips
		cmdBuffer->CMD_Dispatch(static_cast<uint32_t>(ceil(currentWidth / 16.0f)), static_cast<uint32_t>(ceil(currentHeight / 16.0f)), 1);

		// Finish writing to mip N - 1 before we can read from it
		bloomDownscalingTexture.InsertPipelineBarrier(cmdBuffer,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			mipLevel,
			1
		);

		// Go down a mip level
		currentWidth /= 2.0f;
		currentHeight /= 2.0f;
	}
}

void BloomPass::UpscaleTexture(TANG::CommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	cmdBuffer->CMD_BindPipeline(&bloomUpscalingPipeline);

	float currentWidth = bloomUpscalingTexture.GetWidth() / powf(2.0f, (TANG::CONFIG::BloomMaxMips - 2));
	float currentHeight = bloomUpscalingTexture.GetHeight() / powf(2.0f, (TANG::CONFIG::BloomMaxMips - 2));

	float filterRadius = TANG::CONFIG::BloomFilterRadius;
	for (uint32_t i = 0; i < TANG::CONFIG::BloomMaxMips - 1; i++)
	{
		cmdBuffer->CMD_PushConstants(&bloomDownscalingPipeline, static_cast<void*>(&filterRadius), sizeof(filterRadius), VK_SHADER_STAGE_COMPUTE_BIT);
		cmdBuffer->CMD_BindDescriptorSets(&bloomUpscalingPipeline, 1, &bloomUpscalingDescriptorSets[currentFrame][i]);

		// Dispatch as many work groups as the first mip level of the input texture, divided by the number of invocations (local_size in compute shader)
		// We starting sampling from mip level 0 (N - 1) and write to mip level 1 (N), all the way down to N = CONFIG::BloomMaxMips
		cmdBuffer->CMD_Dispatch(static_cast<uint32_t>(ceil(currentWidth / 16.0f)), static_cast<uint32_t>(ceil(currentHeight / 16.0f)), 1);

		bloomUpscalingTexture.InsertPipelineBarrier(cmdBuffer,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			TANG::CONFIG::BloomMaxMips - i - 2,
			1
		);

		// Go up a mip level
		currentWidth *= 2.0f;
		currentHeight *= 2.0f;
	}
}

void BloomPass::PerformComposition(TANG::CommandBuffer* cmdBuffer, uint32_t currentFrame, TANG::TextureResource* inputTexture)
{
	// Update descriptor set with current input scene texture on binding 2
	{
		TANG::WriteDescriptorSets bloomCompositionWriteDescSets(0, 1);
		bloomCompositionWriteDescSets.AddImage(bloomCompositionDescriptorSets[currentFrame].GetDescriptorSet(), 1, inputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);	// Input bloom texture
		bloomCompositionDescriptorSets[currentFrame].Update(bloomCompositionWriteDescSets);
	}

	cmdBuffer->CMD_BindPipeline(&bloomCompositionPipeline);

	glm::vec2 bloomData(TANG::CONFIG::BloomIntensity, TANG::CONFIG::BloomCompositionWeight); // x: bloom intensity, y: bloom mix percentage
	cmdBuffer->CMD_PushConstants(&bloomCompositionPipeline, static_cast<void*>(&bloomData), sizeof(bloomData), VK_SHADER_STAGE_COMPUTE_BIT);
	cmdBuffer->CMD_BindDescriptorSets(&bloomCompositionPipeline, 1, &bloomCompositionDescriptorSets[currentFrame]);

	// Dispatch as many work groups as the first mip level of the input texture, divided by the number of invocations (local_size in compute shader)
	uint32_t x = bloomCompositionTexture.GetWidth();
	uint32_t y = bloomCompositionTexture.GetHeight();
	cmdBuffer->CMD_Dispatch(static_cast<uint32_t>(ceil(x / 32.0)), static_cast<uint32_t>(ceil(y / 32.0)), 1);

	bloomCompositionTexture.InsertPipelineBarrier(cmdBuffer,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		1
	);
}

void BloomPass::CreatePipelines()
{
	// Bloom downscaling
	bloomDownscalingPipeline.SetData(&bloomDownscalingSetLayoutCache);
	bloomDownscalingPipeline.Create();

	// Bloom upscaling
	bloomUpscalingPipeline.SetData(&bloomUpscalingSetLayoutCache);
	bloomUpscalingPipeline.Create();

	// Bloom composition
	bloomCompositionPipeline.SetData(&bloomCompositionSetLayoutCache);
	bloomCompositionPipeline.Create();
}

void BloomPass::CreateSetLayoutCaches()
{
	// Bloom downscaling
	{
		TANG::SetLayoutSummary volatileLayout(0);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Input image (readonly)
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Output image (writeonly)
		bloomDownscalingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	// Bloom upscaling
	{
		TANG::SetLayoutSummary volatileLayout(0);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Input previous upscale mip (blur upsample)
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Input current downscale mip (direct sample)
		volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);	// Output image (writeonly)
		bloomUpscalingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	// Bloom composition
	{
		TANG::SetLayoutSummary volatileLayout(0);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);	// Input upscaled bloom texture (sampler2D)
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);	// Input scene texture (sampler2D)
		volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);			// Output image (writeonly)
		bloomCompositionSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}
}

void BloomPass::CreateDescriptorSets()
{
	// Bloom downscaling
	{
		if (bloomDownscalingSetLayoutCache.GetLayoutCount() != 1)
		{
			TANG::LogError("Failed to create bloom downscaling pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, bloomDownscalingSetLayoutCache.GetLayoutCount());
			return;
		}

		const TANG::DescriptorSetLayout* bloomDownscalingSetLayout = bloomDownscalingSetLayoutCache.GetSetLayout(0);
		if (bloomDownscalingSetLayout == nullptr)
		{
			TANG::LogError("Failed to create bloom downscaling descriptor sets! Descriptor set layout is null");
			return;
		}

		for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
		{
			for (uint32_t j = 0; j < TANG::CONFIG::BloomMaxMips; j++)
			{
				bloomDownscalingDescriptorSets[i][j] = TANG::AllocateDescriptorSet(*bloomDownscalingSetLayout);
			}
		}
	}

	// Bloom upscaling
	{
		if (bloomUpscalingSetLayoutCache.GetLayoutCount() != 1)
		{
			TANG::LogError("Failed to create bloom upscaling pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, bloomUpscalingSetLayoutCache.GetLayoutCount());
			return;
		}

		const TANG::DescriptorSetLayout* bloomUpscalingSetLayout = bloomUpscalingSetLayoutCache.GetSetLayout(0);
		if (bloomUpscalingSetLayout == nullptr)
		{
			TANG::LogError("Failed to create bloom upscaling descriptor sets! Descriptor set layout is null");
			return;
		}

		for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
		{
			for (uint32_t j = 0; j < TANG::CONFIG::BloomMaxMips; j++)
			{
				bloomUpscalingDescriptorSets[i][j] = TANG::AllocateDescriptorSet(*bloomUpscalingSetLayout);
			}
		}
	}

	// Bloom composition
	{
		if (bloomCompositionSetLayoutCache.GetLayoutCount() != 1)
		{
			TANG::LogError("Failed to create bloom composition pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, bloomCompositionSetLayoutCache.GetLayoutCount());
			return;
		}

		const TANG::DescriptorSetLayout* bloomCompositionSetLayout = bloomCompositionSetLayoutCache.GetSetLayout(0);
		if (bloomCompositionSetLayout == nullptr)
		{
			TANG::LogError("Failed to create bloom composition descriptor sets! Descriptor set layout is null");
			return;
		}

		for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
		{
			bloomCompositionDescriptorSets[i] = TANG::AllocateDescriptorSet(*bloomCompositionSetLayout);
		}
	}
}

void BloomPass::CreateTextures(uint32_t width, uint32_t height)
{
	TANG::BaseImageCreateInfo baseImageInfo{};
	baseImageInfo.width = width;
	baseImageInfo.height = height;
	baseImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	baseImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	baseImageInfo.mipLevels = TANG::CONFIG::BloomMaxMips;
	baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	baseImageInfo.generateMipMaps = false;

	TANG::ImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.viewScope = TANG::ImageViewScope::PER_MIP_LEVEL;

	TANG::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.enableAnisotropicFiltering = false;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.magnificationFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minificationFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	// Bloom downscaling
	{
		// Transition to general layout. This is required to bind the image views of this texture to the descriptor sets
		bloomDownscalingTexture.Create(&baseImageInfo, &viewCreateInfo, nullptr);
		bloomDownscalingTexture.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	}

	// Bloom upscaling
	{
		// Transition to general layout. This is required to bind the image views of this texture to the descriptor sets
		// Sampler is used during composition pass to upsample to base render resolution before adding it to a direct sample from the scene texture
		bloomUpscalingTexture.Create(&baseImageInfo, &viewCreateInfo, &samplerCreateInfo);
		bloomUpscalingTexture.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	}

	// Bloom composition
	{
		// TODO - We should really be getting the current window size from the window manager because the
		//        the window could potentially be resized at any point
		baseImageInfo.width = TANG::CONFIG::WindowWidth;
		baseImageInfo.height = TANG::CONFIG::WindowHeight;

		// Sampler is used when updating LDR descriptor set
		bloomCompositionTexture.Create(&baseImageInfo, &viewCreateInfo, &samplerCreateInfo);
		bloomCompositionTexture.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	}
}