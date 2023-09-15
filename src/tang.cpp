
#include "assert.h"
#include "asset_loader.h"
#include "renderer.h"
#include "tang.h"

namespace TANG
{
	static Renderer rendererHandle;

	void Initialize()
	{
		assert(false && "TODO - Implement");
	}

	void Shutdown()
	{
		assert(false && "TODO - Implement");
	}

	// STATE CALLS
	void LoadAsset(const char* name)
	{
		assert(false && "TODO - Implement");
		//LoaderUtils::Load(name);
		//rendererHandle.LoadAssetResources(name);
	}

	// UPDATE CALLS
	void DrawAsset(const char* name)
	{
		assert(false && "TODO - Implement");
	}
}