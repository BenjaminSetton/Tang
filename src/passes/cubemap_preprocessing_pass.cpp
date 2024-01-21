
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../render_passes/base_render_pass.h"
#include "../ubo_structs.h"
#include "cubemap_preprocessing_pass.h"

static const glm::mat4 cubemapProjMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

static const glm::mat4 cubemapViewMatrices[] =
{
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // RIGHT
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // LEFT
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // DOWN
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // UP
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // RIGHT
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // LEFT
};

namespace TANG
{
	CubemapPreprocessingPass::CubemapPreprocessingPass()
	{
	}

	CubemapPreprocessingPass::~CubemapPreprocessingPass()
	{
	}

	CubemapPreprocessingPass::CubemapPreprocessingPass(CubemapPreprocessingPass&& other) noexcept
	{
		UNUSED(other);
		TNG_TODO();
	}

	void CubemapPreprocessingPass::SetData(const DescriptorPool* descriptorPool, VkExtent2D swapChainExtent)
	{
		borrowedData.descriptorPool = descriptorPool;
		borrowedData.swapChainExtent = swapChainExtent;
	}

	void CubemapPreprocessingPass::Create()
	{
		if (wasCreated)
		{
			LogWarning("Attempting to create cubemap preprocessing pass more than once!");
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

		InitializeShaderParameters();

		ResetBorrowedData();
		wasCreated = true;
	}

	void CubemapPreprocessingPass::DestroyIntermediates()
	{
		// Now that the equirectangular texture has been mapped to a cubemap, we don't need the original texture anymore
		skyboxTexture.Destroy();
	}

	void CubemapPreprocessingPass::Destroy()
	{
		cubemapPreprocessingSetLayoutCache.DestroyLayouts();

		vkDestroyFence(GetLogicalDevice(), fence, nullptr);

		irradianceSamplingFramebuffer.Destroy();
		cubemapPreprocessingFramebuffer.Destroy();
		irradianceMap.Destroy();
		skyboxCubemap.Destroy();

		for (uint32_t i = 0; i < 6; i++)
		{
			cubemapPreprocessingCubemapLayerUBO[i].Destroy();
			cubemapPreprocessingViewProjUBO[i].Destroy();
		}

		irradianceSamplingPipeline.Destroy();
		cubemapPreprocessingPipeline.Destroy();

		cubemapPreprocessingRenderPass.Destroy();

		wasCreated = false;
	}

	void CubemapPreprocessingPass::LoadTextureResources()
	{
		VkFormat texFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

		//
		// Load the skybox texture from file
		//
		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = 0; // Unused
		baseImageInfo.height = 0; // Unused
		baseImageInfo.format = texFormat;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = 0; // Unused
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;

		SamplerCreateInfo samplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 1.0f; // Is this an appropriate value??

		skyboxTexture.CreateFromFile(CONFIG::SkyboxTextureFilePath, &baseImageInfo, &viewCreateInfo, &samplerInfo);

		//
		// Create the offscreen textures that we'll render the cube faces to
		//
		baseImageInfo.width = CONFIG::SkyboxResolutionSize;
		baseImageInfo.height = CONFIG::SkyboxResolutionSize;
		baseImageInfo.format = texFormat;
		baseImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = 1;
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		baseImageInfo.arrayLayers = 6;
		baseImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		viewCreateInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 1.0f; // Is this an appropriate value??

		skyboxCubemap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);

		baseImageInfo.width = CONFIG::IrradianceMapSize;
		baseImageInfo.height = CONFIG::IrradianceMapSize;
		irradianceMap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);

		baseImageInfo.width = CONFIG::PrefilterMapSize;
		baseImageInfo.height = CONFIG::PrefilterMapSize;
		baseImageInfo.mipLevels = CONFIG::PrefilterMapMaxMips; 
		prefilterMap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
	}

	void CubemapPreprocessingPass::Preprocess(PrimaryCommandBuffer* cmdBuffer, AssetResources* asset)
	{
		CalculateSkyboxCubemap(cmdBuffer, asset);

		skyboxCubemap.TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuffer);

		CalculateIrradianceMap(cmdBuffer, asset);
		//CalculatePrefilterMap(cmdBuffer, asset);
	}

	void CubemapPreprocessingPass::Draw(uint32_t currentFrame, const DrawData& data)
	{
		// Nothing to do here, all the work is done during Preprocess()
		UNUSED(currentFrame);
		UNUSED(data);
	}

	const TextureResource* CubemapPreprocessingPass::GetSkyboxCubemap() const
	{
		return &skyboxCubemap;
	}

	const TextureResource* CubemapPreprocessingPass::GetIrradianceMap() const
	{
		return &irradianceMap;
	}

	void CubemapPreprocessingPass::CreateFramebuffers()
	{
		// Cubemap preprocessing
		{
			std::array<TextureResource*, 1> attachments =
			{
				&skyboxCubemap
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &cubemapPreprocessingRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.attachments = attachments.data();
			framebufferInfo.width = CONFIG::SkyboxResolutionSize;
			framebufferInfo.height = CONFIG::SkyboxResolutionSize;
			framebufferInfo.layers = 6;

			cubemapPreprocessingFramebuffer.Create(framebufferInfo);
		}

		// Irradiance map
		{
			std::array<TextureResource*, 1> attachments =
			{
				&irradianceMap
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &cubemapPreprocessingRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.attachments = attachments.data();
			framebufferInfo.width = CONFIG::IrradianceMapSize;
			framebufferInfo.height = CONFIG::IrradianceMapSize;
			framebufferInfo.layers = 6;

			irradianceSamplingFramebuffer.Create(framebufferInfo);
		}

		// Prefilter map
		{
			std::array<TextureResource*, 1> attachments =
			{
				&prefilterMap
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &cubemapPreprocessingRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.attachments = attachments.data();
			framebufferInfo.width = CONFIG::PrefilterMapSize;
			framebufferInfo.height = CONFIG::PrefilterMapSize;
			framebufferInfo.layers = 6;

			prefilterMapFramebuffer.Create(framebufferInfo);
		}
	}

	void CubemapPreprocessingPass::CreatePipelines()
	{
		cubemapPreprocessingPipeline.SetData(&cubemapPreprocessingRenderPass, &cubemapPreprocessingSetLayoutCache, borrowedData.swapChainExtent);
		cubemapPreprocessingPipeline.Create();

		irradianceSamplingPipeline.SetData(&cubemapPreprocessingRenderPass, &cubemapPreprocessingSetLayoutCache, borrowedData.swapChainExtent);
		irradianceSamplingPipeline.Create();

		prefilterMapPipeline.SetData(&cubemapPreprocessingRenderPass, &prefilterMapSetLayoutCache, borrowedData.swapChainExtent);
		prefilterMapPipeline.Create();
	}

	void CubemapPreprocessingPass::CreateRenderPasses()
	{
		cubemapPreprocessingRenderPass.Create();
	}

	void CubemapPreprocessingPass::CreateSetLayoutCaches()
	{
		// Cubemap preprocessing / irradiance sampling
		{
			SetLayoutSummary volatileLayout;
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);			// View/proj matrix
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT);			// Cubemap layer
			volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// Equirectangular map
			cubemapPreprocessingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}

		// Prefilter map
		{
			SetLayoutSummary volatileLayout;
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);			// View/proj matrix
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT);			// Cubemap layer
			volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// Equirectangular map
			volatileLayout.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);			// Roughness
			prefilterMapSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}
	}

	void CubemapPreprocessingPass::CreateDescriptorSets()
	{
		// Cubemap preprocessing + irradiance sampling
		// NOTE - We're re-using the cubemap preprocessing descriptor set layout because they do have
		//        the exact same layout, the problem is that we need to update the descriptors separately
		//        during subsequent render passes, which is why we must create sets for irradiance sampling
		const LayoutCache& cubemapPreprocessingCache = cubemapPreprocessingSetLayoutCache.GetLayoutCache();
		if (cubemapPreprocessingSetLayoutCache.GetLayoutCount() != 1)
		{
			TNG_ASSERT_MSG(false, "Failed to create skybox pass descriptor sets!");
			return;
		}

		for (uint32_t i = 0; i < 6; i++)
		{
			cubemapPreprocessingDescriptorSets[i].Create(*(borrowedData.descriptorPool), cubemapPreprocessingCache.begin()->second);
			irradianceSamplingDescriptorSets[i].Create(*(borrowedData.descriptorPool), cubemapPreprocessingCache.begin()->second);
		}

		// Prefilter map
		const LayoutCache& prefilterMapCache = prefilterMapSetLayoutCache.GetLayoutCache();
		if (cubemapPreprocessingSetLayoutCache.GetLayoutCount() != 1)
		{
			TNG_ASSERT_MSG(false, "Failed to create prefilter map descriptor sets!");
			return;
		}

		for (uint32_t i = 0; i < 6; i++)
		{
			prefilterMapDescriptorSets[i].Create(*(borrowedData.descriptorPool), prefilterMapCache.begin()->second);
		}
	}

	void CubemapPreprocessingPass::CreateUniformBuffers()
	{
		// Cubemap preprocessing
		{
			VkDeviceSize viewProjSize = sizeof(ViewProjUBO);
			VkDeviceSize cubemapLayerSize = sizeof(uint32_t);

			for (uint32_t i = 0; i < 6; i++)
			{
				cubemapPreprocessingViewProjUBO[i].Create(viewProjSize);
				cubemapPreprocessingViewProjUBO[i].MapMemory();

				cubemapPreprocessingCubemapLayerUBO[i].Create(cubemapLayerSize);
				cubemapPreprocessingCubemapLayerUBO[i].MapMemory();
			}
		}

		// Prefilter map
		{
			VkDeviceSize roughnessSize = sizeof(float);

			for (uint32_t i = 0; i < 6; i++)
			{
				prefilterMapRoughnessUBO[i].Create(roughnessSize);
				prefilterMapRoughnessUBO[i].MapMemory();
			}
		}
	}

	void CubemapPreprocessingPass::CreateSyncObjects()
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(GetLogicalDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS)
		{
			TNG_ASSERT_MSG(false, "Failed to create cubemap preprocessing fence!");
		}
	}

	void CubemapPreprocessingPass::InitializeShaderParameters()
	{
		// NOTE - GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
		glm::mat4 projMatrix = cubemapProjMatrix;
		projMatrix[1][1] *= -1;

		for (uint32_t i = 0; i < 6; i++)
		{
			ViewProjUBO viewProj{};
			viewProj.view = cubemapViewMatrices[i];
			viewProj.proj = projMatrix;

			// Update the view/proj matrices to look at each cubemap face
			cubemapPreprocessingViewProjUBO[i].UpdateData(&viewProj, sizeof(viewProj));

			// Update the roughness values for the prefilter map, so each mip uses a higher roughness value
			float prefilterMapRoughness = CONFIG::PrefilterMapMaxMips * (i / 6.0f);
			prefilterMapRoughnessUBO->UpdateData(&prefilterMapRoughness, sizeof(prefilterMapRoughness));

			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/gl_Layer.xhtml
			uint32_t cubemapLayer = i;
			cubemapPreprocessingCubemapLayerUBO[i].UpdateData(&cubemapLayer, sizeof(cubemapLayer));

			// Cubemap preprocessing
			{
				WriteDescriptorSets writeDescSets(2, 1);
				writeDescSets.AddUniformBuffer(cubemapPreprocessingDescriptorSets[i].GetDescriptorSet(), 0, &cubemapPreprocessingViewProjUBO[i]);
				writeDescSets.AddUniformBuffer(cubemapPreprocessingDescriptorSets[i].GetDescriptorSet(), 1, &cubemapPreprocessingCubemapLayerUBO[i]);
				writeDescSets.AddImageSampler(cubemapPreprocessingDescriptorSets[i].GetDescriptorSet(),  2, &skyboxTexture);
				cubemapPreprocessingDescriptorSets[i].Update(writeDescSets);
			}

			// Irradiance sampling
			{
				WriteDescriptorSets writeDescSets(2, 1);
				writeDescSets.AddUniformBuffer(irradianceSamplingDescriptorSets[i].GetDescriptorSet(), 0, &cubemapPreprocessingViewProjUBO[i]);
				writeDescSets.AddUniformBuffer(irradianceSamplingDescriptorSets[i].GetDescriptorSet(), 1, &cubemapPreprocessingCubemapLayerUBO[i]);
				writeDescSets.AddImageSampler(irradianceSamplingDescriptorSets[i].GetDescriptorSet(),  2, &skyboxCubemap);
				irradianceSamplingDescriptorSets[i].Update(writeDescSets);
			}

			// Prefilter map
			{
				WriteDescriptorSets writeDescSets(3, 1);
				writeDescSets.AddUniformBuffer(prefilterMapDescriptorSets[i].GetDescriptorSet(), 0, &cubemapPreprocessingViewProjUBO[i]);
				writeDescSets.AddUniformBuffer(prefilterMapDescriptorSets[i].GetDescriptorSet(), 1, &cubemapPreprocessingCubemapLayerUBO[i]);
				writeDescSets.AddImageSampler(prefilterMapDescriptorSets[i].GetDescriptorSet(),  2, &skyboxCubemap);
				writeDescSets.AddUniformBuffer(prefilterMapDescriptorSets[i].GetDescriptorSet(), 3, &(prefilterMapRoughnessUBO[i]));
				prefilterMapDescriptorSets[i].Update(writeDescSets);
			}
		}
	}

	void CubemapPreprocessingPass::CalculateSkyboxCubemap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset)
	{
		cmdBuffer->CMD_BeginRenderPass(&cubemapPreprocessingRenderPass, &cubemapPreprocessingFramebuffer, { CONFIG::SkyboxResolutionSize, CONFIG::SkyboxResolutionSize }, false, true);
		cmdBuffer->CMD_BindGraphicsPipeline(&cubemapPreprocessingPipeline);
		cmdBuffer->CMD_SetScissor({ 0, 0 }, { CONFIG::SkyboxResolutionSize, CONFIG::SkyboxResolutionSize });
		cmdBuffer->CMD_SetViewport(static_cast<float>(CONFIG::SkyboxResolutionSize), static_cast<float>(CONFIG::SkyboxResolutionSize));
		cmdBuffer->CMD_BindMesh(asset);

		// For every face of the cube, we must change our camera's view direction, change the framebuffer (since we're rendering to
		// a different texture each pass) 
		for (uint32_t i = 0; i < 6; i++)
		{
			VkDescriptorSet descriptors[1] = { cubemapPreprocessingDescriptorSets[i].GetDescriptorSet() };
			cmdBuffer->CMD_BindDescriptorSets(&cubemapPreprocessingPipeline, 1, descriptors);

			cmdBuffer->CMD_DrawIndexed(asset->indexCount);
		}

		cmdBuffer->CMD_EndRenderPass();
	}

	void CubemapPreprocessingPass::CalculateIrradianceMap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset)
	{
		cmdBuffer->CMD_BeginRenderPass(&cubemapPreprocessingRenderPass, &irradianceSamplingFramebuffer, { CONFIG::IrradianceMapSize, CONFIG::IrradianceMapSize }, false, true);
		cmdBuffer->CMD_BindGraphicsPipeline(&irradianceSamplingPipeline);
		cmdBuffer->CMD_SetScissor({ 0, 0 }, { CONFIG::IrradianceMapSize, CONFIG::IrradianceMapSize });
		cmdBuffer->CMD_SetViewport(static_cast<float>(CONFIG::IrradianceMapSize), static_cast<float>(CONFIG::IrradianceMapSize));
		cmdBuffer->CMD_BindMesh(asset);

		// For every face of the cube, we must change our camera's view direction, change the framebuffer (since we're rendering to
		// a different texture each pass) 
		for (uint32_t i = 0; i < 6; i++)
		{
			VkDescriptorSet descriptors[1] = { irradianceSamplingDescriptorSets[i].GetDescriptorSet() };
			cmdBuffer->CMD_BindDescriptorSets(&irradianceSamplingPipeline, 1, descriptors);

			cmdBuffer->CMD_DrawIndexed(asset->indexCount);
		}

		cmdBuffer->CMD_EndRenderPass();
	}

	void CubemapPreprocessingPass::CalculatePrefilterMap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset)
	{
		cmdBuffer->CMD_BeginRenderPass(&cubemapPreprocessingRenderPass, &irradianceSamplingFramebuffer, { CONFIG::PrefilterMapSize, CONFIG::PrefilterMapSize }, false, true);
		cmdBuffer->CMD_BindGraphicsPipeline(&prefilterMapPipeline);
		cmdBuffer->CMD_SetScissor({ 0, 0 }, { CONFIG::PrefilterMapSize, CONFIG::PrefilterMapSize });
		cmdBuffer->CMD_SetViewport(static_cast<float>(CONFIG::PrefilterMapSize), static_cast<float>(CONFIG::PrefilterMapSize));
		cmdBuffer->CMD_BindMesh(asset);

		// For every face of the cube, we must change our camera's view direction, change the framebuffer (since we're rendering to
		// a different texture each pass) 
		for (uint32_t i = 0; i < 6; i++)
		{
			VkDescriptorSet descriptors[1] = { prefilterMapDescriptorSets[i].GetDescriptorSet() };
			cmdBuffer->CMD_BindDescriptorSets(&prefilterMapPipeline, 1, descriptors);

			cmdBuffer->CMD_DrawIndexed(asset->indexCount);
		}

		cmdBuffer->CMD_EndRenderPass();
	}

	void CubemapPreprocessingPass::ResetBorrowedData()
	{
		memset(&borrowedData, 0, sizeof(borrowedData));
	}
}