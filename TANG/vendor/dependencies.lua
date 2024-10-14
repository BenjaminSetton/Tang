
VULKAN_SDK = os.getenv("VULKAN_SDK")

TANG_IncludeDirs                     = {}
TANG_IncludeDirs["vulkan"]           = "%{wks.location}/TANG/vendor/vulkan/include"
TANG_IncludeDirs["glfw"]             = "%{wks.location}/TANG/vendor/glfw/include"
TANG_IncludeDirs["nlohmann_json"]    = "%{wks.location}/TANG/vendor/nlohmann_json/include"

TANG_Libraries                       = {}
TANG_Libraries["vulkan"]             = "%{VULKAN_SDK}/Lib/vulkan-1.lib"
TANG_Libraries["glfw_debug"]         = "%{wks.location}/TANG/vendor/glfw/bin/debug/glfw3.lib"
TANG_Libraries["glfw_release"]       = "%{wks.location}/TANG/vendor/glfw/bin/release/glfw3.lib"
