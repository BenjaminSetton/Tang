
VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDirs                     = {}
IncludeDirs["assimp"]           = "%{wks.location}/TANG/vendor/assimp/include"
IncludeDirs["glfw"]             = "%{wks.location}/TANG/vendor/glfw/include"
IncludeDirs["glm"]              = "%{wks.location}/TANG/vendor/glm"
IncludeDirs["stb_image"]        = "%{wks.location}/TANG/vendor/stb_image"
IncludeDirs["vulkan"]           = "%{wks.location}/TANG/vendor/vulkan/include"
IncludeDirs["nlohmann_json"]    = "%{wks.location}/TANG/vendor/nlohmann_json/include"

Libraries = {}
Libraries["vulkan"]             = "%{VULKAN_SDK}/Lib/vulkan-1.lib"

Libraries["assimp_debug"]       = "%{wks.location}/TANG/vendor/assimp/bin/debug/assimp-vc143-mtd.lib"
Libraries["assimp_debug_dll"]   = "%{wks.location}/TANG/vendor/assimp/bin/debug/assimp-vc143-mtd.dll"
Libraries["glfw_debug"]         = "%{wks.location}/TANG/vendor/glfw/bin/debug/glfw3.lib"

Libraries["assimp_release"]     = "%{wks.location}/TANG/vendor/assimp/bin/release/assimp-vc143-mt.lib"
Libraries["assimp_release_dll"] = "%{wks.location}/TANG/vendor/assimp/bin/release/assimp-vc143-mt.dll"
Libraries["glfw_release"]       = "%{wks.location}/TANG/vendor/glfw/bin/release/glfw3.lib"
