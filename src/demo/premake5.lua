project("demo")

generateProject( 
{
	type = "app",
	language = "C++",
})

links { "NUI", "SDL", "nanovg", "glew" }

filter { "system:windows" }
  links { "opengl32", "imm32", "winmm", "version" }