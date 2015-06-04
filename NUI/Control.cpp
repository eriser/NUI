#include "Control.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
Control::ControlPointInfo Control::controlAtPoint(int x, int y)
{
  ControlPointInfo result;
  result.control = this;

  if (_flags & CanResize)
  {
    if (x < Graphics::Style::ResizeEdgeSize) result.edges |= Edge::Left;
    if (y < Graphics::Style::ResizeEdgeSize) result.edges |= Edge::Top;
    if (x >= _rect.width - Graphics::Style::ResizeEdgeSize) result.edges |= Edge::Right;
    if (y >= _rect.height - Graphics::Style::ResizeEdgeSize) result.edges |= Edge::Bottom;
    
    if (result.edges & (Edge::Left | Edge::Right))
    {
      if (y < Graphics::Style::ResizeCornerSize) result.edges |= Edge::Top;
      if (y >= _rect.height - Graphics::Style::ResizeCornerSize) result.edges |= Edge::Bottom;
    }
    else if (result.edges & (Edge::Top | Edge::Bottom))
    {
      if (x < Graphics::Style::ResizeCornerSize) result.edges |= Edge::Left;
      if (x >= _rect.width - Graphics::Style::ResizeCornerSize) result.edges |= Edge::Right;
    }
  }

  if ((_flags & CanMove) && !result.edges)
  {
    if ((_titleHeight && y < _titleHeight) || !_titleHeight)
    {
      result.edges |= Edge::All;
    }
  }

  x -= _padding.left;
  y -= _padding.top + _titleHeight;

  if (!result.edges || result.edges == Edge::All)
  {
    for (auto child : _children)
    {
      if (!child->isVisible()) continue;

      if (child->_docking != Docking::None && child->_docking != Docking::Client)
      {
        if (child->_rect.isPointInside(x, y))
        {
          int px = x - child->_rect.x;
          int py = y - child->_rect.y;

          result = child->controlAtPoint(px, py);
        }
      }
    }

    x -= _undockedOffset.x;
    y -= _undockedOffset.y;

    for (auto child : _children)
    {
      if (!child->isVisible()) continue;

      if ((child->_docking == Docking::None || child->_docking == Docking::Client) && !(child->_flags & ParentClip))
      {
        if (child->_rect.isPointInside(x, y))
        {
          int px = x - child->_rect.x;
          int py = y - child->_rect.y;

          result = child->controlAtPoint(px, py);
        }
      }
    }

    x += _undockedOffset.x;
    y += _undockedOffset.y;

    for (auto child : _children)
    {
      if (!child->isVisible()) continue;

      if (child->_docking == Docking::None && (child->_flags & ParentClip))
      {
        if (child->_rect.isPointInside(x, y))
        {
          int px = x - child->_rect.x;
          int py = y - child->_rect.y;

          result = child->controlAtPoint(px, py);
        }
      }
    }
  }

  return result;
}

//---------------------------------------------------------------------------------------------------------------------
void Control::clampResizeStep(int &dx, int &dy) const
{
  if (_docking == Docking::None)
  {
    Vec2 clampedSize = Vec2(
      clampMinMax((_rect.width + dx), _minimumSize.x, _maximumSize.x),
      clampMinMax((_rect.height + dy), _minimumSize.y, _maximumSize.y));

    dx = clampedSize.x - _rect.width;
    dy = clampedSize.y - _rect.height;

    for (auto child : _children)
      child->clampResizeStep(dx, dy);
  }
}

//---------------------------------------------------------------------------------------------------------------------
void Control::resizeStep(int dx, int dy)
{
  if (!dx && !dy) return;

  _rect.width += dx;
  _rect.height += dy;

  resizeStepChildren(dx, dy);
  arrangeChildren();
}

//---------------------------------------------------------------------------------------------------------------------
void Control::resizeStepChildren(int dx, int dy)
{
  for (auto child : _children)
  {
    if (child->_docking == Docking::None)
    {
      Rect &r = child->_rect;
      unsigned a = child->_anchors;
      Vec2 delta;

      if (!(a & Edge::Left)) r.x += dx; else if (a & Edge::Right) delta.x = dx;
      if (!(a & Edge::Top)) r.y += dy; else if (a & Edge::Bottom) delta.y = dy;

      child->resizeStep(delta.x, delta.y);
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
void Control::updateContentSize()
{
  Borders dockingBorders;
  Vec2 minimumDockingSize;

  _contentRect = Rect(0, 0, _rect.width - _padding.getHorizontal(), _rect.height - _padding.getVertical() - _titleHeight);

  // Undocked bounds
  Vec2 undockedTopLeft;
  Vec2 undockedBottomRight;
  bool hasUndocked = false;

  // Calculate bounds for undocked children
  for (auto child : _children)
  {
    if (!child->isVisible()) continue;

    switch (child->_docking)
    {
      case Docking::None:
      {
        if (!(child->_flags & ParentClip))
        {
          const Rect &rect = child->_rect;

          if (!hasUndocked)
          {
            hasUndocked = true;
            undockedTopLeft = Vec2(rect.x, rect.y);
            undockedBottomRight = Vec2(rect.x + rect.width, rect.y + rect.height);
          }
          else
          {
            undockedTopLeft.x = minimum(undockedTopLeft.x, rect.x);
            undockedTopLeft.y = minimum(undockedTopLeft.y, rect.y);
            undockedBottomRight.x = maximum(undockedBottomRight.x, rect.x + rect.width);
            undockedBottomRight.y = maximum(undockedBottomRight.y, rect.y + rect.height);
          }
        }
      }
      break;

      default:
        break;
    }
  }

  for (auto child : _children)
  {
    if (!child->isVisible()) continue;

    const Rect &rect = child->_rect;
    const Borders &margins = child->_margins;
    const Vec2 &min = child->_minimumSize;

    switch (child->_docking)
    {
      case Docking::Top:
        dockingBorders.top += clampMinimum(rect.height, min.y) + margins.bottom;
        if (min.x)
          minimumDockingSize.x = maximum(minimumDockingSize.x, dockingBorders.getHorizontal() + min.x);
        break;

      case Docking::Right:
        dockingBorders.right += clampMinimum(rect.width, min.x) + margins.left;
        if (min.y)
          minimumDockingSize.y = maximum(minimumDockingSize.y, dockingBorders.getVertical() + min.y);
        break;

      case Docking::Bottom:
        dockingBorders.bottom += clampMinimum(rect.height, min.y) + margins.top;
        if (min.x)
          minimumDockingSize.x = maximum(minimumDockingSize.x, dockingBorders.getHorizontal() + min.x);
        break;

      case Docking::Left:
        dockingBorders.left += clampMinimum(rect.width, min.x) + margins.right;
        if (min.y)
          minimumDockingSize.y = maximum(minimumDockingSize.y, dockingBorders.getVertical() + min.y);
        break;

      case Docking::Client:
        break;

      default:
        continue;
    }
  }

  _undockedOffset.x = dockingBorders.left;
  _undockedOffset.y = dockingBorders.top;
  
  if (undockedTopLeft.x < 0)
    _contentRect.shrinkHorizontal(undockedTopLeft.x);

  if (undockedTopLeft.y < 0)
    _contentRect.shrinkVertical(undockedTopLeft.y);

  Vec2 contentBottomRight = _contentRect.getBottomRight();
  contentBottomRight.x -= dockingBorders.getHorizontal();
  contentBottomRight.y -= dockingBorders.getVertical();

  if (undockedBottomRight.x > contentBottomRight.x)
    _contentRect.width += undockedBottomRight.x - contentBottomRight.x;

  if (undockedBottomRight.y > contentBottomRight.y)
    _contentRect.height += undockedBottomRight.y - contentBottomRight.y;

  undockedTopLeft += _undockedOffset;
  undockedBottomRight += _undockedOffset;

  if (minimumDockingSize.x)
    _contentRect.width = maximum(maximum(_contentRect.width, minimumDockingSize.x), dockingBorders.getHorizontal());
  else
    _contentRect.width = maximum(_contentRect.width, dockingBorders.getHorizontal());

  if (minimumDockingSize.y)
    _contentRect.height = maximum(maximum(_contentRect.height, minimumDockingSize.y), dockingBorders.getVertical());
  else
    _contentRect.height = maximum(_contentRect.height, dockingBorders.getVertical());
}

//---------------------------------------------------------------------------------------------------------------------
bool Control::arrangeChildren()
{
  updateContentSize();

  bool needsRearranging = false;
  
  // Rectangular are that is being "redistributed" between child controls
  Rect rect = _contentRect;

  // Dock children
  for (auto child : _children)
  {
    if (!child->isVisible()) continue;

    Rect r = child->_rect;
    Borders &margins = child->_margins;
    const Vec2 &m = child->_minimumSize;

    switch (child->_docking)
    {
      case Docking::Top:
      {
        r.x = rect.x;
        r.y = rect.y;
        r.width = rect.width;
        r.height = r.height;
        needsRearranging |= r.ensureMinimumSize(m);
        rect.shrinkVertical(r.height + margins.bottom);
      }
      break;

      case Docking::Right:
      {
        r.x = rect.x + rect.width - r.width;
        r.y = rect.y;
        r.width = r.width;
        r.height = rect.height;
        needsRearranging |= r.ensureMinimumSize(m);
        rect.width -= r.width + margins.left;
      }
      break;

      case Docking::Bottom:
      {
        r.x = rect.x;
        r.y = rect.y + rect.height - r.height;
        r.width = rect.width;
        r.height = r.height;
        needsRearranging |= r.ensureMinimumSize(m);
        rect.height -= r.height + margins.top;
      }
      break;

      case Docking::Left:
      {
        r.x = rect.x;
        r.y = rect.y;
        r.width = r.width;
        r.height = rect.height;
        needsRearranging |= r.ensureMinimumSize(m);
        rect.shrinkHorizontal(r.width + margins.right);
      }
      break;

      default:
        continue;
    }

    child->setRect(r);
  }

  // Position undocked children
  for (auto child : _children)
  {
    if (!child->isVisible()) continue;

    Rect r = child->_rect;
    const Vec2 &m = child->_minimumSize;

    switch (child->_docking)
    {
      case Docking::Client:
      {
        r.x = 0;
        r.y = 0;
        r.width = rect.width;
        r.height = rect.height;
        needsRearranging |= r.ensureMinimumSize(m);
      }
      break;

      default:
        continue;
    }

    child->setRect(r);
  }

  return needsRearranging;
}

//---------------------------------------------------------------------------------------------------------------------
void Control::tick(double time, double delta)
{
  if (_dirty)
  {
    if (arrangeChildren())
      arrangeChildren();

    _dirty = false;
  }

  for (auto child : _children)
    child->tick(time, delta);
}

//---------------------------------------------------------------------------------------------------------------------
void Control::bringToFront()
{
  if (!_parent) return;

  Control::Ptr self = this;
  size_t parentIndex = 0;
  for (auto child : _parent->_children)
    if (child == this) break; else ++parentIndex;

  for (size_t i = parentIndex, S = _parent->_children.size() - 1; i < S; ++i)
    _parent->_children[i] = _parent->_children[i + 1];

  _parent->_children.back() = self;
}

//---------------------------------------------------------------------------------------------------------------------
void Control::autoSize(bool recursive)
{
  if (recursive)
  {
    for (auto child : _children)
    {
      if (!child->isVisible()) continue;

      child->autoSize(true);
    }
  }

  updateContentSize();
  setSize(_contentRect.width + _padding.getHorizontal(), _contentRect.height + _padding.getVertical() + _titleHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void Control::processEvent(Event &e, bool propagateUp, bool propagateDown)
{
  if (propagateDown && !_children.empty())
  {
    for (auto child : _children)
      child->processEvent(e, false, true);
  }

  if (propagateUp && _parent)
    _parent->processEvent(e, true, false);
}

}
