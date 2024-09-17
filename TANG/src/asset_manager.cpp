
#include "asset_manager.h"
#include "default_material.h"
#include "cmd_buffer/disposable_command.h"

namespace TANG
{
	bool AssetManager::CreateAssetResources(AssetDisk* asset, CorePipeline corePipeline)
	{
		assetResources.emplace_back(AssetResources());
		resourcesMap.insert({ asset->uuid, static_cast<uint32_t>(assetResources.size() - 1) });

		AssetResources& resources = assetResources.back();

		bool succeeded = true;
		switch (corePipeline)
		{
		case CorePipeline::PBR:
		{
			succeeded = CreatePBRAssetResources(asset, resources);
			break;
		}
		case CorePipeline::CUBEMAP_PREPROCESSING:
		case CorePipeline::SKYBOX:
		{
			succeeded = CreateSkyboxAssetResources(asset, resources);
			break;
		}
		case CorePipeline::FULLSCREEN_QUAD:
		{
			succeeded = CreateFullscreenQuadAssetResources(asset, resources);
			break;
		}
		default:
		{
			TNG_ASSERT_MSG(false, "Create asset resources for pipeline is not yet implemented!");
		}
		}

		if (!succeeded)
		{
			LogError("Failed to create asset resources for asset '%s'", asset->name.c_str());
			return false;
		}

		//CreateAssetCommandBuffer(&resources);

		return &resources;
	}

	void AssetManager::DestroyAssetResources(UUID uuid)
	{
		AssetResources* asset = GetAssetResourcesFromUUID(uuid);
		if (asset == nullptr)
		{
			LogError("Failed to find asset resources for asset with uuid %ull!", uuid);
			return;
		}

		// Destroy the resources
		DestroyAssetBuffersHelper(asset);

		// Remove resources from the vector
		uint32_t resourceIndex = static_cast<uint32_t>((asset - &assetResources[0]) / sizeof(assetResources));
		assetResources.erase(assetResources.begin() + resourceIndex);

		// Destroy reference to resources
		resourcesMap.erase(uuid);

		// Remove the secondary command buffer
		//for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight(); i++)
		//{
		//	auto frameData = GetFDDAtIndex(i);
		//	auto cmdBufferIter = frameData->assetCommandBuffers.find(uuid);
		//	if (cmdBufferIter != frameData->assetCommandBuffers.end())
		//	{
		//		frameData->assetCommandBuffers.erase(cmdBufferIter);
		//	}
		//}
	}

	void AssetManager::DestroyAllAssetResources()
	{
		uint32_t numAssetResources = static_cast<uint32_t>(assetResources.size());

		for (uint32_t i = 0; i < numAssetResources; i++)
		{
			DestroyAssetBuffersHelper(&assetResources[i]);
		}

		assetResources.clear();
		resourcesMap.clear();
	}

	AssetResources* AssetManager::GetAssetResourcesFromUUID(UUID uuid)
	{
		auto resourceindexiter = resourcesMap.find(uuid);
		if (resourceindexiter == resourcesMap.end())
		{
			return nullptr;
		}

		uint32_t resourceindex = resourceindexiter->second;
		if (resourceindex >= assetResources.size())
		{
			return nullptr;
		}

		return &assetResources[resourceindex];
	}

	AssetManager::~AssetManager()
	{
		if (assetResources.size() != 0)
		{
			LogWarning("Applicating is shutting down and we still have asset resources allocated. Probably not a big deal, but might be worthwhile deleting all asset resources regardless");
		}
	}

	bool AssetManager::CreatePBRAssetResources(AssetDisk* asset, AssetResources& out_resources)
	{
		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		//////////////////////////////
		//
		//	MESH
		//
		//////////////////////////////
		Mesh<PBRVertex>* currMesh = reinterpret_cast<Mesh<PBRVertex>*>(asset->mesh);

		// Create the vertex and index buffers
		uint64_t numVertexBytes = currMesh->vertices.size() * sizeof(PBRVertex);
		VertexBuffer& vb = out_resources.vertexBuffer;
		vb.Create(numVertexBytes);

		uint64_t numIndexBytes = currMesh->indices.size() * sizeof(IndexType);
		IndexBuffer& ib = out_resources.indexBuffer;
		ib.Create(numIndexBytes);

		{
			DisposableCommand command(QUEUE_TYPE::TRANSFER, true);
			vb.CopyIntoBuffer(command.GetBuffer(), currMesh->vertices.data(), numVertexBytes);
			ib.CopyIntoBuffer(command.GetBuffer(), currMesh->indices.data(), numIndexBytes);
		}

		// Destroy the staging buffers
		vb.DestroyIntermediateBuffers();
		ib.DestroyIntermediateBuffers();

		// Accumulate the index count of this mesh;
		totalIndexCount += currMesh->indices.size();

		// Set the current offset and then increment
		out_resources.offset = vBufferOffset++;

		//////////////////////////////
		//
		//	MATERIAL
		//
		//////////////////////////////
		uint32_t numMaterials = static_cast<uint32_t>(asset->materials.size());
		TNG_ASSERT_MSG(numMaterials <= 1, "Multiple materials per asset are not currently supported!");

		if (numMaterials == 0)
		{
			// We need at least _one_ material, even if we didn't deserialize any material information
			// In this case we use a default material (look at default_material.h)
			asset->materials.resize(1);
			asset->materials[0].SetName("Default Material");
		}
		Material& material = asset->materials[0];

		// Resize to the number of possible texture types
		out_resources.material.resize(static_cast<uint32_t>(Material::TEXTURE_TYPE::_COUNT));

		// Pre-emptively fill out the texture create info, so we can just pass it to all CreateFromFile() calls
		SamplerCreateInfo samplerInfo{};
		samplerInfo.minificationFilter = VK_FILTER_LINEAR;
		samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.maxAnisotropy = 4.0;
		samplerInfo.enableAnisotropicFiltering = true;

		SamplerCreateInfo fallbackSamplerInfo{};
		fallbackSamplerInfo.minificationFilter = VK_FILTER_NEAREST;
		fallbackSamplerInfo.magnificationFilter = VK_FILTER_NEAREST;
		fallbackSamplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		fallbackSamplerInfo.maxAnisotropy = 1.0;
		fallbackSamplerInfo.enableAnisotropicFiltering = false;

		ImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.aspect = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.viewScope = ImageViewScope::ENTIRE_IMAGE;

		BaseImageCreateInfo baseImageInfo{};
		baseImageInfo.width = 0; // Unused
		baseImageInfo.height = 0; // Unused
		baseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		baseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		baseImageInfo.mipLevels = 1;
		baseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		baseImageInfo.arrayLayers = 1;
		baseImageInfo.flags = 0;

		// Default material fallback
		BaseImageCreateInfo fallbackBaseImageInfo{};
		fallbackBaseImageInfo.width = 1;
		fallbackBaseImageInfo.height = 1;
		fallbackBaseImageInfo.mipLevels = 1;
		fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		fallbackBaseImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		fallbackBaseImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		fallbackBaseImageInfo.arrayLayers = 1;
		fallbackBaseImageInfo.flags = 0;

		for (uint32_t i = 0; i < static_cast<uint32_t>(Material::TEXTURE_TYPE::_COUNT); i++)
		{
			Material::TEXTURE_TYPE texType = static_cast<Material::TEXTURE_TYPE>(i);

			// The only supported texture (currently) that stores actual colors is the diffuse map,
			// so we need to set it's format to sRGB instead of UNORM
			if (texType == Material::TEXTURE_TYPE::DIFFUSE)
			{
				baseImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
				fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			}
			else
			{
				baseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
				fallbackBaseImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			}

			if (material.HasTextureOfType(texType))
			{
				Texture* matTexture = material.GetTextureOfType(texType);
				TNG_ASSERT_MSG(matTexture != nullptr, "Why is this texture nullptr when we specifically checked against it?");

				TextureResource& texResource = out_resources.material[i];
				texResource.CreateFromFile(matTexture->fileName, &baseImageInfo, &viewCreateInfo, &samplerInfo);
			}
			else // use fallback
			{
				uint32_t data = DEFAULT_MATERIAL.at(texType);

				TextureResource& texResource = out_resources.material[i];
				texResource.Create(&fallbackBaseImageInfo, &viewCreateInfo, &fallbackSamplerInfo);
				texResource.CopyFromData(static_cast<void*>(&data), sizeof(data));
				texResource.TransitionLayout_Immediate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		// Insert the asset's uuid into the assetDrawState map. We do not render it
		// upon insertion by default
		out_resources.shouldDraw = false;
		out_resources.transform = Transform();
		out_resources.indexCount = totalIndexCount;
		out_resources.uuid = asset->uuid;

		//pbrPass.Create(&hdrRenderPass, swapChainExtent.width, swapChainExtent.height);

		//Transform defaultTransform{};
		//for (uint32_t i = 0; i < CONFIG::MaxFramesInFlight; i++)
		//{
		//	pbrPass.UpdateTransformUniformBuffer(i, defaultTransform);
		//	pbrPass.UpdateViewUniformBuffer(i, startingCameraViewMatrix);
		//	pbrPass.UpdateProjUniformBuffer(i, startingProjectionMatrix);
		//	pbrPass.UpdateCameraUniformBuffer(i, startingCameraPosition);
		//}

		return true;
	}

	bool AssetManager::CreateSkyboxAssetResources(AssetDisk* asset, AssetResources& out_resources)
	{
		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		Mesh<CubemapVertex>* currMesh = reinterpret_cast<Mesh<CubemapVertex>*>(asset->mesh);

		// Create the vertex buffer
		uint64_t numVertexBytes = currMesh->vertices.size() * sizeof(CubemapVertex);
		VertexBuffer& vb = out_resources.vertexBuffer;
		vb.Create(numVertexBytes);

		uint64_t numIndexBytes = currMesh->indices.size() * sizeof(IndexType);
		IndexBuffer& ib = out_resources.indexBuffer;
		ib.Create(numIndexBytes);
		{
			DisposableCommand command(QUEUE_TYPE::TRANSFER, true);
			vb.CopyIntoBuffer(command.GetBuffer(), currMesh->vertices.data(), numVertexBytes);
			ib.CopyIntoBuffer(command.GetBuffer(), currMesh->indices.data(), numIndexBytes);
		}

		// Destroy the staging buffers
		vb.DestroyIntermediateBuffers();
		ib.DestroyIntermediateBuffers();

		// Accumulate the index count of this mesh;
		totalIndexCount += currMesh->indices.size();

		// Set the current offset and then increment
		out_resources.offset = vBufferOffset++;

		out_resources.shouldDraw = false;
		out_resources.transform = Transform();
		out_resources.indexCount = totalIndexCount;
		out_resources.uuid = asset->uuid;

		//LogInfo("Starting cubemap preprocessing...");

		//cubemapPreprocessingPass.Create();

		//// Convert the HDR texture into a cubemap and calculate IBL components (irradiance + prefilter map + BRDF LUT)
		//PrimaryCommandBuffer cmdBuffer;
		//cmdBuffer.Allocate(QUEUE_TYPE::GRAPHICS);
		//cmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

		//cubemapPreprocessingPass.Draw(&cmdBuffer, &out_resources, GetAssetResourcesFromUUID(fullscreenQuadAssetUUID));

		//cmdBuffer.EndRecording();

		//VkCommandBuffer commandBuffers[1] = { cmdBuffer.GetBuffer() };

		//VkSubmitInfo submitInfo{};
		//submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.waitSemaphoreCount = 0;
		//submitInfo.pWaitSemaphores = nullptr;
		//submitInfo.pWaitDstStageMask = 0;
		//submitInfo.commandBufferCount = 1;
		//submitInfo.pCommandBuffers = commandBuffers;

		//VkFence cubemapPreprocessingFence = cubemapPreprocessingPass.GetFence();
		//vkResetFences(GetLogicalDevice(), 1, &cubemapPreprocessingFence);

		//if (SubmitQueue(QUEUE_TYPE::GRAPHICS, &submitInfo, 1, cubemapPreprocessingFence) != VK_SUCCESS)
		//{
		//	LogError("Failed to execute commands for cubemap preprocessing!");
		//	return;
		//}

		//// Wait for the GPU to finish preprocessing the cubemap
		//vkWaitForFences(GetLogicalDevice(), 1, &cubemapPreprocessingFence, VK_TRUE, UINT64_MAX);

		//cubemapPreprocessingPass.UpdatePrefilterMapViewScope();
		//cubemapPreprocessingPass.DestroyIntermediates();

		//LogInfo("Cubemap preprocessing done!");

		// Initialize the skybox pass
		//skyboxPass.Create(&hdrRenderPass, swapChainExtent.width, swapChainExtent.height);

		//skyboxPass.UpdateViewProjUniformBuffers(currentFrame, startingCameraViewMatrix, startingProjectionMatrix);
		//skyboxPass.UpdateSkyboxCubemap(cubemapPreprocessingPass.GetSkyboxCubemap());

		//cmdBuffer.Destroy();

		return true;
	}

	bool AssetManager::CreateFullscreenQuadAssetResources(AssetDisk* asset, AssetResources& out_resources)
	{
		uint64_t totalIndexCount = 0;
		uint32_t vBufferOffset = 0;

		Mesh<UVVertex>* currMesh = reinterpret_cast<Mesh<UVVertex>*>(asset->mesh);

		// Create the vertex buffer
		uint64_t numVertexBytes = currMesh->vertices.size() * sizeof(UVVertex);
		VertexBuffer& vb = out_resources.vertexBuffer;
		vb.Create(numVertexBytes);

		uint64_t numIndexBytes = currMesh->indices.size() * sizeof(IndexType);
		IndexBuffer& ib = out_resources.indexBuffer;
		ib.Create(numIndexBytes);

		{
			DisposableCommand command(QUEUE_TYPE::TRANSFER, true);
			vb.CopyIntoBuffer(command.GetBuffer(), currMesh->vertices.data(), numVertexBytes);
			ib.CopyIntoBuffer(command.GetBuffer(), currMesh->indices.data(), numIndexBytes);
		}

		// Destroy the staging buffers
		vb.DestroyIntermediateBuffers();
		ib.DestroyIntermediateBuffers();

		// Accumulate the index count of this mesh;
		totalIndexCount += currMesh->indices.size();

		// Set the current offset and then increment
		out_resources.offset = vBufferOffset++;

		out_resources.shouldDraw = false;
		out_resources.transform = Transform();
		out_resources.indexCount = totalIndexCount;
		out_resources.uuid = asset->uuid;

		//ldrPass.Create(&ldrRenderPass, swapChainExtent.width, swapChainExtent.height);

		return true;
	}

	void AssetManager::DestroyAssetBuffersHelper(AssetResources* resources)
	{
		// Destroy the vertex buffer
		resources->vertexBuffer.Destroy();

		// Destroy the index buffer
		resources->indexBuffer.Destroy();

		// Destroy textures
		for (auto& tex : resources->material)
		{
			tex.Destroy();
		}
	}
}