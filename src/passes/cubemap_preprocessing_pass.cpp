
#include "../asset_types.h"
#include "../cmd_buffer/primary_command_buffer.h"
#include "../cmd_buffer/secondary_command_buffer.h"
#include "../descriptors/write_descriptor_set.h"
#include "../device_cache.h"
#include "../render_passes/base_render_pass.h"
#include "../ubo_structs.h"
#include "cubemap_preprocessing_pass.h"

// TMEPO DEBUG
#include "../device_cache.h"

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
		prefilterMapRoughnessSetLayoutCache.DestroyLayouts();
		prefilterMapCubemapSetLayoutCache.DestroyLayouts();
		cubemapPreprocessingSetLayoutCache.DestroyLayouts();

		vkDestroyFence(GetLogicalDevice(), fence, nullptr);

		brdfConvolutionFramebuffer.Destroy();
		for (auto& prefilterMapFramebuffer : prefilterMapFramebuffers)
		{
			prefilterMapFramebuffer.Destroy();
		}
		irradianceSamplingFramebuffer.Destroy();
		cubemapPreprocessingFramebuffer.Destroy();

		brdfConvolutionMap.Destroy();
		prefilterMap.Destroy();
		irradianceMap.Destroy();
		skyboxCubemapMipped.Destroy();
		skyboxCubemap.Destroy();

		for (uint32_t i = 0; i < 6; i++)
		{
			cubemapPreprocessingCubemapLayerUBO[i].Destroy();
			cubemapPreprocessingViewProjUBO[i].Destroy();
			prefilterMapRoughnessUBO[i].Destroy();
		}

		brdfConvolutionPipeline.Destroy();
		prefilterMapPipeline.Destroy();
		irradianceSamplingPipeline.Destroy();
		cubemapPreprocessingPipeline.Destroy();

		brdfConvolutionRenderPass.Destroy();
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
		baseImageInfo.mipLevels = 1;
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		baseImageInfo.generateMipMaps = false;

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;

		SamplerCreateInfo samplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.enableAnisotropicFiltering = false;
		samplerInfo.maxAnisotropy = 1.0f;

		skyboxTexture.CreateFromFile(CONFIG::SkyboxTextureFilePath, &baseImageInfo, &viewCreateInfo, &samplerInfo);

		//
		// Create the offscreen textures that we'll render the cube faces to
		//

		// Skybox cubemap
		{
			baseImageInfo.width = CONFIG::SkyboxCubemapSize;
			baseImageInfo.height = CONFIG::SkyboxCubemapSize;
			baseImageInfo.format = texFormat;
			baseImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			baseImageInfo.mipLevels = 1;
			baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			baseImageInfo.arrayLayers = 6;
			baseImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			baseImageInfo.generateMipMaps = false;

			viewCreateInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			viewCreateInfo.viewScope = ImageViewScope::ENTIRE_IMAGE;

			samplerInfo.minificationFilter = VK_FILTER_LINEAR;
			samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			skyboxCubemap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
		}

		// Skybox cubemap mipped
		{
			baseImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			baseImageInfo.mipLevels = CONFIG::PrefilterMapMaxMips;
			baseImageInfo.generateMipMaps = false;

			skyboxCubemapMipped.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
		}

		// Irradiance map
		{
			baseImageInfo.width = CONFIG::IrradianceMapSize;
			baseImageInfo.height = CONFIG::IrradianceMapSize;
			baseImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			baseImageInfo.mipLevels = 1;
			baseImageInfo.generateMipMaps = false;

			irradianceMap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
		}

		// Prefilter map
		{
			VkPhysicalDeviceProperties deviceProps = DeviceCache::Get().GetPhysicalDeviceProperties();
			float maxAnisotropy = deviceProps.limits.maxSamplerAnisotropy;

			baseImageInfo.width = CONFIG::PrefilterMapSize;
			baseImageInfo.height = CONFIG::PrefilterMapSize;
			baseImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			baseImageInfo.mipLevels = CONFIG::PrefilterMapMaxMips;
			baseImageInfo.generateMipMaps = true;

			viewCreateInfo.viewScope = ImageViewScope::PER_MIP_LEVEL;

			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			samplerInfo.enableAnisotropicFiltering = true;
			samplerInfo.maxAnisotropy = maxAnisotropy;

			prefilterMap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
		}

		// BRDF convolution map
		{
			baseImageInfo.width = CONFIG::BRDFConvolutionMapSize;
			baseImageInfo.height = CONFIG::BRDFConvolutionMapSize;
			baseImageInfo.format = VK_FORMAT_R16G16_SFLOAT; // Two 16-bit components, floating point precision format
			baseImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			baseImageInfo.flags = 0;
			baseImageInfo.arrayLayers = 1;
			baseImageInfo.mipLevels = 1;
			baseImageInfo.generateMipMaps = false;

			viewCreateInfo.viewScope = ImageViewScope::ENTIRE_IMAGE;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.enableAnisotropicFiltering = false;
			samplerInfo.maxAnisotropy = 1.0f;

			brdfConvolutionMap.Create(&baseImageInfo, &viewCreateInfo, &samplerInfo);
		}
	}

	void CubemapPreprocessingPass::Preprocess(PrimaryCommandBuffer* cmdBuffer, AssetResources* cubemap, AssetResources* fullscreenQuad)
	{
		CalculateSkyboxCubemap(cmdBuffer, cubemap);

		// Copy the skybox cubemap over to the mipped texture and generate the mip maps
		skyboxCubemapMipped.CopyFromTexture(cmdBuffer, &skyboxCubemap, 1);
		skyboxCubemapMipped.GenerateMipmaps(cmdBuffer, CONFIG::PrefilterMapMaxMips);

		// Update the descriptor sets using the skybox cubemap after it's layout has been transitioned, including
		// the irradiance sampling pass and prefilter map pass
		for (uint32_t i = 0; i < 6; i++)
		{
			// Irradiance sampling
			{
				WriteDescriptorSets irradianceSamplingWriteDescSets(0, 1);
				irradianceSamplingWriteDescSets.AddImageSampler(irradianceSamplingDescriptorSets[i].GetDescriptorSet(), 2, &skyboxCubemapMipped);
				irradianceSamplingDescriptorSets[i].Update(irradianceSamplingWriteDescSets);
			}

			// Prefilter map
			{
				WriteDescriptorSets prefilterMapWriteDescSets(0, 1);
				prefilterMapWriteDescSets.AddImageSampler(prefilterMapCubemapDescriptorSets[i].GetDescriptorSet(), 2, &skyboxCubemapMipped);
				prefilterMapCubemapDescriptorSets[i].Update(prefilterMapWriteDescSets);
			}
		}

		CalculateIrradianceMap(cmdBuffer, cubemap);
		CalculatePrefilterMap(cmdBuffer, cubemap);
		CalculateBRDFConvolution(cmdBuffer, fullscreenQuad);
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

	const TextureResource* CubemapPreprocessingPass::GetPrefilterMap() const
	{
		return &prefilterMap;
	}

	const TextureResource* CubemapPreprocessingPass::GetBRDFConvolutionMap() const
	{
		return &brdfConvolutionMap;
	}

	void CubemapPreprocessingPass::UpdatePrefilterMapViewScope()
	{
		// Recreate the image views of the prefilter map to ENTIRE_IMAGE, since we want to sample from all mips
		ImageViewCreateInfo viewInfo{};
		viewInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.viewScope = ImageViewScope::ENTIRE_IMAGE;
		prefilterMap.RecreateImageViews(&viewInfo);
	}

	void CubemapPreprocessingPass::CreateFramebuffers()
	{
		// Skybox cubemap
		{
			std::vector<TextureResource*> attachments =
			{
				&skyboxCubemap
			};

			std::vector<uint32_t> imageViewIndices =
			{
				0
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &cubemapPreprocessingRenderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = CONFIG::SkyboxCubemapSize;
			framebufferInfo.height = CONFIG::SkyboxCubemapSize;
			framebufferInfo.layers = 6;

			cubemapPreprocessingFramebuffer.Create(framebufferInfo);
		}

		// Irradiance map
		{
			std::vector<TextureResource*> attachments =
			{
				&irradianceMap
			};

			std::vector<uint32_t> imageViewIndices =
			{
				0
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &cubemapPreprocessingRenderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = CONFIG::IrradianceMapSize;
			framebufferInfo.height = CONFIG::IrradianceMapSize;
			framebufferInfo.layers = 6;

			irradianceSamplingFramebuffer.Create(framebufferInfo);
		}

		// Prefilter map
		{
			std::vector<TextureResource*> attachments =
			{
				&prefilterMap
			};

			std::vector<uint32_t> imageViewIndices =
			{
				0
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &cubemapPreprocessingRenderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = CONFIG::PrefilterMapSize;
			framebufferInfo.height = CONFIG::PrefilterMapSize;
			framebufferInfo.layers = 6;

			for (uint32_t i = 0; i < CONFIG::PrefilterMapMaxMips; i++)
			{
				// Change the image view to every mip level
				framebufferInfo.imageViewIndices[0] = i;

				prefilterMapFramebuffers[i].Create(framebufferInfo);

				// Reduce the resolution of the framebuffer to match the mip level
				framebufferInfo.width >>= 1;
				framebufferInfo.height >>= 1;
			}
		}

		// BRDF convolution map
		{
			std::vector<TextureResource*> attachments =
			{
				&brdfConvolutionMap
			};

			std::vector<uint32_t> imageViewIndices =
			{
				0
			};

			FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = &brdfConvolutionRenderPass;
			framebufferInfo.attachments = attachments;
			framebufferInfo.imageViewIndices = imageViewIndices;
			framebufferInfo.width = CONFIG::BRDFConvolutionMapSize;
			framebufferInfo.height = CONFIG::BRDFConvolutionMapSize;
			framebufferInfo.layers = 1;

			brdfConvolutionFramebuffer.Create(framebufferInfo);
		}
	}

	void CubemapPreprocessingPass::CreatePipelines()
	{
		cubemapPreprocessingPipeline.SetData(&cubemapPreprocessingRenderPass, &cubemapPreprocessingSetLayoutCache, { CONFIG::SkyboxCubemapSize, CONFIG::SkyboxCubemapSize });
		cubemapPreprocessingPipeline.Create();

		irradianceSamplingPipeline.SetData(&cubemapPreprocessingRenderPass, &cubemapPreprocessingSetLayoutCache, { CONFIG::IrradianceMapSize, CONFIG::IrradianceMapSize });
		irradianceSamplingPipeline.Create();

		prefilterMapPipeline.SetData(&cubemapPreprocessingRenderPass, &prefilterMapCubemapSetLayoutCache, &prefilterMapRoughnessSetLayoutCache, { CONFIG::PrefilterMapSize, CONFIG::PrefilterMapSize });
		prefilterMapPipeline.Create();

		brdfConvolutionPipeline.SetData(&brdfConvolutionRenderPass, { CONFIG::BRDFConvolutionMapSize, CONFIG::BRDFConvolutionMapSize });
		brdfConvolutionPipeline.Create();
	}

	void CubemapPreprocessingPass::CreateRenderPasses()
	{
		cubemapPreprocessingRenderPass.Create();
		brdfConvolutionRenderPass.Create();
	}

	void CubemapPreprocessingPass::CreateSetLayoutCaches()
	{
		// Cubemap preprocessing / irradiance sampling
		{
			SetLayoutSummary volatileLayout(0);
			volatileLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);			// View/proj matrix
			volatileLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT);			// Cubemap layer
			volatileLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// Equirectangular map
			cubemapPreprocessingSetLayoutCache.CreateSetLayout(volatileLayout, 0);
		}

		// Prefilter map
		{
			SetLayoutSummary cubemapLayout(0);
			cubemapLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);				// View/proj matrix
			cubemapLayout.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT);			// Cubemap layer
			cubemapLayout.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// Equirectangular map
			prefilterMapCubemapSetLayoutCache.CreateSetLayout(cubemapLayout, 0);

			SetLayoutSummary roughnessLayout(0);
			roughnessLayout.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);			// Roughness
			prefilterMapRoughnessSetLayoutCache.CreateSetLayout(roughnessLayout, 0);
		}
	}

	void CubemapPreprocessingPass::CreateDescriptorSets()
	{
		// Cubemap preprocessing + irradiance sampling
		// NOTE - We're re-using the cubemap preprocessing descriptor set layout because they do have
		//        the exact same layout, the problem is that we need to update the descriptors separately
		//        during subsequent render passes, which is why we must create sets for irradiance sampling
		if (cubemapPreprocessingSetLayoutCache.GetLayoutCount() != 1)
		{
			LogError("Failed to create skybox pass descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, cubemapPreprocessingSetLayoutCache.GetLayoutCount());
			return;
		}

		std::optional<DescriptorSetLayout> cubemapSetLayout = cubemapPreprocessingSetLayoutCache.GetSetLayout(0);
		if (!cubemapSetLayout.has_value())
		{
			LogError("Failed to create cubemap preprocessing descriptor sets! Descriptor set layout is null");
			return;
		}

		for (uint32_t i = 0; i < 6; i++)
		{
			cubemapPreprocessingDescriptorSets[i].Create(*(borrowedData.descriptorPool), cubemapSetLayout.value());
			irradianceSamplingDescriptorSets[i].Create(*(borrowedData.descriptorPool), cubemapSetLayout.value());
		}

		// Prefilter map
		if (prefilterMapCubemapSetLayoutCache.GetLayoutCount() != 1)
		{
			LogError("Failed to create prefilter map cubemap descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, prefilterMapCubemapSetLayoutCache.GetLayoutCount());
			return;
		}
		if (prefilterMapRoughnessSetLayoutCache.GetLayoutCount() != 1)
		{
			LogError("Failed to create prefilter map roughness descriptor sets, too many layouts! Expected (%u) vs. actual (%u)", 1, prefilterMapRoughnessSetLayoutCache.GetLayoutCount());
			return;
		}

		std::optional<DescriptorSetLayout> prefilterCubemapSetLayout = prefilterMapCubemapSetLayoutCache.GetSetLayout(0);
		if (!prefilterCubemapSetLayout.has_value())
		{
			LogError("Failed to create prefilter cubemap descriptor sets! Descriptor set layout is null");
			return;
		}

		std::optional<DescriptorSetLayout> prefilterRoughnessSetLayout = prefilterMapRoughnessSetLayoutCache.GetSetLayout(0);
		if (!prefilterRoughnessSetLayout.has_value())
		{
			LogError("Failed to create prefilter roughness descriptor sets! Descriptor set layout is null");
			return;
		}

		for (uint32_t i = 0; i < 6; i++)
		{
			prefilterMapCubemapDescriptorSets[i].Create(*(borrowedData.descriptorPool), prefilterCubemapSetLayout.value());
			prefilterMapRoughnessDescriptorSets[i].Create(*(borrowedData.descriptorPool), prefilterRoughnessSetLayout.value());
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

			// Update the roughness values for the prefilter map, so each subsequent mip uses a higher roughness value (from 0-1)
			float prefilterMapRoughness = (i / 5.0f);
			prefilterMapRoughnessUBO[i].UpdateData(&prefilterMapRoughness, sizeof(prefilterMapRoughness));

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
				WriteDescriptorSets writeDescSets(2, 0);
				writeDescSets.AddUniformBuffer(irradianceSamplingDescriptorSets[i].GetDescriptorSet(), 0, &cubemapPreprocessingViewProjUBO[i]);
				writeDescSets.AddUniformBuffer(irradianceSamplingDescriptorSets[i].GetDescriptorSet(), 1, &cubemapPreprocessingCubemapLayerUBO[i]);
				irradianceSamplingDescriptorSets[i].Update(writeDescSets);
			}

			// Prefilter map - cubemap
			{
				WriteDescriptorSets writeDescSets(2, 0);
				writeDescSets.AddUniformBuffer(prefilterMapCubemapDescriptorSets[i].GetDescriptorSet(), 0, &cubemapPreprocessingViewProjUBO[i]);
				writeDescSets.AddUniformBuffer(prefilterMapCubemapDescriptorSets[i].GetDescriptorSet(), 1, &cubemapPreprocessingCubemapLayerUBO[i]);
				prefilterMapCubemapDescriptorSets[i].Update(writeDescSets);
			}

			// Prefilter map - roughness
			{
				WriteDescriptorSets writeDescSets(1, 0);
				writeDescSets.AddUniformBuffer(prefilterMapRoughnessDescriptorSets[i].GetDescriptorSet(), 0, &(prefilterMapRoughnessUBO[i]));
				prefilterMapRoughnessDescriptorSets[i].Update(writeDescSets);
			}
		}
	}

	void CubemapPreprocessingPass::CalculateSkyboxCubemap(PrimaryCommandBuffer* cmdBuffer, const AssetResources* asset)
	{
		cmdBuffer->CMD_BeginRenderPass(&cubemapPreprocessingRenderPass, &cubemapPreprocessingFramebuffer, { CONFIG::SkyboxCubemapSize, CONFIG::SkyboxCubemapSize }, false, true);
		cmdBuffer->CMD_BindGraphicsPipeline(&cubemapPreprocessingPipeline);
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
		uint32_t renderAreaSize = CONFIG::PrefilterMapSize;

		// Iterate over all the mip levels
		for (uint32_t i = 0; i < CONFIG::PrefilterMapMaxMips; i++)
		{
			cmdBuffer->CMD_BeginRenderPass(&cubemapPreprocessingRenderPass, &prefilterMapFramebuffers[i], { renderAreaSize, renderAreaSize }, false, true);
			cmdBuffer->CMD_BindGraphicsPipeline(&prefilterMapPipeline);
			cmdBuffer->CMD_SetScissor({ 0, 0 }, { renderAreaSize, renderAreaSize });
			cmdBuffer->CMD_SetViewport(static_cast<float>(renderAreaSize), static_cast<float>(renderAreaSize));
			cmdBuffer->CMD_BindMesh(asset);

			// For every face of the cube, we must change our camera's view direction, change the framebuffer (since we're rendering to
			// a different texture each pass) 
			for (uint32_t j = 0; j < 6; j++)
			{
				VkDescriptorSet descriptors[2] = 
				{ 
					prefilterMapCubemapDescriptorSets[j].GetDescriptorSet(),
					prefilterMapRoughnessDescriptorSets[i].GetDescriptorSet()
				};
				cmdBuffer->CMD_BindDescriptorSets(&prefilterMapPipeline, 2, descriptors);

				cmdBuffer->CMD_DrawIndexed(asset->indexCount);
			}

			cmdBuffer->CMD_EndRenderPass();

			// We're moving onto a new mip-map, so reduce the render area size by half. Since this variable is used for
			// both the width and height the effective size reduction is actually 4x (2x in every dimension)
			renderAreaSize /= 2;
		}
	}

	void CubemapPreprocessingPass::CalculateBRDFConvolution(PrimaryCommandBuffer* cmdBuffer, const AssetResources* fullscreenQuad)
	{
		cmdBuffer->CMD_BeginRenderPass(&brdfConvolutionRenderPass, &brdfConvolutionFramebuffer, { CONFIG::BRDFConvolutionMapSize, CONFIG::BRDFConvolutionMapSize }, false, true);
		cmdBuffer->CMD_BindGraphicsPipeline(&brdfConvolutionPipeline);
		cmdBuffer->CMD_BindMesh(fullscreenQuad);
		cmdBuffer->CMD_DrawIndexed(fullscreenQuad->indexCount);

		cmdBuffer->CMD_EndRenderPass();
	}

	void CubemapPreprocessingPass::ResetBorrowedData()
	{
		memset(&borrowedData, 0, sizeof(borrowedData));
	}
}