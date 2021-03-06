#pragma once

#include "../Control.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Button : public Control
{
  public:
    NUI_CONTROL(Button, Control);

    explicit Button(Control *parent = nullptr, const std::string &text = std::string(), Docking docking = Docking::None, int icon = -1);

    void draw(Graphics *graphics) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
