/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   TANG renderer
// 
//   Benjamin Setton
//   2024
// 
//   TANG is a simple Vulkan rendering library meant to kick-start the rendering process for your own projects! 
//   The API is written to make it fast and easy to draw something on the screen.
// 
//   The functions labeled under [CORE] calls must be called for the rendering loop to be setup correctly. More specifically,
//   the two mandatory calls are the Initialize() and Shutdown() calls, the Update() and Draw() calls can technically
//   be excluded and your application will still compile, link, and run without issue (but nothing will be drawn
//   on screen).
// 
//   The rest of the API calls are structured in two main components: [STATE] calls and [UPDATE] calls.
//   
//   STATE CALLS  - Meant to be called once to set or load a state for a non-trivial period
//                  of time. Examples include loading assets or setting the preferred
//                  renderer state
//   
//   UPDATE CALLS - Meant to be called on a per-frame basis. The effect of these functions will only
//                  last a single frame, so as soon as they're not called anymore the effect will not
//                  be visible on the screen. Examples include drawing assets or simple polygons and
//                  setting the camera transform.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils/uuid.h"    // TANG::UUID
#include "input_manager.h" // TANG::KeyState

// TEMP TEMP TEMP
#include "callback_types.h"
#include "config.h"
#include "queue_types.h"
#include "descriptors/descriptor_set.h"
#include "cmd_buffer/primary_command_buffer.h"
#include "cmd_buffer/secondary_command_buffer.h"
// TEMP TEMP TEMP

namespace TANG
{
	///////////////////////////////////////////////////////////
	//
	//		CORE
	// 
	///////////////////////////////////////////////////////////

	// Initializes the TANG renderer and sets up internal objects. The windowTitle parameter
	// is optional. In the case it's left to nullptr, a default window title will be used instead.
	// Note that the window title can be changed later on using the SetWindowTitle() function.
	// 
	// NOTE - This must be the FIRST API function call.
	void Initialize(const char* windowTitle = nullptr);

	// Core API update loop
	void Update(float deltaTime);

	////////////////////////////////////////////////////////////////////////
	// TEMP TEMP TEMP
	DescriptorSet AllocateDescriptorSet(const DescriptorSetLayout& setLayout);
	PrimaryCommandBuffer AllocatePrimaryCommandBuffer(QUEUE_TYPE type);
	SecondaryCommandBuffer AllocateSecondaryCommandBuffer(QUEUE_TYPE type);

	bool CreateSemaphore(VkSemaphore* semaphore, const VkSemaphoreCreateInfo& info);
	void DestroySemaphore(VkSemaphore* semaphore);

	bool CreateFence(VkFence* fence, const VkFenceCreateInfo& info);
	void DestroyFence(VkFence* fence);

	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);
	void QueueCommandBuffer(const PrimaryCommandBuffer& cmdBuffer, const QueueSubmitInfo& info);
	void Submit();

	void WaitForFence(VkFence fence, uint64_t timeout = std::numeric_limits<uint64_t>::max());

	uint32_t GetCurrentFrameIndex();
	constexpr uint32_t GetMaxFramesInFlight() { return CONFIG::MaxFramesInFlight; }

	VkSemaphore GetCurrentImageAvailableSemaphore();
	VkSemaphore GetCurrentRenderFinishedSemaphore();
	VkFence GetCurrentFrameFence();

	Framebuffer* GetCurrentSwapChainFramebuffer();

	void RegisterSwapChainRecreatedCallback(SwapChainRecreatedCallback callback);
	void RegisterRendererShutdownCallback(RendererShutdownCallback callback);

	// TEMP TEMP TEMP
	////////////////////////////////////////////////////////////////////////

	void BeginFrame();

	// Core API draw loop. Simply calls the renderer system Draw() call
	void Draw();

	void EndFrame();

	// Shuts down the TANG renderer and cleans up internal objects
	// NOTE - This must be the LAST API function call. All other API calls
	//        after this are invalid
	void Shutdown();

	///////////////////////////////////////////////////////////
	//
	//		STATE
	// 
	///////////////////////////////////////////////////////////

	// Returns true if the window should close. This can happen for many reasons, but usually
	// this is because the user clicked the close (X) button on the window
	bool WindowShouldClose();

	bool WindowInFocus();

	// Sets the title of the window using a format buffer. This may only be called after TANG::Initialize()
	void SetWindowTitle(const char* format, ...);

	// Returns the size of the main window
	void GetWindowSize(uint32_t& outWidth, uint32_t& outHeight);

	// Loads an asset given the filepath to the asset file on disk. If the asset has not been
	// imported before, this function will import any of the supported asset types: FBX and OBJ. 
	// Upon importing the asset, the Load() call will serialize a TASSET file corresponding to
	// the loaded asset, and all subsequent attempts to load the same asset by name will instead
	// load the TASSET file directly
	UUID LoadAsset(const char* filepath);
	void DestroyAsset(UUID uuid);
	void DestroyAllAssets(); // Convenient for application shutdown

	AssetResources* GetAssetResources(UUID uuid);

	///////////////////////////////////////////////////////////
	//
	//		UPDATE
	// 
	///////////////////////////////////////////////////////////

	// Renders an asset given it's UUID for this particular frame. This function will not do anything on the following
	// cases:
	// 1. The UUID points to an asset internally that does not exist
	// 2. The UUID is invalid (refer to INVALID_UUID inside uuid.h)
	//void ShowAsset(UUID uuid);

	// Update the transform of the asset represented by the provided UUID. 
	// NOTE - The position, rotation and scale parameters MUST be vectors with exactly three components
	//void UpdateAssetTransform(UUID uuid, float* position, float* rotation, float* scale);

	// Update the position of the asset represented by the provided UUID.
	// NOTE - The position parameter MUST be a vector with exactly three components
	//void UpdateAssetPosition(UUID uuid, float* position);

	// Update the rotation of the asset represented by the provided UUID.
	// If the given rotation is in degrees it must be specified using the "isDegrees" parameter,
	// a value of false is interpreted as a rotation in radians instead.
	// NOTE - The rotation parameter MUST be a vector with exactly three components
	//void UpdateAssetRotation(UUID uuid, float* rotation, bool isDegrees);

	// Update the scale of the asset represented by the provided UUID.
	// NOTE - The scale parameter MUST be a vector with exactly three components
	//void UpdateAssetScale(UUID uuid, float* scale);

	// Returns whether the provided key is pressed. Note that this function will return true as long as the key is held down
	bool IsKeyPressed(int key);

	// Returns whether the provided key is released. Similar to the function above, it will return true as long as the
	// key is NOT being pressed
	bool IsKeyReleased(int key);

	// Returns the current state of the provided key. This can be either PRESSED, HELD (TODO) or RELEASED.
	InputState GetKeyState(int key);

}