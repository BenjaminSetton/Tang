workspace "TANG"

	architecture "x64"
	startproject "SceneDemo"
	
	configurations
	{
		"Debug",
		"Release"
	}

-- Projects
include "TANG/premake5.lua"
include "SceneDemo/premake5.lua"