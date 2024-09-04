
#include <array>

#include "base_pass.h"
#include "../utils/sanity_check.h"
#include "../utils/logger.h"

static const std::array<uint8_t, sizeof(TANG::DrawData)> ZERO_BLOCK = {};

namespace TANG
{
	void BasePass::Create()
	{
		if (wasCreated)
		{
			LogWarning("Attempting to create base pass when it was already created!");
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

		wasCreated = true;
	}

	bool BasePass::IsDrawDataValid(const DrawData& data) const
	{
#ifdef _DEBUG
		// Ensure that ZERO_BLOCK actually was zero-initialized to some extent
		// I know this looks stupid, but better safe than sorry
		uint8_t testBlock[5] = { 0, 0, 0, 0, 0 };
		TNG_ASSERT((memcmp(testBlock, ZERO_BLOCK.data(), 5)) == 0);
#endif

		// memcmp returns 0 if both blocks are equal
		return (memcmp(&data, ZERO_BLOCK.data(), sizeof(TANG::DrawData))) != 0;
	}

	void BasePass::CreateFramebuffers()		{                         }
	void BasePass::CreatePipelines()		{                         }
	void BasePass::CreateRenderPasses()		{                         }
	void BasePass::CreateSetLayoutCaches()	{                         }
	void BasePass::CreateDescriptorSets()	{                         }
	void BasePass::CreateUniformBuffers()	{                         }
	void BasePass::CreateSyncObjects()		{ fence = VK_NULL_HANDLE; }

	void BasePass::ResetBaseMembers()
	{
		fence = VK_NULL_HANDLE;
		wasCreated = false;
	}

	VkFence BasePass::GetFence()
	{
		return fence;
	}
}