workspace "III.VC.SA.SaveLoader"
   configurations { "Release", "Debug" }
   location "build"
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://github.com/ThirteenAG\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.SaveLoader\"" }
   
   files { "source/*.h" }
   files { "source/*.cpp", "source/*.c" }
   files { "source/*.rc" }

   files { "external/cpr/cpr/*.cpp" }
   files { "external/jsoncpp/src/lib_json/*.cpp" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   
   includedirs { "source/" }
   includedirs { "external/hooking" }
   includedirs { "external/injector/include" }
   includedirs { "external/inireader" }
   includedirs { "external/curl/builds/libcurl-vc14-x86-release-static-ipv6-sspi-winssl/include" }
   includedirs { "external/cpr/include" }
   includedirs { "external/jsoncpp/include" }
   
   libdirs { "external/curl/builds/libcurl-vc14-x86-release-static-ipv6-sspi-winssl/lib" }
   
   links { "wldap32.lib", "Ws2_32.lib" }
   links { "libcurl_a.lib" }
   defines { "CURL_STATICLIB" }
   
   prebuildcommands {
	"cd ../external/curl/winbuild/",
	"nmake /f Makefile.vc mode=static RTLIBCFG=static ENABLE_IDN=no VC=14"
   }

project "GTASNPTestApp"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".exe"
   excludes { "source/dllmain.cpp" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      flags { "Symbols" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  characterset ("MBCS")
	  
	  
project "III.VC.SA.SaveLoader"
   kind "SharedLib"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".asi"
   excludes { "source/GTASNPTestApp.cpp" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      flags { "Symbols" }
	  characterset ("MBCS")

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  characterset ("MBCS")
