#include "Graphics.h"
#include "Control.h"

#ifdef _MSC_VER
#pragma warning (disable: 4244)
#endif

namespace nui {

#define N nvgContext
#define I iconAtlasInfo

//---------------------------------------------------------------------------------------------------------------------
void Graphics::pushState()
{
  nvgSave(N);
}

//---------------------------------------------------------------------------------------------------------------------
void Graphics::popState()
{
  nvgRestore(N);
}

//---------------------------------------------------------------------------------------------------------------------
void Graphics::intersectScissor(int x, int y, int w, int h)
{
  nvgIntersectScissor(N, x, y, w, h);
}

//---------------------------------------------------------------------------------------------------------------------
void Graphics::drawShadow(int x, int y, int w, int h, int r, float alpha)
{
  x -= r / 2;
  y -= r / 2;
  w += r;
  h += r;
  
  nvgBeginPath(N);
  nvgMoveTo(N, x, y - r);
  nvgLineTo(N, x + w, y - r);
  nvgArcTo(N, x + w + r, y - r, x + w + r, y, r);
  nvgLineTo(N, x + w + r, y + h);
  nvgArcTo(N, x + w + r, y + h + r, x + w, y + h + r, r);
  nvgLineTo(N, x, y + h + r);
  nvgArcTo(N, x - r, y + h + r, x - r, y + h, r);
  nvgLineTo(N, x - r, y);
  nvgArcTo(N, x - r, y - r, x, y - r, r);
  nvgClosePath(N);

  nvgFillPaint(N, nvgBoxGradient(N, x, y, w, h, r, r * 2, nvgRGBAf(0, 0, 0, alpha), nvgRGBAf(0, 0, 0, 0)));
  nvgFill(N);
}

//---------------------------------------------------------------------------------------------------------------------
float Graphics::drawText(int x, int y, const char *text, const char *end, bool shadow, HAlign halign, VAlign valign, bool monospace)
{
  nvgFontSize(N, state.style->textSize);
  nvgTextAlign(N, static_cast<unsigned>(halign) | static_cast<unsigned>(valign));
  nvgFontFaceId(N, monospace ? monospaceFontID : normalFontID);

  if (shadow)
  {
    nvgFillColor(N, nvgRGBf(0, 0, 0));
    nvgText(N, x + 1, y + 1, text, end);
  }

  nvgFillColor(N, state.style->textColor.nvg());
  return nvgText(N, x, y, text, end) - x;
}

//---------------------------------------------------------------------------------------------------------------------
void Graphics::drawTextCursor(int x, int y)
{
  float a = 1.0f - clamp(powf(static_cast<float>(cursorBlinker), 5.0f), 0.0f, 1.0f);

  nvgBeginPath(N);
  nvgStrokeColor(N, nvgRGBAf(1.0f, 1.0f, 1.0f, a));
  nvgStrokeWidth(N, 1.0f);
  nvgMoveTo(N, x + 0.5f, y - state.style->textSize / 2);
  nvgLineTo(N, x + 0.5f, y + state.style->textSize / 2);
  nvgStroke(N);
}

//---------------------------------------------------------------------------------------------------------------------
void Graphics::drawIcon(int cx, int cy, int iconID)
{
  if (iconID < 0) return;

  float w = static_cast<float>(I.iconSize.x);
  float h = static_cast<float>(I.iconSize.y);
  float x = static_cast<float>(cx - static_cast<int>(w * 0.5f));
  float y = static_cast<float>(cy - static_cast<int>(h * 0.5f));

  int ix = iconID & 0xFF;
  int iy = (iconID >> 8) & 0xFF;

  nvgBeginPath(N);
  nvgFillPaint(N, nvgImagePattern(N
    , x - static_cast<float>(I.offset.x - I.iconSize.x / 2 + ix * I.iconStride.x), y - static_cast<float>(I.offset.y - I.iconSize.y / 2 + iy * I.iconStride.y)
    , static_cast<float>(I.imageSize.x), static_cast<float>(I.imageSize.y), 0.0f, I.imageID, 1.0f));

  nvgRect(N, x, y, w, h);
  nvgFill(N);
}

//---------------------------------------------------------------------------------------------------------------------
void Graphics::drawBevel(int px, int py, int width, int height, unsigned controlState, Bevel type)
{
  if (width <= 0 || height <= 0)
    return;

  float x = static_cast<float>(px)+0.5f;
  float y = static_cast<float>(py)+0.5f;
  float w = static_cast<float>(width - 1);
  float h = static_cast<float>(height - 1);

  switch (type)
  {
    case Bevel::Window:
    {
      nvgBeginPath(N);
      nvgStrokeWidth(N, 1.0f);
      nvgStrokeColor(N, state.style->color.nvg(0.1f));
      nvgFillColor(N, state.style->color.nvg());
      nvgRoundedRect(N, x, y, w, h, 3.0f);
      nvgFill(N);
      nvgStroke(N);
    }
    break;

    case Bevel::Title:
    {
      nvgBeginPath(N);
      if ((controlState & nui::State::Focused) || (controlState & nui::State::DeepFocused))
      {
        nvgFillPaint(N, nvgLinearGradient(N, 0, 0, 0, h - 2.0f, state.style->secondaryColor.nvg(), state.style->secondaryColor.nvg(0.75f)));
        nvgStrokeWidth(N, 1.0f);
        nvgStrokeColor(N, state.style->color.nvg(0.1f));
        nvgRoundedRect(N, x, y, w, h, 3.0f);
        nvgStroke(N);
      }
      else
      {
        nvgFillPaint(N, nvgLinearGradient(N, 0, 0, 0, h - 2.0f, state.style->color.nvg(2.0f), state.style->color.nvg()));
        nvgRoundedRect(N, x, y, w, h, 3.0f);
      }

      nvgFill(N);
    }
    break;

    case Bevel::ScrollPanel:
    {
      nvgBeginPath(N);
      nvgStrokeWidth(N, 1.0f);
      nvgStrokeColor(N, nvgRGB(48, 48, 48));
      nvgFillColor(N, nvgRGB(32, 32, 32));
      nvgRoundedRect(N, x, y, w, h, 3.0f);
      nvgFill(N);
      nvgStroke(N);
    }
    break;

    case Bevel::ButtonUp:
    {
      nvgBeginPath(N);
      nvgStrokeWidth(N, 1.0f);
      nvgStrokeColor(N, state.style->color.nvg(0.25f));

      if (controlState & nui::State::Hot)
        nvgFillPaint(N, nvgLinearGradient(N, 0, 0, 0, h - 2.0f, state.style->color.nvg(), state.style->color.nvg(1.15f)));
      else
        nvgFillPaint(N, nvgLinearGradient(N, 0, 0, 0, h - 2.0f, state.style->color.nvg(), state.style->color.nvg(0.85f)));

      nvgRoundedRect(N, x, y, w, h, 3.0f);
      nvgFill(N);
      nvgStroke(N);

      nvgBeginPath(N);
      nvgStrokeColor(N, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.125f));
      nvgMoveTo(N, x + 1, y + h - 3);
      nvgLineTo(N, x + 1, y + 3);
      nvgArcTo(N, x + 1, y + 1, x + 3, y + 1, 2);
      nvgLineTo(N, x + w - 2, y + 1);
      nvgStroke(N);

      nvgBeginPath(N);
      nvgStrokeColor(N, nvgRGBAf(0, 0, 0, 0.3f));
      nvgMoveTo(N, x + 2, y + h - 1);
      nvgLineTo(N, x + w - 3, y + h - 1);
      nvgArcTo(N, x + w - 1, y + h - 1, x + w - 1, y + h - 3, 2);
      nvgLineTo(N, x + w - 1, y + 2);
      nvgStroke(N);

      nvgBeginPath(N);
      nvgStrokeColor(N, nvgRGBAf(0, 0, 0, 0.15f));
      nvgRect(N, x + 2, y + 2, w - 4, h - 4);
      nvgStroke(N);

      if (controlState & nui::State::Focused)
      {
        nvgBeginPath(N);
        nvgStrokeWidth(N, 1.0f);
        nvgStrokeColor(N, state.style->secondaryColor.nvg());
        nvgRoundedRect(N, x, y, w, h, 3.0f);
        nvgStroke(N);
      }
    }
    break;

    case Bevel::ButtonDown:
    {
      nvgBeginPath(N);
      nvgStrokeWidth(N, 1.0f);
      nvgStrokeColor(N, state.style->color.nvg(0.25f));
      nvgFillPaint(N, nvgLinearGradient(N, 0, 0, 0, h - 2.0f, state.style->color.nvg(0.65f), state.style->color.nvg(0.75f)));
      nvgRoundedRect(N, x, y, w, h, 3.0f);
      nvgFill(N);
      nvgStroke(N);

      nvgBeginPath(N);
      nvgStrokeColor(N, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.125f));
      nvgMoveTo(N, x + 1, y + h - 1);
      nvgLineTo(N, x + 1, y + 3);
      nvgArcTo(N, x + 1, y + 1, x + 3, y + 1, 2);
      nvgLineTo(N, x + w - 1, y + 1);
      nvgStroke(N);

      if (controlState & nui::State::Focused)
      {
        nvgBeginPath(N);
        nvgStrokeWidth(N, 1.0f);
        nvgStrokeColor(N, state.style->secondaryColor.nvg());
        nvgRoundedRect(N, x, y, w, h, 3.0f);
        nvgStroke(N);
      }
    }
    break;

    case Bevel::Menu:
    {
      nvgBeginPath(N);
      nvgStrokeWidth(N, 1.0f);
      nvgStrokeColor(N, state.style->color.nvg(1.25f));
      nvgFillColor(N, state.style->color.nvgA(0.75f, 0.1f));
      nvgRect(N, x, y, w, h);
      nvgFill(N);
      nvgStroke(N);
    }
    break;

    case Bevel::MenuBar:
    {
      nvgBeginPath(N);
      nvgFillColor(N, state.style->color.nvg());
      nvgRect(N, x, y, w, h);
      nvgFill(N);
    }
    break;

    case Bevel::MenuItem:
    {
      if (controlState & (nui::State::Hot | nui::State::Selected))
      {
        bool selected = (controlState & nui::State::Selected) != 0;

        nvgBeginPath(N);
        nvgStrokeWidth(N, 1.0f);
        nvgStrokeColor(N, state.style->color.nvgA(0.5f, 0.25f));
        nvgFillColor(N, state.style->secondaryColor.nvg(selected ? 0.8f : 1.0f));
        nvgRoundedRect(N, x, y, w, h, 3.0f);
        nvgFill(N);
        nvgStroke(N);
      }
    }
    break;

    case Bevel::TextBox:
    {
      nvgBeginPath(N);
      nvgStrokeWidth(N, 1.0f);
      nvgStrokeColor(N, state.style->color.nvg(0.25f));
      nvgFillColor(N, state.style->color.nvg(0.65f));
      nvgRoundedRect(N, x, y, w, h, 3.0f);
      nvgFill(N);
      nvgStroke(N);

      nvgBeginPath(N);
      nvgStrokeColor(N, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.125f));
      nvgMoveTo(N, x + 1, y + h - 1);
      nvgLineTo(N, x + 1, y + 3);
      nvgArcTo(N, x + 1, y + 1, x + 3, y + 1, 2);
      nvgLineTo(N, x + w - 1, y + 1);
      nvgStroke(N);

      if (controlState & (nui::State::Focused | nui::State::DeepFocused))
      {
        nvgBeginPath(N);
        nvgStrokeWidth(N, 1.0f);
        nvgStrokeColor(N, state.style->secondaryColor.nvg());
        nvgRoundedRect(N, x, y, w, h, 3.0f);
        nvgStroke(N);
      }
    }
    break;

    default:
      break;
  }
}

#undef I
#undef N

}
