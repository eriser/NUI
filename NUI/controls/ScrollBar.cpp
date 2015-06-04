#include "ScrollBar.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
ScrollBar::ScrollBar(Control *parent, Docking docking)
  : Control(parent, std::string(), docking, -1)
{
  addFlags(CanFocus);
  setSize(Graphics::Style::DefaultScrollBarSize, Graphics::Style::DefaultScrollBarSize);
  setMinimumSize(Graphics::Style::DefaultScrollBarSize, Graphics::Style::DefaultScrollBarSize);
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::draw(Graphics *graphics)
{
  graphics->drawBevel(0, 0, _rect.width, _rect.height, 0, Graphics::Bevel::ButtonDown);

  if (_rect.height > _rect.width)
    graphics->drawBevel(0, _handleOffset, _rect.width, _handleSize, _state, Graphics::Bevel::ButtonUp);
  else
    graphics->drawBevel(_handleOffset, 0, _handleSize, _rect.height, _state, Graphics::Bevel::ButtonUp);
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::tick(double time, double delta)
{
  if (getDirty())
  {
    int scrollArea = maximum(_rect.width, _rect.height);
    int handleHeight = minimum(_rect.width, _rect.height);
    double range = _maximum - _minimum;

    if (_rect.width == _rect.height || range <= 0.0)
    {
      _handleSize = scrollArea;
      _handleOffset = 0;
    }
    else
    {
      _handleSize = clamp(static_cast<int>(handleHeight + static_cast<double>(scrollArea) / (range + 1)), handleHeight, maximum(handleHeight, scrollArea / 2));

      _handleOffset = static_cast<int>((_value - _minimum) / range * static_cast<double>(scrollArea - _handleSize));
      _handleOffset = clamp(_handleOffset, 0, scrollArea - _handleSize);
    }
  }

  Super::tick(time, delta);
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::setIntegerSteps(bool set)
{
  if (_integerSteps != set)
  {
    _integerSteps = set;
    setDirty();
  }
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::setLimits(double min, double max)
{
  if (min > max)
    swapValues(min, max);

  if (_minimum != min || _maximum != max)
  {
    _minimum = min;
    _maximum = max;
    _value = clamp(_value, min, max);
    setDirty();
  }
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::setValue(double value)
{
  value = clamp(value, _minimum, _maximum);
  if (_value != value)
  {
    _value = value;
    setDirty();
  }
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::processEvent(Event &e, bool propagateUp, bool propagateDown)
{
  switch (e.type)
  {
    case Event::Type::MouseMotion:
    {
      if (_state & State::Grabbed)
      {
        if (_rect.height > _rect.width)
          setHandleOffset(_grabbedOffset + (e.mouseMotion.y - e.mouseMotion.grabbedY));
        else
          setHandleOffset(_grabbedOffset + (e.mouseMotion.x - e.mouseMotion.grabbedX));
      }
    }
    break;

    case Event::Type::MouseButton:
    {
      if (e.mouseButton.down && e.mouseButton.button == MouseButton::Left)
      {
        if (_rect.height > _rect.width)
        {
          if ((e.mouseButton.y < _handleOffset) || (e.mouseButton.y >= _handleOffset + _handleSize))
            _handleOffset = e.mouseButton.y - _handleSize / 2;
        }
        else
        {
          if ((e.mouseButton.x < _handleOffset) || (e.mouseButton.x >= _handleOffset + _handleSize))
            _handleOffset = e.mouseButton.x - _handleSize / 2;
        }

        _grabbedOffset = _handleOffset;

        if (_rect.height > _rect.width)
          _handleOffset = clamp(_handleOffset, 0, _rect.height - _handleSize);
        else
          _handleOffset = clamp(_handleOffset, 0, _rect.width - _handleSize);
      }
    }
    break;

    default:
      break;
  }

  Super::processEvent(e, propagateUp, propagateDown);
}

//---------------------------------------------------------------------------------------------------------------------
void ScrollBar::setHandleOffset(int offset)
{
  if (offset != _handleOffset)
  {
    int scrollArea = maximum(_rect.width, _rect.height);
    int handleHeight = minimum(_rect.width, _rect.height);
    double range = _maximum - _minimum;

    if (_rect.width == _rect.height || range <= 0.0)
    {
      _handleOffset = 0;
      _value = _minimum;
    }
    else
    {
      offset = clamp(offset, 0, scrollArea - _handleSize);
      _handleOffset = offset;
      _value = _minimum + (static_cast<double>(_handleOffset) / static_cast<double>(scrollArea - _handleSize)) * range;
    }
  }
}

}
