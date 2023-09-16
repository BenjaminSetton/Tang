
#include <string>

#include "asset_loader.h"
#include "renderer.h"
#include "utils/sanity_check.h"

static constexpr uint32_t NUM_ASSETS = 1;
static std::string assets[NUM_ASSETS] =
{
    "../src/data/assets/sample/suzanne.fbx"
};

int main() {

    TANG::Renderer rendererHandle;
    rendererHandle.Initialize();

    // Load in the models
    for (uint32_t i = 0; i < NUM_ASSETS; i++)
    {
        TANG::Asset* asset = TANG::LoaderUtils::Load(assets[i]);
        // If Load() returns nullptr, we know it didn't allocate memory on the heap, so no need to de-allocate anything here
        if (asset == nullptr) continue;

        rendererHandle.CreateAssetResources(asset);
    }

    // Update loop
    try {
        rendererHandle.Update();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Shutdown
    rendererHandle.DestroyAllAssetResources();
    TANG::LoaderUtils::UnloadAll();
    rendererHandle.Shutdown();

    return EXIT_SUCCESS;
}
