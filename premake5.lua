dofile("etc/premake5/Utils.lua")

solution "NUI"
  configurations { "Debug", "Release" }
  platforms { "32-bit", "64-bit" }
  location(path.join(".build", _ACTION))
  debugdir "data"
  
filter { "platforms:32*" }
  architecture "x86"

filter { "platforms:64*" }
  architecture "x86_64"
  
filter { "configurations:*Debug" }
  defines { "_DEBUG", "DEBUG" }
  flags { "Symbols" }
  targetsuffix "_debug"
  
filter { "configurations:*Release" }
  defines { "_NDEBUG", "NDEBUG" }
  optimize "On"

filter { "system:windows", "platforms:64*" }
  defines { "_WIN64", "WIN64" }
  
filter { "system:windows" }
  defines { "_WIN32", "WIN32", "_CRT_SECURE_NO_WARNINGS", "_WIN32_WINNT=0x0601", "WINVER=0x0601", "NTDDI_VERSION=0x06010000" }
  flags { "NoMinimalRebuild", "MultiProcessorCompile" }
  buildoptions { '/wd"4503"' }
  
  if _ACTION == "vs2013" then
    defines { "_MSC_VER=1800" }
  elseif _ACTION == "vs2015" then
    defines { "_MSC_VER=1900" }

    filter { "system:windows", "configurations:DLL Debug" }
      links { "ucrtd.lib", "vcruntimed.lib", "msvcrtd.lib" }

    filter { "system:windows", "configurations:DLL Release" }
      links { "ucrt.lib", "vcruntime.lib", "msvcrt.lib" }
      
    filter { }
  end
  
filter { }
  includedirs { "src", "srcSDL" }
  targetdir "bin/%{cfg.buildcfg}"
  
group "src"
  include "src"
  
group "srcSDL"
  include "srcSDL"
