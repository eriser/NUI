project "NUI"

generateProject( 
{
  language = "C++",
  type = "static"
})

links { "SDL", "nanovg" }
