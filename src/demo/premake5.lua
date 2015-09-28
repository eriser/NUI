project("demo")

generateProject( 
{
	type = "app",
	language = "C++",
})

links { "NUI", "SDL", "nanovg" }
