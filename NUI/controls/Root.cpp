#include "Root.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
Root::Root(NVGcontext *nvgCtx)
  : Control()
  , _nvgContext(nvgCtx)
{
  setFlags(getFlags() | CanDockChildren);
  setMargins(0);
  setPadding(0);
  setStyle(new Graphics::Style());

  _normalFontID = nvgCreateFont(nvgCtx, "default", "DejaVuSans.ttf");
  _monospaceFontID = nvgCreateFont(nvgCtx, "default", "DejaVuSansMono.ttf");
 
  _iconAtlasInfo.imageID = nvgCreateImage(nvgCtx, "blender_icons16.png", 0);
  nvgImageSize(nvgCtx, _iconAtlasInfo.imageID, &_iconAtlasInfo.imageSize.x, &_iconAtlasInfo.imageSize.y);

  _iconAtlasInfo.offset.set(13, 18);
  _iconAtlasInfo.iconSize.set(18, 18);
  _iconAtlasInfo.iconStride.set(21, 21);
  _iconAtlasInfo.iconMargin = 4;
}

//---------------------------------------------------------------------------------------------------------------------
void Root::traverseControl(Graphics *graphics, Control *control)
{
  Graphics::State oldState = graphics->state;

  const Rect &r = control->getRect();
  int titleHeight = control->getTitleHeight();

  nvgSave(_nvgContext);

  // Transform coordinate system so that [0, 0] is control's top-left corner
  nvgTranslate(_nvgContext, static_cast<float>(r.x), static_cast<float>(r.y));

  float alpha = 1.0f;

  graphics->state.alpha *= alpha;
  nvgGlobalAlpha(_nvgContext, graphics->state.alpha);

  if (control->_style)
    graphics->state.style = control->_style;

  if (control->_flags & Flags::PreDraw)
    control->preDraw(graphics);

  nvgIntersectScissor(_nvgContext, 0, 0, static_cast<float>(r.width), static_cast<float>(r.height));

  if (control->_flags & Flags::Draw)
    control->draw(graphics);

  // Pad & clip & draw child control area
  const Borders &padding = control->getPadding();

  nvgSave(_nvgContext);

  nvgTranslate(_nvgContext, static_cast<float>(padding.left), static_cast<float>(padding.top + titleHeight));
  nvgIntersectScissor(_nvgContext, 0, 0, static_cast<float>(r.width - padding.getHorizontal()), static_cast<float>(r.height - padding.getVertical() - titleHeight));

  // Draw all docked children first
  for (auto child : (*control))
  {
    if (!child->isVisible()) continue;

    if (child->_docking != Docking::None && child->_docking != Docking::Client && !((child->_flags & ParentClip)))
      traverseControl(graphics, child);
  }

  nvgTranslate(_nvgContext, static_cast<float>(control->_undockedOffset.x), static_cast<float>(control->_undockedOffset.y));

  // Draw remaining undocked children with standard clipping
  for (auto child : (*control))
  {
    if (!child->isVisible()) continue;

    if (child->_docking == Docking::None && (child->_flags & ParentClip))
      continue;
    
    if (child->_docking == Docking::None || child->_docking == Docking::Client)
      traverseControl(graphics, child);
  }

  nvgRestore(_nvgContext);

  nvgTranslate(_nvgContext, static_cast<float>(padding.left), static_cast<float>(padding.top + titleHeight));

  // Draw children with parent clipping
  for (auto child : (*control))
  {
    if (!child->isVisible()) continue;

    if (child->_docking == Docking::None && (child->_flags & ParentClip))
      traverseControl(graphics, child);
  }

  if (control->_flags & Flags::PostDraw)
    control->postDraw(graphics);

  nvgRestore(_nvgContext);
  graphics->state = oldState;
}

//---------------------------------------------------------------------------------------------------------------------
void Root::draw()
{
  Graphics graphics;
  graphics.nvgContext = _nvgContext;
  graphics.normalFontID = _normalFontID;
  graphics.monospaceFontID = _monospaceFontID;
  graphics.iconAtlasInfo = _iconAtlasInfo;

  graphics.state.style = getStyle();
  graphics.state.alpha = 1.0f;
  
  traverseControl(&graphics, this);
}

//---------------------------------------------------------------------------------------------------------------------
Vec2 Root::measureText(float fontSize, const char *text) const
{
  float bounds[4];
  Vec2 result;

  nvgFontSize(_nvgContext, fontSize);
  nvgTextBounds(_nvgContext, 0, 0, text, nullptr, bounds);

  result.set(static_cast<int>(bounds[2]), static_cast<int>(bounds[3]));
  return result;
}

//---------------------------------------------------------------------------------------------------------------------
void Root::setExclusiveControl(Control *control)
{
  if (_exclusiveControl != control)
  {
    if (_exclusiveControl)
    {
      _exclusiveControl->setParent(_exclusiveOldParent);
      _exclusiveControl->setPosition(_exclusiveOldPosition.x, _exclusiveOldPosition.y);

      _exclusiveOldParent = nullptr;
      _exclusiveControl->removeState(State::Exclusive);
    }

    Control *oldExclusiveControl = _exclusiveControl;

    _exclusiveControl = control;

    if (oldExclusiveControl)
    {
      Event e(Event::Type::ExclusivityChanged, oldExclusiveControl);
      oldExclusiveControl->processEvent(e);
    }

    if (_exclusiveControl)
    {
      _exclusiveOldParent = _exclusiveControl->getParent();
      _exclusiveOldPosition = _exclusiveControl->getPosition();

      Vec2 absPos = _exclusiveControl->getAbsolutePosition();
      _exclusiveControl->setPosition(absPos.x, absPos.y);
      _exclusiveControl->addState(State::Exclusive);
      _exclusiveControl->addFlags(Flags::ParentClip);
      _exclusiveControl->setParent(this);

      Event e(Event::Type::ExclusivityChanged, _exclusiveControl);
      _exclusiveControl->processEvent(e);
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
bool Root::eventMouseMotion(int x, int y)
{
  Vec2 delta = Vec2(x - _mousePosition.x, y - _mousePosition.y);
  ControlPointInfo hot = controlAtPoint(x, y);

  switch (_mouseMode)
  {
    case MouseMode::Normal:
    {
      _mouseCursor = MouseCursor::Default;
      setHotControl(hot.control, hot.edges);

      if (hot.edges)
      {
        unsigned resizeEdges = (hot.edges & Edge::All);

        switch (resizeEdges)
        {
          case Edge::Top:
          case Edge::Bottom:
            _mouseCursor = MouseCursor::ResizeV;
            break;

          case Edge::Left:
          case Edge::Right:
            _mouseCursor = MouseCursor::ResizeH;
            break;

          case Edge::TopLeft:
          case Edge::BottomRight:
            _mouseCursor = MouseCursor::ResizeLR;
            break;

          case Edge::TopRight:
          case Edge::BottomLeft:
            _mouseCursor = MouseCursor::ResizeRL;
            break;
        }
      }
    }
    break;

    case MouseMode::Down:
    {
      if (_grabbedControl)
      {
        if (hot.control != _grabbedControl && (_grabbedControl->_state & State::Down))
        {
          _grabbedControl->removeState(State::Down);
          _grabbedControl->setDirty();
        }
        else if (hot.control == _grabbedControl && !(_grabbedControl->_state & State::Down))
        {
          _grabbedControl->addState(State::Down);
          _grabbedControl->setDirty();
        }
      }
    }
    break;

    case MouseMode::Moving:
    {
      if (_grabbedControl)
      {
        _grabbedControl->setPosition(_grabbedControl->getX() + delta.x, _grabbedControl->getY() + delta.y);
        _mouseCursor = MouseCursor::Move;

        Event e(Event::Type::Moving, _grabbedControl);
        e.moving.deltaX = delta.x;
        e.moving.deltaY = delta.y;
        _grabbedControl->processEvent(e);
      }
    }
    break;

    case MouseMode::Resizing:
    {
      if (_grabbedControl)
      {
        if (!(_grabbedEdges & Edge::TopOrBottom)) delta.y = 0;
        if (!(_grabbedEdges & Edge::LeftOrRight)) delta.x = 0;
        if (_grabbedEdges & Edge::Left) delta.x *= -1;
        if (_grabbedEdges & Edge::Top) delta.y *= -1;
        
        _grabbedControl->clampResizeStep(delta.x, delta.y);

        if (_grabbedControl->_docking != Docking::None && _grabbedControl->_parent)
        {
          delta *= -1;
          _grabbedControl->_parent->clampResizeStep(delta.x, delta.y);
          delta *= -1;
        }

        if (_grabbedEdges & Edge::Left) { _grabbedControl->_rect.x -= delta.x; }
        if (_grabbedEdges & Edge::Top) { _grabbedControl->_rect.y -= delta.y; }

        _grabbedControl->resizeStep(delta.x, delta.y);

        if (_grabbedControl->_docking != Docking::None && _grabbedControl->_parent)
        {
          delta *= -1;
          _grabbedControl->_parent->resizeStepChildren(delta.x, delta.y);
          _grabbedControl->_parent->arrangeChildren();
          delta *= -1;
        }

        Event e(Event::Type::Resizing, _grabbedControl);
        e.resizing.deltaX = delta.x;
        e.resizing.deltaY = delta.y;
        e.resizing.edges = _grabbedEdges;
        _grabbedControl->processEvent(e);
      }
    }
    break;

    default:
      break;
  }

  Control *eventSender = _grabbedControl ? _grabbedControl : _hotControl;
  if (eventSender)
  {
    Vec2 absPos = eventSender->getAbsolutePosition();

    Event e(Event::Type::MouseMotion, eventSender);
    e.mouseMotion.x = x - absPos.x;
    e.mouseMotion.y = y - absPos.y;
    e.mouseMotion.deltaX = delta.x;
    e.mouseMotion.deltaY = delta.y;
    e.mouseMotion.grabbedX = _grabbedPosition.x - absPos.x;
    e.mouseMotion.grabbedY = _grabbedPosition.y - absPos.y;
    e.mouseMotion.buttonFlags = _mouseButtonFlags;
    eventSender->processEvent(e);
  }

  _mousePosition.set(x, y);
  return false;
}

//---------------------------------------------------------------------------------------------------------------------
bool Root::eventMouseButtonDown(MouseButton button)
{
  _mouseButtonFlags |= static_cast<unsigned>(button);

  // Test is we should get rid of exclusive control (menus, dropdowns, etc.)
  if (_exclusiveControl)
  {
    if (_hotControl != _exclusiveControl && !_hotControl->hasParent(_exclusiveControl))
      setExclusiveControl(nullptr);
  }

  switch (_mouseMode)
  {
    case MouseMode::Normal:
    {
      if (button == MouseButton::Left)
      {
        if (_hotControl)
        {
          if (_hotControl->_flags & Control::CanFocus)
            setFocusedControl(_hotControl);

          if (_hotEdges == Edge::All)
          {
            setGrabbedControl(_hotControl, _hotEdges);
            _mouseMode = MouseMode::Moving;
          }
          else if (_hotEdges & Edge::All)
          {
            setGrabbedControl(_hotControl, _hotEdges);
            _mouseMode = MouseMode::Resizing;
          }
          else
          {
            setGrabbedControl(_hotControl, 0);
            _hotControl->addState(State::Down);
            _mouseMode = MouseMode::Down;
          }
        }
      }
    }
    break;

    default:
      break;
  }

  Control *eventSender = _grabbedControl ? _grabbedControl : _hotControl;
  if (eventSender)
  {
    Vec2 absPos = eventSender->getAbsolutePosition();

    Event e(Event::Type::MouseButton, eventSender);
    e.mouseButton.down = true;
    e.mouseButton.button = button;
    e.mouseButton.buttonFlags = _mouseButtonFlags;
    e.mouseButton.x = _mousePosition.x - absPos.x;
    e.mouseButton.y = _mousePosition.y - absPos.y;
    eventSender->processEvent(e);
  }

  return false;
}

//---------------------------------------------------------------------------------------------------------------------
bool Root::eventMouseButtonUp(MouseButton button)
{
  _mouseButtonFlags &= ~static_cast<unsigned>(button);

  switch (_mouseMode)
  {
    case MouseMode::Down:
    {
      if (_grabbedControl)
      {
        bool clicked = (_hotControl == _grabbedControl) && (_grabbedControl->_state & State::Down);
        _grabbedControl->removeState(State::Down);

        if (clicked)
        {
          Event e(Event::Type::Click, _grabbedControl);
          _grabbedControl->processEvent(e);
        }
      }
    }
    break;

    default:
      break;
  }

  Control *eventSender = _grabbedControl ? _grabbedControl : _hotControl;
  if (eventSender)
  {
    Vec2 absPos = eventSender->getAbsolutePosition();

    Event e(Event::Type::MouseButton, eventSender);
    e.mouseButton.down = false;
    e.mouseButton.button = button;
    e.mouseButton.buttonFlags = _mouseButtonFlags;
    e.mouseButton.x = _mousePosition.x - absPos.x;
    e.mouseButton.y = _mousePosition.y - absPos.y;
    eventSender->processEvent(e);
  }

  setGrabbedControl(nullptr, 0);
  _mouseMode = MouseMode::Normal;

  eventMouseMotion(_mousePosition.x, _mousePosition.y);
  return false;
}

//---------------------------------------------------------------------------------------------------------------------
void Root::setGrabbedControl(Control *control, unsigned edges)
{
  if (_grabbedControl != control)
  {
    if (_grabbedControl)
    {
      _grabbedControl->removeState(State::Moving | State::Resizing | State::Down | State::Grabbed);
      _grabbedControl->setDirty();
    }

    _grabbedControl = control;
    _grabbedEdges = edges;

    if (_grabbedControl)
    {
      if (edges == Edge::All)
        _grabbedControl->addState(State::Moving);
      else
        _grabbedControl->addState(State::Resizing);

      _grabbedRect = _grabbedControl->_rect;
      _grabbedPosition = _mousePosition;

      _grabbedControl->addState(State::Grabbed);
      _grabbedControl->setDirty();

      Control *c = _grabbedControl;
      while (c)
      {
        if (c->_flags & AlwaysBringToFront)
          c->bringToFront();

        c = c->_parent;
      }
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
void Root::setHotControl(Control *control, unsigned edges)
{
  if (control != _hotControl)
  {
    if (_hotControl)
    {
      _hotControl->removeState(State::Hot);
      _hotControl->setDirty();

      Event e(Event::Type::HotChanged, _hotControl);
      _hotControl->processEvent(e);
    }

    _hotControl = control;

    if (_hotControl)
    {
      _hotControl->addState(State::Hot);
      _hotControl->setDirty();

      Event e(Event::Type::HotChanged, _hotControl);
      _hotControl->processEvent(e);
    }
  }
  
  if (edges != _hotEdges)
  {
    _hotEdges = edges;
  }
}

//---------------------------------------------------------------------------------------------------------------------
void Root::setFocusedControl(Control *control)
{
  if (control && !(control->_flags & CanFocus))
    return;

  if ((control->_flags & PreserveDeepFocus) && (control->_state & State::DeepFocused))
    return;

  if (control != _focusedControl)
  {
    if (_focusedControl)
    {
      _focusedControl->removeState(State::Focused);
      _focusedControl->setDirty();

      Event e(Event::Type::FocusChanged, _focusedControl);
      _focusedControl->processEvent(e);

      Control *c = _focusedControl->_parent;
      while (c)
      {
        if (c->_state & State::DeepFocused)
        {
          c->removeState(State::DeepFocused);
          c->setDirty();
        }

        c = c->_parent;
      }
    }

    _focusedControl = control;

    if (_focusedControl)
    {
      _focusedControl->addState(State::Focused);
      _focusedControl->setDirty();

      Event e(Event::Type::FocusChanged, _focusedControl);
      _focusedControl->processEvent(e);

      Control *c = _focusedControl->_parent;
      while (c)
      {
        if (!(c->_state & State::DeepFocused))
        {
          c->addState(State::DeepFocused);
          c->setDirty();
        }

        c = c->_parent;
      }
    }
  }
}

}
