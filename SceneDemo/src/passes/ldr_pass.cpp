
// DISABLE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)

#include <glm/gtx/euler_angles.hpp>

#pragma warning(pop)

#include "TANG/asset_types.h"
#include "TANG/base_render_pass.h"
#include "TANG/device_cache.h"
#include "TANG/framebuffer.h"
#include "TANG/logger.h"
#include "TANG/primary_command_buffer.h"
#include "TANG/sanity_check.h"
#include "TANG/tang.h"
#include "TANG/ubo_structs.h"
#include "TANG/write_descriptor_set.h"

#include "../render_passes/ldr_render_pass.h"
#include "ldr_pass.h"


LDRPass::LDRPass()
{ }

LDRPass::~LDRPass()
{ }

LDRPass::LDRPass(LDRPass&& other) noexcept
{
	UNUSED(other);
	TNG_TODO();
}

void LDRPass::UpdateExposureUniformBuffer(uint32_t frameIndex, float exposure)
{
	ldrExposureUBO[frameIndex].UpdateData(&exposure, sizeof(exposure));
}

void LDRPass::UpdateDescriptorSets(uint32_t frameIndex, const TANG::TextureResource* hdrTexture)
{
	TANG::DescriptorSet& descSet = ldrDescriptorSet[frameIndex];

	// Using final HDR texture and exposure uniform
	TANG::WriteDescriptorSets writeDescSets(1, 1);
	writeDescSets.AddImage(descSet.GetDescriptorSet(), 0, hdrTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
	writeDescSets.AddUniformBuffer(descSet.GetDescriptorSet(), 1, &ldrExposureUBO[frameIndex]);
	descSet.Update(writeDescSets);
}

void LDRPass::Create(const LDRRenderPass* ldrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight)
{
	if (wasCreated)
	{
		TANG::LogWarning("Attempting to create LDR pass more than once!");
		return;
	}

	CreateSetLayoutCaches();
	CreateUniformBuffers();
	CreateDescriptorSets();
	CreatePipelines(ldrRenderPass, swapChainWidth, swapChainHeight);

	wasCreated = true;
}

void LDRPass::Destroy()
{
	ldrSetLayoutCache.DestroyLayouts();
	ldrPipeline.Destroy();

	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		ldrExposureUBO[i].Destroy();
	}
}

void LDRPass::Draw(uint32_t frameIndex, const TANG::DrawData& data)
{
	if (!data.IsValid())
	{
		return;
	}

	data.cmdBuffer->CMD_BindMesh(data.asset);
	data.cmdBuffer->CMD_BindDescriptorSets(&ldrPipeline, 1, &ldrDescriptorSet[frameIndex]);
	data.cmdBuffer->CMD_BindPipeline(&ldrPipeline);
	data.cmdBuffer->CMD_SetScissor({ 0, 0 }, { data.framebufferWidth, data.framebufferHeight });
	data.cmdBuffer->CMD_SetViewport(static_cast<float>(data.framebufferWidth), static_cast<float>(data.framebufferHeight));
	data.cmdBuffer->CMD_DrawIndexed(data.asset->indexCount);
}

void LDRPass::CreatePipelines(const LDRRenderPass* ldrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight)
{
	ldrPipeline.SetData(ldrRenderPass, &ldrSetLayoutCache, { swapChainWidth, swapChainHeight });
	ldrPipeline.Create();
}

void LDRPass::CreateSetLayoutCaches()
{
	TANG::SetLayoutSummary layout(0);
	layout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// HDR texture
	layout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);			// Camera exposure
	ldrSetLayoutCache.CreateSetLayout(layout, 0);
}

void LDRPass::CreateDescriptorSets()
{
	if (ldrSetLayoutCache.GetLayoutCount() != 1)
	{
		TANG::LogError("Failed to create LDR descriptor set, invalid layout count! Expected (%u) vs. actual (%u)", 1, ldrSetLayoutCache.GetLayoutCount());
		return;
	}

	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		ldrDescriptorSet[i] = TANG::AllocateDescriptorSet(*(ldrSetLayoutCache.GetSetLayout(0)));
	}
}

void LDRPass::CreateUniformBuffers()
{
	VkDeviceSize ldrCameraDataUBOSize = sizeof(float);

	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		ldrExposureUBO[i].Create(ldrCameraDataUBOSize);
		ldrExposureUBO[i].MapMemory();
	}
}