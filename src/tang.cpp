
#include "asset_loader.h"
#include "renderer.h"
#include "utils/sanity_check.h"
#include "tang.h"

namespace TANG
{
	static Renderer rendererHandle;

	void Initialize()
	{
		TNG_ASSERT_MSG(false, "TODO - Implement");
	}

	void Shutdown()
	{
		TNG_ASSERT_MSG(false, "TODO - Implement");
	}

	// STATE CALLS
	void LoadAsset(const char* name)
	{
		TNG_ASSERT_MSG(false, "TODO - Implement");
		Asset* asset = LoaderUtils::Load(name);
		if (asset == nullptr) return;

		rendererHandle.CreateAssetResources(asset);
	}

	// UPDATE CALLS
	void DrawAsset(const char* name)
	{
		TNG_ASSERT_MSG(false, "TODO - Implement");
	}
}