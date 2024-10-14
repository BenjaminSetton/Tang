
include "vendor/dependencies.lua"

outputDir = "%{cfg.system}/%{cfg.buildcfg}/%{cfg.architecture}"

project "TANG"
	location "out"
	language "C++"
	kind "StaticLib"
	
	targetdir ("out/bin/" .. outputDir)
	objdir ("out/obj/" .. outputDir)
	
	files
	{
		"src/**.h",
		"src/**.cpp"
	}
	
	includedirs
	{
		"%{TANG_IncludeDirs.vulkan}",
		"%{TANG_IncludeDirs.glfw}",
		"%{TANG_IncludeDirs.nlohmann_json}"
	}
	
	links
	{
		"%{TANG_Libraries.vulkan}"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"
		warnings "High"
		defines "TNG_WINDOWS"
		
		-- Copy the header files into the include directory
		prebuildcommands
		{
			("CALL \"%{wks.location}/TANG/tools/copy_header_files.bat\"")
		}
	
	filter "configurations:Debug"
		defines "TNG_DEBUG"
		symbols "On"
		
		links
		{
			"%{TANG_Libraries.glfw_debug}",
		}
	
	filter "configurations:Release"
		defines "TNG_NDEBUG"
		optimize "On"
		
		links
		{
			"%{TANG_Libraries.glfw_release}",
		}