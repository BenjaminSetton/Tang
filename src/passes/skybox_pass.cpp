
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../framebuffer.h"
#include "../render_passes/base_render_pass.h"
#include "skybox_pass.h"

namespace TANG
{
	SkyboxPass::SkyboxPass()
	{
	}

	SkyboxPass::~SkyboxPass()
	{
	}

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

	void SkyboxPass::UpdateSkyboxCubemapShaderParameter(const TextureResource* skyboxCubemap)
	{
		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			WriteDescriptorSets writeSetPersistent(0, 1);
			writeSetPersistent.AddImageSampler(skyboxDescriptorSets[i][0].GetDescriptorSet(), 0, skyboxCubemap);
			skyboxDescriptorSets[i][0].Update(writeSetPersistent);
		}
	}

	void SkyboxPass::UpdateCameraMatricesShaderParameters(uint32_t frameIndex, const UniformBuffer* view, const UniformBuffer* proj)
	{
		WriteDescriptorSets writeSetVolatile(2, 0);
		writeSetVolatile.AddUniformBuffer(skyboxDescriptorSets[frameIndex][1].GetDescriptorSet(), 0, view);
		writeSetVolatile.AddUniformBuffer(skyboxDescriptorSets[frameIndex][1].GetDescriptorSet(), 1, proj);
		skyboxDescriptorSets[frameIndex][1].Update(writeSetVolatile);
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
		data.cmdBuffer->CMD_BindGraphicsPipeline(&skyboxPipeline);
		data.cmdBuffer->CMD_BindMesh(data.asset);
		data.cmdBuffer->CMD_BindDescriptorSets(&skyboxPipeline, static_cast<uint32_t>(skyboxDescriptorSets.size()), reinterpret_cast<VkDescriptorSet*>(skyboxDescriptorSets[currentFrame].data()));
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
		{
			SetLayoutSummary persistentLayout;
			persistentLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // Skybox texture
			skyboxSetLayoutCache.CreateSetLayout(persistentLayout, 0);

			SetLayoutSummary volatileLayout;
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // View matrix
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // Projection matrix
			skyboxSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}
	}

	void SkyboxPass::CreateDescriptorSets()
	{
		const LayoutCache& cache = skyboxSetLayoutCache.GetLayoutCache();
		if (skyboxSetLayoutCache.GetLayoutCount() != 2)
		{
			TNG_ASSERT_MSG(false, "Failed to create skybox descriptor set!");
			return;
		}

		for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		{
			uint32_t j = 0;
			for (auto& iter : cache)
			{
				skyboxDescriptorSets[i][j].Create(*(borrowedData.descriptorPool), iter.second);
				j++;
			}
		}
	}

	void SkyboxPass::ResetBorrowedData()
	{
		memset(&borrowedData, 0, sizeof(borrowedData));
	}
}