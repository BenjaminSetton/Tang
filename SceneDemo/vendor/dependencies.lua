
SceneDemo_IncludeDirs                     = {}
SceneDemo_IncludeDirs["TANG"]             = "%{wks.location}/TANG/src"
SceneDemo_IncludeDirs["glm"]              = "%{wks.location}/SceneDemo/vendor/glm"
SceneDemo_IncludeDirs["assimp"]           = "%{wks.location}/SceneDemo/vendor/assimp/include"
SceneDemo_IncludeDirs["stb_image"]        = "%{wks.location}/SceneDemo/vendor/stb_image"

SceneDemo_Libraries                       = {}
SceneDemo_Libraries["assimp_debug"]       = "%{wks.location}/SceneDemo/vendor/assimp/bin/debug/assimp-vc143-mtd.lib"
SceneDemo_Libraries["assimp_release"]     = "%{wks.location}/SceneDemo/vendor/assimp/bin/release/assimp-vc143-mt.lib"
SceneDemo_Libraries["assimp_debug_dll"]   = "%{wks.location}/SceneDemo/vendor/assimp/bin/debug/assimp-vc143-mtd.dll"
SceneDemo_Libraries["assimp_release_dll"] = "%{wks.location}/SceneDemo/vendor/assimp/bin/release/assimp-vc143-mt.dll"
SceneDemo_Libraries["TANG_debug"]         = "%{wks.location}/TANG/out/bin/debug/x86_64/TANG.lib"
SceneDemo_Libraries["TANG_release"]       = "%{wks.location}/TANG/out/bin/release/x86_64/TANG.lib"
