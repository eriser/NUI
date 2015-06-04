#include "Button.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
Button::Button(Control *parent, const std::string &text, Docking docking, int icon)
  : Control(parent, text, docking, icon)
{
  addFlags(CanFocus);
}

//---------------------------------------------------------------------------------------------------------------------
void Button::draw(Graphics *graphics)
{
  bool isDown = (_state & State::Down) != 0;

  graphics->drawBevel(0, 0, _rect.width, _rect.height, _state, isDown ? Graphics::Bevel::ButtonDown : Graphics::Bevel::ButtonUp);

  Vec2 offset = isDown ? Vec2(1, 1) : Vec2(0, 0);

  if (!_text.empty())
  {
    if (_icon >= 0)
      offset.x += graphics->iconAtlasInfo.iconSize.x / 2;

    int tw = static_cast<int>(graphics->drawText(offset.x + _rect.width / 2, offset.y + _rect.height / 2, _text.c_str(), true));
    graphics->drawIcon(offset.x + (_rect.width - tw) / 2 - graphics->iconAtlasInfo.iconSize.x - graphics->iconAtlasInfo.iconMargin, offset.y + _rect.height / 2, _icon);
  }
  else
    graphics->drawIcon(offset.x + _rect.width / 2, offset.y + _rect.height / 2, _icon);
}

}
