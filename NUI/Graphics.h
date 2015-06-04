#pragma once

#include "Base.h"

namespace nui {

struct Graphics
{
  NVGcontext *nvgContext = nullptr;
  int normalFontID = -1;
  int monospaceFontID = -1;

  struct IconAtlasInfo
  {
    int imageID = -1;
    Vec2 imageSize;
    Vec2 offset;
    Vec2 iconSize;
    Vec2 iconStride;
    int iconMargin = 0;
  };

  IconAtlasInfo iconAtlasInfo;

  class Style : public Object
  {
    public:
      enum Values
      {
        DefaultColor = 0x303438,
        DefaultSecondaryColor = 0x3060B0,
        DefaultTextColor = 0xFFFFFF,
        DefaultTextSize = 13,
        DefaultWindowButtonColor = 0x804020,
        DefaultTitleHeight = 22,
        DefaultMargin = 2,
        DefaultPadding = 4,
        ResizeEdgeSize = 6,
        ResizeCornerSize = 16,
        DefaultWindowWidth = 400,
        DefaultWindowHeight = 300,
        DefaultControlWidth = 90,
        DefaultControlHeight = 22,
        DefaultScrollBarSize = 14,
        CheckBoxSize = 18,
      };

      RGB color = DefaultColor;
      RGB secondaryColor = DefaultSecondaryColor;
      RGB textColor = DefaultTextColor;
      float textSize = static_cast<float>(DefaultTextSize);

      typedef nui::Ptr<Style> Ptr;

      Style() : Object() { }
      explicit Style(const RGB &c) : Object(), color(c), secondaryColor(c) { }
      Style(const RGB &c, const RGB &secondary) : Object(), color(c), secondaryColor(secondary) { }

    protected:
      virtual ~Style() { }
  };

  struct State
  {
    Graphics::Style::Ptr style;
    float alpha = 1.0f;
  };

  State state;

  enum class Bevel
  {
    Normal = 0,
    Window,
    Title,
    ScrollPanel,
    ButtonUp,
    ButtonDown,
    Menu,
    MenuItem,
    MenuBar,
    MenuBarItem,
  };

  enum class VAlign
  {
    Top = NVG_ALIGN_TOP,
    Middle = NVG_ALIGN_MIDDLE,
    BaseLine = NVG_ALIGN_BASELINE,
    Bottom = NVG_ALIGN_BOTTOM
  };

  enum class HAlign
  {
    Left = NVG_ALIGN_LEFT,
    Center = NVG_ALIGN_CENTER,
    Right = NVG_ALIGN_RIGHT
  };

  void drawShadow(int x, int y, int w, int h, int r, float alpha);

  float drawText(int x, int y, const char *text, bool shadow, HAlign halign = HAlign::Center, VAlign valign = VAlign::Middle, bool monospace = false);

  void drawIcon(int cx, int cy, int iconID);

  void drawBevel(int px, int py, int width, int height, unsigned state, Bevel type);
};

}
