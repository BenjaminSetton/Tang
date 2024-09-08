
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/base_render_pass.h"
#include "../ubo_structs.h"

#include "skybox_pass.h"

namespace TANG
{
	SkyboxPass::SkyboxPass()
	{ }

	SkyboxPass::~SkyboxPass()
	{ }

	SkyboxPass::SkyboxPass(SkyboxPass&& other) noexcept
	{
		UNUSED(other);
		TNG_TODO();
	}

	void SkyboxPass::SetData(const DescriptorPool* descriptorPool, const HDRRenderPass* hdrRenderPass, VkExtent2D swapChainExtent)
	{
		borrowedData.descriptorPool = descriptorPool;
		borrowedData.hdrRenderPass = hdrRenderPass;
		borrowedData.swapChainExtent = swapChainExtent;
	}

	void SkyboxPass::UpdateSkyboxCubemap(const TextureResource* skyboxCubemap)
	{
		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			WriteDescriptorSets writeSetPersistent(0, 1);
			writeSetPersistent.AddImage(skyboxDescriptorSets[i][0].GetDescriptorSet(), 0, skyboxCubemap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
			skyboxDescriptorSets[i][0].Update(writeSetPersistent);
		}
	}

	void SkyboxPass::UpdateViewProjUniformBuffers(uint32_t frameIndex, const glm::mat4& view, const glm::mat4 proj)
	{
		{
			ViewUBO ubo{};
			ubo.view = view;
			viewUBO[frameIndex].UpdateData(&ubo, sizeof(ViewUBO));
		}

		{
			ProjUBO ubo{};
			ubo.proj = proj;
			projUBO[frameIndex].UpdateData(&ubo, sizeof(ProjUBO));
		}
	}

	void SkyboxPass::UpdateDescriptorSets(uint32_t frameIndex)
	{
		DescriptorSet& descSet = skyboxDescriptorSets[frameIndex][1];

		WriteDescriptorSets writeSetVolatile(2, 0);
		writeSetVolatile.AddUniformBuffer(descSet.GetDescriptorSet(), 0, &viewUBO[frameIndex]);
		writeSetVolatile.AddUniformBuffer(descSet.GetDescriptorSet(), 1, &projUBO[frameIndex]);
		descSet.Update(writeSetVolatile);
	}

	void SkyboxPass::Create()
	{
		if (wasCreated)
		{
			LogWarning("Attempting to create skybox pass more than once!");
			return;
		}

		ResetBaseMembers();

		CreateSetLayoutCaches();
		CreateUniformBuffers();
		CreateDescriptorSets();
		CreateSyncObjects();
		CreateRenderPasses();
		CreatePipelines();
		CreateFramebuffers();

		ResetBorrowedData();
		wasCreated = true;
	}

	void SkyboxPass::Destroy()
	{
		skyboxSetLayoutCache.DestroyLayouts();
		skyboxPipeline.Destroy();

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			viewUBO[i].Destroy();
			projUBO[i].Destroy();
		}
	}

	void SkyboxPass::Draw(uint32_t currentFrame, const DrawData& data)
	{
		if (!IsDrawDataValid(data))
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

	void SkyboxPass::CreatePipelines()
	{
		skyboxPipeline.SetData(borrowedData.hdrRenderPass, &skyboxSetLayoutCache, borrowedData.swapChainExtent);
		skyboxPipeline.Create();
	}

	void SkyboxPass::CreateSetLayoutCaches()
	{
		SetLayoutSummary persistentLayout(0);
		persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Skybox texture
		skyboxSetLayoutCache.CreateSetLayout(persistentLayout, 0);

		SetLayoutSummary volatileLayout(1);
		volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
		volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
		skyboxSetLayoutCache.CreateSetLayout(volatileLayout, 0);
	}

	void SkyboxPass::CreateDescriptorSets()
	{
		uint32_t skyboxSetLayoutCount = skyboxSetLayoutCache.GetLayoutCount();
		if (skyboxSetLayoutCount != 2)
		{
			LogError("Failed to create skybox descriptor set, invalid set layout count! Expected (%u) vs. actual (%u)", 2, skyboxSetLayoutCount);
			return;
		}

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			for (uint32_t j = 0; j < skyboxSetLayoutCount; j++)
			{
				std::optional<DescriptorSetLayout> setLayoutOpt = skyboxSetLayoutCache.GetSetLayout(j);
				if (!setLayoutOpt.has_value())
				{
					LogWarning("Failed to create skybox descriptor set! Set layout at %u was null", j);
					continue;
				}

				skyboxDescriptorSets[i][j].Create(*(borrowedData.descriptorPool), setLayoutOpt.value());
			}
		}
	}

	void SkyboxPass::CreateUniformBuffers()
	{
		VkDeviceSize viewUBOSize = sizeof(ViewUBO);
		VkDeviceSize projUBOSize = sizeof(ProjUBO);

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			viewUBO[i].Create(viewUBOSize);
			viewUBO[i].MapMemory();

			projUBO[i].Create(projUBOSize);
			projUBO[i].MapMemory();
		}
	}

	void SkyboxPass::ResetBorrowedData()
	{
		memset(&borrowedData, 0, sizeof(borrowedData));
	}
}