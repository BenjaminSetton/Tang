
#include "assert.h"
#include "asset_loader.h"
#include "renderer.h"

static constexpr uint32_t NUM_ASSETS = 1;
static const char* assets[NUM_ASSETS] =
{

};

int main() {

    TANG::Renderer rendererHandle;

    // Load in the models
    for (uint32_t i = 0; i < NUM_ASSETS; i++)
    {
        TANG::LoaderUtils::Load(assets[i]);
        assert(false && "TODO - Implement");
        //rendererHandle.LoadAssetResources(assets[i]);
    }

    try {
        rendererHandle.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
