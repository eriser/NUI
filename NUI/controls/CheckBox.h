#pragma once

#include "../Control.h"

namespace nui {
  
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CheckBox : public Control
{
  public:
    NUI_CONTROL(CheckBox, Control);

    explicit CheckBox(Control *parent = nullptr, const std::string &text = std::string(), Docking docking = Docking::None);

    void draw(Graphics *graphics) override;

    void setChecked(bool set = true)
    {
      if (_checked != set)
      {
        _checked = set;
        setDirty();
      }
    }

    bool getChecked() const { return _checked; }

    void toggle() { setChecked(!_checked); }

    void processEvent(Event &e, bool propageteUp, bool propagateDown) override;

  private:
    bool _checked = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
