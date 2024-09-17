
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/hdr_render_pass.h"
#include "../ubo_structs.h"

#include "../tang.h"

#include "skybox_pass.h"

SkyboxPass::SkyboxPass()
{ }

SkyboxPass::~SkyboxPass()
{ }

SkyboxPass::SkyboxPass(SkyboxPass&& other) noexcept
{
	UNUSED(other);
	TNG_TODO();
}

void SkyboxPass::UpdateSkyboxCubemap(const TANG::TextureResource* skyboxCubemap)
{
	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		TANG::WriteDescriptorSets writeSetPersistent(0, 1);
		writeSetPersistent.AddImage(skyboxDescriptorSets[i][0].GetDescriptorSet(), 0, skyboxCubemap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		skyboxDescriptorSets[i][0].Update(writeSetPersistent);
	}
}

void SkyboxPass::UpdateViewProjUniformBuffers(uint32_t frameIndex, const glm::mat4& view, const glm::mat4 proj)
{
	{
		TANG::ViewUBO ubo{};
		ubo.view = view;
		viewUBO[frameIndex].UpdateData(&ubo, sizeof(TANG::ViewUBO));
	}

	{
		TANG::ProjUBO ubo{};
		ubo.proj = proj;
		projUBO[frameIndex].UpdateData(&ubo, sizeof(TANG::ProjUBO));
	}
}

void SkyboxPass::UpdateDescriptorSets(uint32_t frameIndex)
{
	TANG::DescriptorSet& descSet = skyboxDescriptorSets[frameIndex][1];

	TANG::WriteDescriptorSets writeSetVolatile(2, 0);
	writeSetVolatile.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &viewUBO[frameIndex]);
	writeSetVolatile.AddUniformBuffer(descSet.GetDescriptorSet(), 1, &projUBO[frameIndex]);
	descSet.Update(writeSetVolatile);
}

void SkyboxPass::Create(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight)
{
	if (wasCreated)
	{
		TANG::LogWarning("Attempting to create skybox pass more than once!");
		return;
	}

	CreateSetLayoutCaches();
	CreateUniformBuffers();
	CreateDescriptorSets();
	CreatePipelines(hdrRenderPass, swapChainWidth, swapChainHeight);

	wasCreated = true;
}

void SkyboxPass::Destroy()
{
	skyboxSetLayoutCache.DestroyLayouts();
	skyboxPipeline.Destroy();

	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		viewUBO[i].Destroy();
		projUBO[i].Destroy();
	}
}

void SkyboxPass::Draw(uint32_t currentFrame, const TANG::DrawData& data)
{
	if (!data.IsValid())
	{
		return;
	}

	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = nullptr;
	inheritanceInfo.renderPass = data.renderPass->GetRenderPass();
	inheritanceInfo.subpass = 0;
	inheritanceInfo.framebuffer = data.framebuffer->GetFramebuffer();

	data.cmdBuffer->BeginRecording(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, &inheritanceInfo);

	data.cmdBuffer->CMD_SetScissor({ 0, 0 }, { data.framebufferWidth, data.framebufferHeight });
	data.cmdBuffer->CMD_SetViewport(static_cast<float>(data.framebufferWidth), static_cast<float>(data.framebufferHeight));
	data.cmdBuffer->CMD_BindPipeline(&skyboxPipeline);
	data.cmdBuffer->CMD_BindMesh(data.asset);
	data.cmdBuffer->CMD_BindDescriptorSets(&skyboxPipeline, static_cast<uint32_t>(skyboxDescriptorSets.size()), skyboxDescriptorSets[currentFrame].data());
	data.cmdBuffer->CMD_DrawIndexed(data.asset->indexCount);

	data.cmdBuffer->EndRecording();
}

void SkyboxPass::CreatePipelines(const HDRRenderPass* hdrRenderPass, uint32_t swapChainWidth, uint32_t swapChainHeight)
{
	skyboxPipeline.SetData(hdrRenderPass, &skyboxSetLayoutCache, { swapChainWidth, swapChainHeight });
	skyboxPipeline.Create();
}

void SkyboxPass::CreateSetLayoutCaches()
{
	TANG::SetLayoutSummary persistentLayout(0);
	persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Skybox texture
	skyboxSetLayoutCache.CreateSetLayout(persistentLayout, 0);

	TANG::SetLayoutSummary volatileLayout(1);
	volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
	volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
	skyboxSetLayoutCache.CreateSetLayout(volatileLayout, 0);
}

void SkyboxPass::CreateDescriptorSets()
{
	uint32_t skyboxSetLayoutCount = skyboxSetLayoutCache.GetLayoutCount();
	if (skyboxSetLayoutCount != 2)
	{
		TANG::LogError("Failed to create skybox descriptor set, invalid set layout count! Expected (%u) vs. actual (%u)", 2, skyboxSetLayoutCount);
		return;
	}

	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		for (uint32_t j = 0; j < skyboxSetLayoutCount; j++)
		{
			std::optional<TANG::DescriptorSetLayout> setLayoutOpt = skyboxSetLayoutCache.GetSetLayout(j);
			if (!setLayoutOpt.has_value())
			{
				TANG::LogWarning("Failed to create skybox descriptor set! Set layout at %u was null", j);
				continue;
			}

			skyboxDescriptorSets[i][j] = TANG::AllocateDescriptorSet(setLayoutOpt.value());
		}
	}
}

void SkyboxPass::CreateUniformBuffers()
{
	VkDeviceSize viewUBOSize = sizeof(TANG::ViewUBO);
	VkDeviceSize projUBOSize = sizeof(TANG::ProjUBO);

	for (uint32_t i = 0; i < TANG::CONFIG::MaxFramesInFlight; i++)
	{
		viewUBO[i].Create(viewUBOSize);
		viewUBO[i].MapMemory();

		projUBO[i].Create(projUBOSize);
		projUBO[i].MapMemory();
	}
}