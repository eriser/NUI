#pragma once

#include "Base.h"

namespace nui {

class Control;

namespace events
{
  struct MouseMotion
  {
    int x, y;
    int grabbedX, grabbedY;
    int deltaX, deltaY;
    unsigned buttonFlags;
  };

  struct MouseButton
  {
    bool down;
    int x, y;
    nui::MouseButton button;
    unsigned buttonFlags;
  };

  struct Key
  {
    bool down;
    nui::Key key;
    int character;
    unsigned modKeyFlags;
  };

  struct Resizing
  {
    int deltaX, deltaY;
    unsigned edges;
  };

  struct Moving
  {
    int deltaX, deltaY;
  };

  struct ParentChanged
  {
    Control *oldParent;
  };
}

struct Event
{
  enum class Type
  {
    MouseMotion,
    MouseButton,
    Key,
    Resizing,
    SizeChanged,
    Moving,
    FocusChanged,
    HotChanged,
    Click,
    TextChanged,
    ValueChanged,
    ParentChanged,
    ExclusivityChanged,
  };

  Type type;
  nui::Ptr<Control> sender;

  union
  {
    events::MouseMotion mouseMotion;
    events::MouseButton mouseButton;
    events::Key key;
    events::Resizing resizing;
    events::Moving moving;
    events::ParentChanged parentChanged;
  };

  Event(Type _type, Control *_sender)
    : type(_type)
    , sender(_sender)
  {
    
  }
};

}
