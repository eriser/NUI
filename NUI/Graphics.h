#pragma once

#include "Base.h"

namespace nui {

struct Graphics
{
  NVGcontext *nvgContext = nullptr;
  int normalFontID = -1;
  int monospaceFontID = -1;
  double cursorBlinker = 0.0;

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
        DefaultTextSize = 14,
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
        DefaultScrollBarSize = 16,
        CheckBoxSize = 18,
      };

      RGB color = DefaultColor;
      RGB secondaryColor = DefaultSecondaryColor;
      RGB textColor = DefaultTextColor;
      int textSize = DefaultTextSize;

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
    TextBox,
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

  void pushState();

  void popState();

  void intersectScissor(int x, int y, int w, int h);

  void drawShadow(int x, int y, int w, int h, int r, float alpha);

  float drawText(int x, int y, const char *text, const char *end, bool shadow, HAlign halign = HAlign::Center, VAlign valign = VAlign::Middle, bool monospace = false);

  float drawText(int x, int y, const char *text, bool shadow, HAlign halign = HAlign::Center, VAlign valign = VAlign::Middle, bool monospace = false)
  {
    return drawText(x, y, text, nullptr, shadow, halign, valign, monospace);
  }

  void drawTextCursor(int x, int y);

  void drawIcon(int cx, int cy, int iconID);

  void drawBevel(int px, int py, int width, int height, unsigned state, Bevel type);
};

}
