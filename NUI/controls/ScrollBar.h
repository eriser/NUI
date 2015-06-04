#pragma once

#include "../Control.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ScrollBar : public Control
{
  public:
    NUI_CONTROL(ScrollBar, Control);

    explicit ScrollBar(Control *parent = nullptr, Docking docking = Docking::None);

    void draw(Graphics *graphics) override;

    void tick(double time, double delta) override;

    void setIntegerSteps(bool set = true);

    bool getIntegerSteps() const { return _integerSteps; }

    void setLimits(double min, double max);

    void setMinimum(double min) { setLimits(min, _maximum); }

    void setMaximum(double max) { setLimits(_minimum, max); }

    double getMinimum() const { return _minimum; }

    double getMaximum() const { return _maximum; }

    void setValue(double value);

    double getValue() const { return _value; }

    void processEvent(Event &e, bool propagateUp, bool propagateDown) override;

    int getHandleSize() const { return _handleSize; }

    void setHandleOffset(int offset);

    int getHandleOffset() const { return _handleOffset; }

  private:
    bool _integerSteps = false;

    double _handleRatio = 0.0;

    int _handleSize = 0;

    int _handleOffset = 0;

    int _grabbedOffset = 0;

    double _minimum = 0.0;

    double _maximum = 1.0;

    double _value = 0.0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
