
VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDirs = {}
IncludeDirs["assimp"] 			= "%{wks.location}/vendor/assimp/include"
IncludeDirs["glfw"] 			= "%{wks.location}/vendor/glfw/include"
IncludeDirs["glm"] 				= "%{wks.location}/vendor/glm"
IncludeDirs["stb_image"] 		= "%{wks.location}/vendor/stb_image"
IncludeDirs["vulkan"] 			= "%{wks.location}/vendor/vulkan/include"

Libraries = {}
Libraries["vulkan"] 			= "%{VULKAN_SDK}/Lib/vulkan-1.lib"

Libraries["assimp_debug"] 		= "%{wks.location}/vendor/assimp/bin/debug/assimp-vc143-mtd.lib"
Libraries["assimp_debug_dll"] 	= "%{wks.location}/vendor/assimp/bin/debug/assimp-vc143-mtd.dll"
Libraries["glfw_debug"] 		= "%{wks.location}/vendor/glfw/bin/debug/glfw3.lib"

Libraries["assimp_release"] 	= "%{wks.location}/vendor/assimp/bin/release/assimp-vc143-mt.lib"
Libraries["assimp_release_dll"] = "%{wks.location}/vendor/assimp/bin/release/assimp-vc143-mt.dll"
Libraries["glfw_release"] 		= "%{wks.location}/vendor/glfw/bin/release/glfw3.lib"
