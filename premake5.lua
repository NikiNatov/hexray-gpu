workspace "hexray-gpu"
	architecture "x64"
	startproject "hexray-gpu"

	configurations 
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	outputdir = "%{cfg.buildcfg}"

project "hexray-gpu"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	systemversion "latest"
	staticruntime "on"
	characterset("ASCII")

	targetdir("%{wks.location}/bin/" .. outputdir)
	objdir("%{wks.location}/tmp/" .. outputdir)

	files
	{
		"%{wks.location}/src/**.cpp",
		"%{wks.location}/src/**.h",
	}

	includedirs
	{
		"%{wks.location}/src",
		"%{wks.location}/extern/spdlog/include",
		"%{wks.location}/extern/glm/glm",
		"%{wks.location}/extern/entt/include",
		"%{wks.location}/extern/pix/include",
		"%{wks.location}/extern/stb",
		"%{wks.location}/extern",
	}

	libdirs
	{
		"%{wks.location}/extern/pix/lib"
	}

	links
	{
		"d3d12",
		"dxgi",
		"dxguid",
		"WinPixEventRuntime"
	}

	postbuildcommands
	{
		"XCOPY %{wks.location}\\extern\\pix\\lib\\WinPixEventRuntime.dll \"%{cfg.targetdir}\"  /S /Y"
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

		defines
		{
			"HEXRAY_DEBUG",
			"_DEBUG"
		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

		defines
		{
			"HEXRAY_RELEASE",
			"NDEBUG"
		}