
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <time.h>
#include <vector>

#include "tang.h"
#include "utils/sanity_check.h"
#include "utils/logger.h"

// TEMP - This will be moved over to SceneDemo project
#include "passes/bloom_pass.h"
#include "passes/cubemap_preprocessing_pass.h"
#include "passes/ldr_pass.h"
#include "passes/pbr_pass.h"
#include "passes/skybox_pass.h"

#include "render_passes/hdr_render_pass.h"
#include "render_passes/ldr_render_pass.h"

#include "asset_types.h"
#include "texture_resource.h"
#include "framebuffer.h"
#include "camera/freefly_camera.h"

// DISABLE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
// 4244: warning C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(push)
#pragma warning(disable : 4201 4244)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// TEMP

struct MyAsset
{
	MyAsset(std::string _name) : name(_name), uuid(TANG::INVALID_UUID)
	{ }

    MyAsset(std::string _name, TANG::UUID _uuid) : name(_name), uuid(_uuid)
    { }

    std::string name;
    TANG::UUID uuid;
    float pos[3]   = {0.0f, 0.0f, 0.0f};
    float rot[3]   = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
};

static std::vector<std::string> assetNames =
{
    "../src/data/assets/brass_vase/scene.gltf"
};

int RandomRangeInt(int min, int max)
{
    return rand() % (max - min) + min;
}

float RandomRangeFloat(float min, float max)
{
    return (rand() / static_cast<float>(RAND_MAX)) * (max - min) + min;
}

// PASSES
static BloomPass bloomPass;
static SkyboxPass skyboxPass;
static CubemapPreprocessingPass cubemapPreprocessingPass;
static PBRPass pbrPass;
static LDRPass ldrPass;
// PASSES

// RENDER PASSES
	// Some terminology about render passes from Reddit thread (https://www.reddit.com/r/vulkan/comments/a27cid/what_is_an_attachment_in_the_render_passes/):
	//
	// An image is just a piece of memory with some metadata about layout, format etc. A Framebuffer is a container for multiple images 
	// with additional metadata for each image, like usage, identifier(index) and type(color, depth, etc.). 
	// These images used in a framebuffer are called attachments, because they are attached to, and owned by, a framebuffer.
	// 
	// Attachments that get rendered to are called Render Targets, Attachments which are used as input are called Input Attachments.
	// 
	// Attachments which hold information about Multisampling are called Resolve Attachments.
	// 
	// Attachments with RGB/Depth/Stencil information are called Color/Depth/Stencil Attachments respectively.
static HDRRenderPass hdrRenderPass;
static LDRRenderPass ldrRenderPass;
// RENDER PASSES

// FRAMEBUFFER + FRAMEBUFFER RESOURCES
static std::array<TANG::TextureResource, TANG::GetMaxFramesInFlight()> hdrColorAttachments;
static std::array<TANG::TextureResource, TANG::GetMaxFramesInFlight()> depthAttachments;
static std::array<TANG::Framebuffer, TANG::GetMaxFramesInFlight()> hdrFramebuffers;
// FRAMEBUFFER + FRAMEBUFFER RESOURCES

// SYNC OBJECTS
static std::array<VkSemaphore, TANG::GetMaxFramesInFlight()> coreRenderFinishedSemaphore;
static std::array<VkSemaphore, TANG::GetMaxFramesInFlight()> postProcessingFinishedSemaphore;
// SYNC OBJECTS

// MISC
static glm::vec3 startingCameraPosition;
static glm::mat4 startingViewMatrix;
static glm::mat4 startingProjMatrix;
static FreeflyCamera camera;
// MISC

void CreatePasses(uint32_t windowWidth, uint32_t windowHeight)
{
    bloomPass.Create(windowWidth, windowHeight);
    skyboxPass.Create(&hdrRenderPass, windowWidth, windowHeight);
    cubemapPreprocessingPass.Create();
    pbrPass.Create(&hdrRenderPass, windowWidth, windowHeight);
    ldrPass.Create(&ldrRenderPass, windowWidth, windowHeight);
}

void DestroyPasses()
{
    ldrPass.Destroy();
    pbrPass.Destroy();
    cubemapPreprocessingPass.Destroy();
    skyboxPass.Destroy();
    bloomPass.Destroy();
}

void CreateRenderPasses()
{
    hdrRenderPass.Create();
    ldrRenderPass.Create();
}

void DestroyRenderPasses()
{
    ldrRenderPass.Destroy();
    hdrRenderPass.Destroy();
}

void CreateFramebuffer(uint32_t windowWidth, uint32_t windowHeight)
{
    // Depth attachment
    {
        TANG::BaseImageCreateInfo imageInfo{};
        imageInfo.width = windowWidth;
        imageInfo.height = windowHeight;
        imageInfo.format = TANG::FindDepthFormat();
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.mipLevels = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.arrayLayers = 1;
        imageInfo.flags = 0;

        TANG::ImageViewCreateInfo imageViewInfo{};
        imageViewInfo.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        TANG::SamplerCreateInfo samplerInfo{};
        samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
        samplerInfo.minificationFilter = VK_FILTER_LINEAR;
        samplerInfo.enableAnisotropicFiltering = false;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.enableAnisotropicFiltering = false;

        for (uint32_t i = 0; i < TANG::GetMaxFramesInFlight(); i++)
        {
            depthAttachments[i].Create(&imageInfo, &imageViewInfo, &samplerInfo);
        }
    }

    // HDR attachment
    {
        TANG::BaseImageCreateInfo imageInfo{};
        imageInfo.width = windowWidth;
        imageInfo.height = windowHeight;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageInfo.mipLevels = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.arrayLayers = 1;
        imageInfo.flags = 0;

		TANG::ImageViewCreateInfo imageViewInfo{};
		imageViewInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.viewScope = TANG::ImageViewScope::ENTIRE_IMAGE;

        TANG::SamplerCreateInfo samplerInfo{};
        samplerInfo.addressModeUVW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.magnificationFilter = VK_FILTER_LINEAR;
        samplerInfo.minificationFilter = VK_FILTER_LINEAR;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.enableAnisotropicFiltering = false;

        for (uint32_t i = 0; i < TANG::GetMaxFramesInFlight(); i++)
        {
            hdrColorAttachments[i].Create(&imageInfo, &imageViewInfo, &samplerInfo);
        }
    }

    // Framebuffer
    {
	    for (uint32_t i = 0; i < TANG::GetMaxFramesInFlight(); i++)
	    {
		    std::vector<TANG::TextureResource*> attachments =
		    {
			    &(hdrColorAttachments[i]),
			    &(depthAttachments[i])
		    };

		    std::vector<uint32_t> imageViewIndices =
		    {
			    0,
			    0
		    };

		    TANG::FramebufferCreateInfo framebufferInfo{};
		    framebufferInfo.renderPass = &hdrRenderPass;
		    framebufferInfo.attachments = attachments;
		    framebufferInfo.imageViewIndices = imageViewIndices;
		    framebufferInfo.width = windowWidth;
		    framebufferInfo.height = windowHeight;
		    framebufferInfo.layers = 1;

		    hdrFramebuffers[i].Create(framebufferInfo);
	    }
    }
}

void DestroyFramebuffer()
{
    for (uint32_t i = 0; i < TANG::GetMaxFramesInFlight(); i++)
    {
        hdrColorAttachments[i].Destroy();
        depthAttachments[i].Destroy();
        hdrFramebuffers[i].Destroy();
    }
}

void CreateSyncObjects()
{
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < TANG::GetMaxFramesInFlight(); i++)
    {
        TANG::CreateSemaphore(&coreRenderFinishedSemaphore[i], info);
        TANG::CreateSemaphore(&postProcessingFinishedSemaphore[i], info);
    }
}

void DestroySyncObjects()
{
	for (uint32_t i = 0; i < TANG::GetMaxFramesInFlight(); i++)
	{
		TANG::DestroySemaphore(&postProcessingFinishedSemaphore[i]);
		TANG::DestroySemaphore(&coreRenderFinishedSemaphore[i]);
	}
}

void CalculateStartingMatrices(uint32_t windowWidth, uint32_t windowHeight)
{
	// Calculate the starting view direction and position of the camera
	glm::vec3 eye = { 0.0f, 0.0f, 1.0f };
	startingCameraPosition = { 0.0f, 5.0f, 15.0f };
	startingViewMatrix = glm::inverse(glm::lookAt(startingCameraPosition, startingCameraPosition + eye, { 0.0f, 1.0f, 0.0f }));

	// Calculate the starting projection matrix
	float aspectRatio = windowWidth / static_cast<float>(windowHeight);
	startingProjMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
	startingProjMatrix[1][1] *= -1; // NOTE - GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
}

void PreprocessSkyboxCubemap(TANG::UUID cubemap, TANG::UUID quad)
{
	TANG::LogInfo("Starting cubemap preprocessing...");

    // Convert the HDR texture into a cubemap and calculate IBL components (irradiance + prefilter map + BRDF LUT)
    TANG::PrimaryCommandBuffer cmdBuffer = TANG::AllocatePrimaryCommandBuffer(TANG::QUEUE_TYPE::GRAPHICS);
    cmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);

    cubemapPreprocessingPass.Draw(&cmdBuffer, TANG::GetAssetResources(cubemap), TANG::GetAssetResources(quad));

    cmdBuffer.EndRecording();

    TANG::QueueSubmitInfo info{};
    info.waitSemaphore = VK_NULL_HANDLE;
    info.signalSemaphore = VK_NULL_HANDLE;
    info.waitStages = 0;
    info.fence = cubemapPreprocessingPass.GetFence();
    TANG::QueueCommandBuffer(cmdBuffer, info);
    TANG::Draw();
    TANG::WaitForFence(info.fence);

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
	   // TANG::LogError("Failed to execute commands for cubemap preprocessing!");
	   // return;
    //}

    //// Wait for the GPU to finish preprocessing the cubemap
    //vkWaitForFences(GetLogicalDevice(), 1, &cubemapPreprocessingFence, VK_TRUE, UINT64_MAX);

    cubemapPreprocessingPass.UpdatePrefilterMapViewScope();
    cubemapPreprocessingPass.DestroyIntermediates();

    // Update skybox texture on the skybox pass
    skyboxPass.UpdateSkyboxCubemap(cubemapPreprocessingPass.GetSkyboxCubemap());

    TANG::LogInfo("Cubemap preprocessing done!");
}

TANG::SecondaryCommandBuffer DrawSkybox(TANG::UUID skyboxUUID)
{
	TANG::AssetResources* skyboxAsset = TANG::GetAssetResources(skyboxUUID);
	if (skyboxAsset == nullptr)
	{
		TANG::LogError("Skybox asset is not loaded! Failed to draw skybox");
        return {};
	}

	TANG::SecondaryCommandBuffer secondaryCmdBuffer = TANG::AllocateSecondaryCommandBuffer(TANG::QUEUE_TYPE::GRAPHICS);
	if (!secondaryCmdBuffer.IsAllocated())
	{
        return {};
	}

    uint32_t currentFrame = TANG::GetCurrentFrameIndex();
    TANG::Framebuffer* hdrFramebuffer = &hdrFramebuffers[currentFrame];

    skyboxPass.UpdateViewProjUniformBuffers(currentFrame, camera.GetViewMatrix(), camera.GetProjMatrix());
	skyboxPass.UpdateDescriptorSets(currentFrame);

	TANG::DrawData drawData{};
	drawData.asset = skyboxAsset;
	drawData.cmdBuffer = &secondaryCmdBuffer;
	drawData.framebuffer = hdrFramebuffer;
	drawData.renderPass = &hdrRenderPass;
	drawData.framebufferWidth = hdrFramebuffer->GetWidth();
    drawData.framebufferHeight = hdrFramebuffer->GetHeight();
	skyboxPass.Draw(currentFrame, drawData);

    return secondaryCmdBuffer;
}

TANG::SecondaryCommandBuffer DrawAsset(TANG::UUID assetUUID)
{
    TANG::AssetResources* resources = TANG::GetAssetResources(assetUUID);
    if (resources == nullptr)
    {
        TANG::LogError("Asset resources are not loaded for asset with UUID %ull", assetUUID);
        return {};
    }

	if (resources->shouldDraw)
	{
        uint32_t currentFrame = TANG::GetCurrentFrameIndex();
        TANG::Framebuffer* hdrFramebuffer = &hdrFramebuffers[currentFrame];

		// Update the transform for the asset
		pbrPass.UpdateTransformUniformBuffer(currentFrame, resources->transform);

		// TODO - This is super hard-coded, but it'll do for now
		std::array<const TANG::TextureResource*, 8> textures;
		textures[0] = &resources->material[static_cast<uint32_t>(TANG::Material::TEXTURE_TYPE::DIFFUSE)];
		textures[1] = &resources->material[static_cast<uint32_t>(TANG::Material::TEXTURE_TYPE::NORMAL)];
		textures[2] = &resources->material[static_cast<uint32_t>(TANG::Material::TEXTURE_TYPE::METALLIC)];
		textures[3] = &resources->material[static_cast<uint32_t>(TANG::Material::TEXTURE_TYPE::ROUGHNESS)];
		textures[4] = &resources->material[static_cast<uint32_t>(TANG::Material::TEXTURE_TYPE::LIGHTMAP)];
		textures[5] = cubemapPreprocessingPass.GetIrradianceMap();
		textures[6] = cubemapPreprocessingPass.GetPrefilterMap();
		textures[7] = cubemapPreprocessingPass.GetBRDFConvolutionMap();
		pbrPass.UpdateDescriptorSets(currentFrame, textures);

		TANG::SecondaryCommandBuffer secondaryCmdBuffer = TANG::AllocateSecondaryCommandBuffer(TANG::QUEUE_TYPE::GRAPHICS);
		if (!secondaryCmdBuffer.IsAllocated())
		{
            TANG::LogError("Failed to draw asset with UUID &ull", assetUUID);
            return {};
		}

		TANG::DrawData drawData{};
		drawData.asset = resources;
		drawData.cmdBuffer = &secondaryCmdBuffer;
		drawData.framebuffer = hdrFramebuffer;
		drawData.renderPass = &hdrRenderPass;
        drawData.framebufferWidth = hdrFramebuffer->GetWidth();
		drawData.framebufferHeight = hdrFramebuffer->GetHeight();
		pbrPass.Draw(currentFrame, drawData);

        return secondaryCmdBuffer;
	}

    return {};
}

void PerformLDRConversion(TANG::PrimaryCommandBuffer& cmdBuffer, TANG::UUID quadUUID)
{
	TANG::AssetResources* fullscreenQuadAsset = TANG::GetAssetResources(quadUUID);
	if (fullscreenQuadAsset == nullptr)
	{
		TANG::LogError("Fullscreen quad asset is not loaded! Failed to perform LDR conversion");
		return;
	}

    uint32_t currentFrame = TANG::GetCurrentFrameIndex();
    TANG::Framebuffer* hdrFramebuffer = &hdrFramebuffers[currentFrame];

	ldrPass.UpdateExposureUniformBuffer(currentFrame, 1.0f);
	ldrPass.UpdateDescriptorSets(currentFrame, bloomPass.GetOutputTexture());

	TANG::DrawData drawData{};
	drawData.asset = fullscreenQuadAsset;
	drawData.cmdBuffer = &cmdBuffer;
	drawData.framebuffer = hdrFramebuffer;
	drawData.renderPass = &hdrRenderPass;
	drawData.framebufferWidth = hdrFramebuffer->GetWidth();
    drawData.framebufferHeight = hdrFramebuffer->GetHeight();;
	ldrPass.Draw(currentFrame, drawData);

	// NOTE - color attachment is cleared at the beginning of the frame, so transitioning the layout to something
	//        else won't make a difference
}

static void QueueCommandBuffers(const TANG::PrimaryCommandBuffer& assetCmdBuffer, const TANG::PrimaryCommandBuffer& postProcessingCmdBuffer, const TANG::PrimaryCommandBuffer& ldrCmdBuffer)
{
    uint32_t currentFrame = TANG::GetCurrentFrameIndex();

    {
        TANG::QueueSubmitInfo info{};
        info.waitSemaphore = TANG::GetCurrentImageAvailableSemaphore();
        info.signalSemaphore = coreRenderFinishedSemaphore[currentFrame];
        info.waitStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        TANG::QueueCommandBuffer(assetCmdBuffer, info);
    }

    {
        TANG::QueueSubmitInfo info{};
        info.waitSemaphore = coreRenderFinishedSemaphore[currentFrame];
        info.signalSemaphore = postProcessingFinishedSemaphore[currentFrame];
        info.waitStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        TANG::QueueCommandBuffer(postProcessingCmdBuffer, info);
    }

    {
        TANG::QueueSubmitInfo info{};
        info.waitSemaphore = postProcessingFinishedSemaphore[currentFrame];
        info.signalSemaphore = TANG::GetCurrentRenderFinishedSemaphore();
        info.waitStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        info.fence = TANG::GetCurrentFrameFence();
        TANG::QueueCommandBuffer(ldrCmdBuffer, info);
    }
}

static void RecreateFramebuffers(uint32_t newWidth, uint32_t newHeight)
{
    DestroyFramebuffer();
    CreateFramebuffer(newWidth, newHeight);

    // Fix the projection after swap chain is resized
    camera.Update(0.0f);
}

static void Shutdown()
{
	// Clean up after TANG is shutdown so we know resources are not in-use
    camera.Shutdown();
	DestroyFramebuffer();
	DestroyPasses();
	DestroyRenderPasses();
    DestroySyncObjects();
    TANG::DestroyAllAssets();
}

int main(uint32_t argc, const char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

#ifdef TNG_WINDOWS
    system("cls");
#endif

    // Seed rand()
    srand(static_cast<unsigned int>(time(NULL)));

	// Initialize and set the camera attributes
    camera.Initialize({ 0.0f, 5.0f, 15.0f }, { 0.0f, 0.0f, 0.0f }); // Start the camera facing towards negative Z
	camera.SetSpeed(4.0f);
	camera.SetSensitivity(5.0f);


    TANG::Initialize();
    TANG::RegisterSwapChainRecreatedCallback(RecreateFramebuffers);
    TANG::RegisterRendererShutdownCallback(Shutdown);

	uint32_t windowWidth, windowHeight;
	TANG::GetWindowSize(windowWidth, windowHeight);
    CreateRenderPasses();
    CreatePasses(windowWidth, windowHeight);
    CreateFramebuffer(windowWidth, windowHeight);
    CalculateStartingMatrices(windowWidth, windowHeight);

    std::vector<MyAsset> assets;

	// Load core asset (fullscreen quad, cube for skybox, etc)
	TANG::UUID quadUUID = TANG::LoadAsset(TANG::CONFIG::FullscreenQuadMeshFilePath.c_str());
    UNUSED(quadUUID);
	TANG::UUID skyboxUUID = TANG::LoadAsset(TANG::CONFIG::SkyboxCubeMeshFilePath.c_str());

    // Load all the other assets
    for (auto& assetName : assetNames)
    {
        TANG::UUID id = TANG::LoadAsset(assetName.c_str());
        if (id != TANG::INVALID_UUID)
        {
            MyAsset asset(assetName, id);

			////float scale = 0.005f;
   //         float scale = 15.0f;
   //         //float scale = 1.0f;
			//asset.scale[0] = scale;
			//asset.scale[1] = scale;
			//asset.scale[2] = scale;

   //         TANG::UpdateAssetTransform(id, asset.pos, asset.rot, asset.scale);

            assets.push_back(asset);
        }
    }

	for (auto& asset : assets)
    {
          TANG::UUID id = asset.uuid;
          TANG::AssetResources* resources = TANG::GetAssetResources(id);
          resources->transform.scale = glm::vec3(15.0f);
          resources->shouldDraw = true; // Draw loaded assets by default
	}

    // Now that assets are loaded, let's preprocess the skybox texture
    PreprocessSkyboxCubemap(skyboxUUID, quadUUID);

    const float fpsUpdateCycle = 1.0f;
    float fpsUpdateTimer = 0.0f;
    float accumulatedDT = 0.0f;
    uint32_t fpsSampleCount = 0;

    float elapsedTime = 0.0f;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (!TANG::WindowShouldClose())
    {
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - startTime).count();
        elapsedTime += dt;
        startTime = std::chrono::high_resolution_clock::now();

        // Variables for FPS tracking
        accumulatedDT += dt;
        fpsSampleCount++;
        fpsUpdateTimer += dt;

        if (fpsUpdateTimer > fpsUpdateCycle)
        {
            float averageDT = accumulatedDT / static_cast<float>(fpsSampleCount);
            uint32_t frameFPS = static_cast<uint32_t>(1.0f / averageDT);
            TANG::SetWindowTitle("TANG - %u FPS", frameFPS);

            fpsUpdateTimer -= fpsUpdateCycle;
            accumulatedDT = 0.0f;
            fpsSampleCount = 0;
        }

		// Update camera only if window is focused
        if (TANG::WindowInFocus())
        {
		    camera.Update(dt);
        }
        
        TANG::Update(dt);

        TANG::BeginFrame();

        uint32_t currentFrame = TANG::GetCurrentFrameIndex();
        TANG::Framebuffer* framebuffer = &hdrFramebuffers[currentFrame];

        // RENDER ASSETS
        std::vector<VkCommandBuffer> secondaryCmdBuffers;
        secondaryCmdBuffers.reserve(assets.size());

        TANG::PrimaryCommandBuffer assetCmdBuffer = TANG::AllocatePrimaryCommandBuffer(TANG::QUEUE_TYPE::GRAPHICS);
		assetCmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
		assetCmdBuffer.CMD_BeginRenderPass(&hdrRenderPass, framebuffer, { framebuffer->GetWidth(), framebuffer->GetHeight() }, true, true);

        TANG::SecondaryCommandBuffer skyboxCmdBuffer = DrawSkybox(skyboxUUID);
        if (skyboxCmdBuffer.IsValid())
        {
            secondaryCmdBuffers.push_back(skyboxCmdBuffer.GetBuffer());
        }

        pbrPass.UpdateViewUniformBuffer(currentFrame, camera.GetViewMatrix());
        pbrPass.UpdateProjUniformBuffer(currentFrame, camera.GetProjMatrix());
        pbrPass.UpdateCameraUniformBuffer(currentFrame, camera.GetPosition());
        for (const auto& asset : assets)
        {
            TANG::SecondaryCommandBuffer assetSecCmdBuffer = DrawAsset(asset.uuid);
            if (assetSecCmdBuffer.IsValid())
            {
                secondaryCmdBuffers.push_back(assetSecCmdBuffer.GetBuffer());
            }
        }

        if (secondaryCmdBuffers.size() > 0)
        {
            assetCmdBuffer.CMD_ExecuteSecondaryCommands(secondaryCmdBuffers.data(), static_cast<uint32_t>(secondaryCmdBuffers.size()));
        }

		assetCmdBuffer.CMD_EndRenderPass(&hdrRenderPass, framebuffer);
		assetCmdBuffer.EndRecording();

        // POST-PROCESSING
		TANG::PrimaryCommandBuffer postProcessingCmdBuffer = TANG::AllocatePrimaryCommandBuffer(TANG::QUEUE_TYPE::COMPUTE);
		postProcessingCmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
		bloomPass.Draw(currentFrame, &postProcessingCmdBuffer, &hdrColorAttachments[currentFrame]);
		postProcessingCmdBuffer.EndRecording();

		// Submit the LDR conversion commands separately, since they use a different render pass
		TANG::Framebuffer* swapChainFramebuffer = TANG::GetCurrentSwapChainFramebuffer();
        if (swapChainFramebuffer == nullptr)
        {
            TANG::LogError("Failed to retrieve swap chain framebuffer. Cannot render to back buffer!");
            continue;
        }
		TANG::PrimaryCommandBuffer ldrCmdBuffer = TANG::AllocatePrimaryCommandBuffer(TANG::QUEUE_TYPE::GRAPHICS);
		ldrCmdBuffer.BeginRecording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
        ldrCmdBuffer.CMD_BeginRenderPass(&ldrRenderPass, swapChainFramebuffer, { swapChainFramebuffer->GetWidth(), swapChainFramebuffer->GetHeight() }, false, true);

		PerformLDRConversion(ldrCmdBuffer, quadUUID);

		ldrCmdBuffer.CMD_EndRenderPass(&ldrRenderPass, swapChainFramebuffer);
		ldrCmdBuffer.EndRecording();

        // QUEUE COMMAND BUFFERS
        QueueCommandBuffers(assetCmdBuffer, postProcessingCmdBuffer, ldrCmdBuffer);

        // Consumes the queued command buffers
        TANG::Draw();

        TANG::EndFrame();
    }

    TANG::Shutdown();


    return EXIT_SUCCESS;
}
