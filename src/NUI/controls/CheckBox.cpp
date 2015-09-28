#include "CheckBox.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
CheckBox::CheckBox(Control *parent, const std::string &text, Docking docking)
  : Control(parent, text, docking)
{
  addFlags(CanFocus);
  setMinimumSize(Graphics::Style::CheckBoxSize, Graphics::Style::CheckBoxSize);
}

//---------------------------------------------------------------------------------------------------------------------
void CheckBox::draw(Graphics *graphics)
{
  bool isDown = (_state & State::Down) != 0;
  int offset = isDown ? 1 : 0;

  const int size = Graphics::Style::CheckBoxSize;
  int y = (_rect.height - size) / 2;
  graphics->drawBevel(0, y, size, size, _state, Graphics::Bevel::ButtonDown);

  if (_checked || isDown)
    graphics->drawIcon(size / 2 + offset, _rect.height / 2 + offset, NUI_ICON_CHECK);

  graphics->drawText(size + Graphics::Style::DefaultPadding, _rect.height / 2, _text.c_str(), true, Graphics::HAlign::Left);
}

//---------------------------------------------------------------------------------------------------------------------
void CheckBox::processEvent(Event& e, bool propageteUp, bool propagateDown)
{
  switch (e.type)
  {
    case Event::Type::Click: 
    {
      toggle();
    }
    break;

    default:
      break;
  }

  Super::processEvent(e, propageteUp, propagateDown);
}

}
