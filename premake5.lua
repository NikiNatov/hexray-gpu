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
		"%{wks.location}/extern/stb/**.h",
		"%{wks.location}/extern/stb/**.cpp",
		"%{wks.location}/extern/DirectXTex/**.h",
		"%{wks.location}/extern/DirectXTex/**.cpp",
		"%{wks.location}/extern/DirectXShaderCompiler/**.h",
		"%{wks.location}/extern/openexr/**.h",
	}

	includedirs
	{
		"%{wks.location}/src",
		"%{wks.location}/extern/assimp/include",
		"%{wks.location}/extern/spdlog/include",
		"%{wks.location}/extern/glm/glm",
		"%{wks.location}/extern/entt/include",
		"%{wks.location}/extern/pix/include",
		"%{wks.location}/extern/stb",
		"%{wks.location}/extern/DirectXTex",
		"%{wks.location}/extern/yaml-cpp/include",
		"%{wks.location}/extern/DirectXShaderCompiler/include",
		"%{wks.location}/extern/openexr/include/OpenEXR",
	}

	libdirs
	{
		"%{wks.location}/extern/pix/lib",
		"%{wks.location}/extern/assimp/lib",
		"%{wks.location}/extern/yaml-cpp/lib",
		"%{wks.location}/extern/openexr/lib",
	}

	links
	{
		"d3d12",
		"dxgi",
		"dxguid",
		"WinPixEventRuntime",
	}

	postbuildcommands
	{
		"XCOPY %{wks.location}\\extern\\pix\\lib\\WinPixEventRuntime.dll \"%{cfg.targetdir}\"  /S /Y",
		"XCOPY %{wks.location}\\extern\\DirectXShaderCompiler\\lib\\dxcompiler.dll \"%{cfg.targetdir}\"  /S /Y",
		"XCOPY %{wks.location}\\extern\\DirectXShaderCompiler\\lib\\dxil.dll \"%{cfg.targetdir}\"  /S /Y",
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

		defines
		{
			"HEXRAY_DEBUG",
			"_DEBUG"
		}

		links
		{
			"assimp-vc143-mtd",
			"yaml_d",
		}

		postbuildcommands
		{
			"XCOPY %{wks.location}\\extern\\assimp\\lib\\assimp-vc143-mtd.dll \"%{cfg.targetdir}\"  /S /Y",
			"XCOPY %{wks.location}\\extern\\openexr\\lib\\**-2_5_d.dll \"%{cfg.targetdir}\"  /S /Y",
			"XCOPY %{wks.location}\\extern\\zlib\\lib\\zlibd1.dll \"%{cfg.targetdir}\"  /S /Y",
		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

		defines
		{
			"HEXRAY_RELEASE",
			"NDEBUG"
		}

		links
		{
			"assimp-vc143-mt",
			"yaml_r",
		}

		postbuildcommands
		{
			"XCOPY %{wks.location}\\extern\\assimp\\lib\\assimp-vc143-mt.dll \"%{cfg.targetdir}\"  /S /Y",
			"XCOPY %{wks.location}\\extern\\openexr\\lib\\*-2_5.dll \"%{cfg.targetdir}\"  /S /Y",
			"XCOPY %{wks.location}\\extern\\zlib\\lib\\zlib1.dll \"%{cfg.targetdir}\"  /S /Y",
		}