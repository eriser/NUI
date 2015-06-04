#pragma once

#include "Button.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Window : public Control
{
  public:
    NUI_CONTROL(Window, Control);

    explicit Window(Control *parent = nullptr, const std::string &text = std::string(), int icon = -1);

    void preDraw(Graphics *graphics) override;

    void draw(Graphics *graphics) override;

  private:
    Button::Ptr _buttonClose;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
