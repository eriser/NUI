#pragma once

#include "Graphics.h"
#include "Events.h"

#if !defined(NUI_CONTROL)
#define NUI_CONTROL(type, super) \
  typedef super Super; \
  typedef nui::Ptr<type> Ptr; \
  nui::Control::Type getType() const override { return nui::Control::Type::type; }
#endif

namespace nui {

/* Forward declarations */
class Control;
class Box;
class Button;
class ComboBox;
class CheckBox;
class ListBox;
class Menu;
class MenuBar;
class MenuItem;
class Root;
class ScrollBar;
class TextBox;
class TreeView;
class Window;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Control : public Object
{
  friend class Root;

  public:
    typedef nui::Ptr<Control> Ptr;

    explicit Control(Control *parent = nullptr, const std::string &text = std::string(), Docking docking = Docking::None, int icon = -1)
      : Object()
    {
      setParent(parent);
      _text = text;
      _docking = docking;
      _icon = icon;
    }

    virtual void preDraw(Graphics *graphics) { }

    virtual void draw(Graphics *graphics) { }

    virtual void postDraw(Graphics *graphics) { }
    
    virtual void tick(double time, double delta);

    enum class Type
    {
      Unknown = 0,
      Box,
      Button,
      ComboBox,
      CheckBox,
      ListBox,
      Menu,
      MenuItem,
      MenuBar,
      Root,
      ScrollBar,
      TextBox,
      TreeView,
      Window,
    };

    virtual Type getType() const { return Type::Unknown; }
    
    bool is(Type type) const { return getType() == type; }

    enum Flags
    {
      None = 0,
      Visible = 1,
      Enabled = Visible << 1,
      CanFocus = Enabled << 1,
      CanMove = CanFocus << 1,
      CanResize = CanMove << 1,
      CanMaximize = CanResize << 1,
      CanMinimize = CanMaximize << 1,
      CanDockChildren = CanMinimize << 1,
      AlwaysBringToFront = CanDockChildren << 1,
      ParentClip = AlwaysBringToFront << 1,
      CustomColors = ParentClip << 1,
      PreserveDeepFocus = CustomColors << 1,
      PreDraw = PreserveDeepFocus << 1,
      Draw = PreDraw << 1,
      PostDraw = Draw << 1,
      NeedsTextInput = PostDraw << 1,
      Flat = NeedsTextInput << 1,
    };

    void setFlags(unsigned flags)
    {
      if (_flags != flags)
      {
        _flags = flags;
        setDirty();
      }
    }

    unsigned getFlags() const { return _flags; }

    void addFlags(unsigned flags) { setFlags(_flags | flags); }

    void removeFlags(unsigned flags) { setFlags(_flags & ~flags); }

    void setDirty(bool set = true)
    {
      if (_dirty != set)
      {
        _dirty = set;

        if (set)
        {
          if (_parent)
            _parent->_dirty = true;
        }

        for (auto child : _children)
          child->setDirty(set);
      }
    }

    bool getDirty() const { return _dirty; }

    void setParent(Control *parent)
    {
      Control::Ptr p = _parent;

      if (p != parent)
      {
        _parent = parent;
        setDirty();

        // New parent
        if (parent)
          parent->addChild(this);

        // Old parent
        if (p)
          p->removeChild(this);

        Event e(Event::Type::ParentChanged, this);
        e.parentChanged.oldParent = p;
        processEvent(e);
      }
    }

    Control *getParent() const { return _parent; }

    bool hasParent(const Control *parent) const
    {
      if (_parent == parent)
        return true;

      return _parent ? _parent->hasParent(parent) : false;
    }

    Root *getRoot() const { return _parent ? _parent->getRoot() : reinterpret_cast<Root *>(const_cast<Control *>(this)); }

    void clear();

    size_t getNumChildren() const { return _children.size(); }

    Control *getChild(size_t index) const { return _children[index]; }
    
    typedef std::vector<Control::Ptr> Children;

    Children::const_iterator begin() const { return _children.begin(); }

    Children::const_iterator end() const { return _children.end(); }

    void show(bool visible = true) { visible ? addFlags(Visible) : removeFlags(Visible); }

    bool isVisible(bool recursive = false) const
    {
      return recursive
        ? (_parent ? (_parent->isVisible(true) && (_flags & Visible) != 0) : ((_flags & Visible) != 0))
        : ((_flags & Visible) != 0);
    }

    int getDepth() const
    {
      int result = 0;
      const Control *ctrl = _parent;

      while (ctrl)
      {
        ++result;
        ctrl = ctrl->_parent;
      }

      return result;
    }

    virtual void setText(const std::string &text)
    {
      if (_text != text)
      {
        _text = text;
        setDirty();
      }
    }

    const std::string &getText() const { return _text; }

    void setIcon(int iconID) { _icon = iconID; }

    int getIcon() const { return _icon; }

    void setMinimumSize(int width, int height)
    {
      if (_minimumSize.set(width, height))
      {
        setSize(_rect.width, _rect.height);
        setDirty();
      }
    }

    const Vec2 &getMinimumSize() const { return _minimumSize; }

    void setMaximumSize(int width, int height)
    {
      if (_maximumSize.set(width, height))
      {
        setSize(_rect.width, _rect.height);
        setDirty();
      }
    }

    void setPosition(int x, int y)
    {
      if (_rect.x != x || _rect.y != y)
      {
        _rect.x = x;
        _rect.y = y;
        setDirty();
      }
    }

    Vec2 getPosition() const { return _rect.getPosition(); }

    Vec2 getAbsolutePosition() const
    {
      Vec2 position = _rect.getPosition();

      const Control *p = _parent;
      const Control *child = this;
      while (p)
      {
        position.x += p->_rect.x + p->_padding.left;
        position.y += p->_rect.y + p->_padding.top + p->_titleHeight;

        if (child->_docking == Docking::None || child->_docking == Docking::Client)
        {
          position.x += p->_undockedOffset.x;
          position.y += p->_undockedOffset.y;
        }

        child = p;
        p = p->_parent;
      }

      return position;
    }

    int getX() const { return _rect.x; }

    int getY() const { return _rect.y; }

    void setRect(const Rect &rect)
    {
      int dx = rect.width - _rect.width;
      int dy = rect.height - _rect.height;
      clampResizeStep(dx, dy);

      if (_rect.x != rect.x || _rect.y != rect.y || dx || dy)
      {
        _rect.x = rect.x;
        _rect.y = rect.y;
        resizeStep(dx, dy);
        setDirty();
      }
    }

    void setRect(int x, int y, int width, int height) { setRect(Rect(x, y, width, height)); }

    const Rect &getRect() const { return _rect; }

    void setSize(int width, int height)
    {
      width = clampMinMax(width, _minimumSize.x, _maximumSize.x);
      height = clampMinMax(height, _minimumSize.y, _maximumSize.y);

      if (_rect.width != width || _rect.height != height)
      {
        resizeStep(width - _rect.width, height - _rect.height);
        setDirty();
      }
    }

    Vec2 getSize() const { return _rect.getSize(); }
    
    int getWidth() const { return _rect.width; }

    int getHeight() const { return _rect.height; }

    void setTitleHeight(int height)
    {
      if (_titleHeight != height)
      {
        _titleHeight = height;
        setDirty();
      }
    }

    int getTitleHeight() const { return _titleHeight; }

    unsigned getState() const { return _state; }

    void setMargins(int top, int right, int bottom, int left)
    {
      if (_margins.set(top, right, bottom, left))
        setDirty();
    }

    void setMargins(int all) { setMargins(all, all, all, all); }

    void setMargins(int horizontal, int vertical) { setMargins(horizontal, vertical, horizontal, vertical); }

    const Borders &getMargins() const { return _margins; }

    void setPadding(int top, int right, int bottom, int left)
    {
      if (_padding.set(top, right, bottom, left))
        setDirty();
    }

    void setPadding(short all) { setPadding(all, all, all, all); }

    void setPadding(short horizontal, short vertical) { setPadding(horizontal, vertical, horizontal, vertical); }

    const Borders &getPadding() const { return _padding; }

    void setDocking(Docking docking)
    {
      if (_docking != docking)
      {
        _docking = docking;
        setDirty();
      }
    }

    Docking getDocking() const { return _docking; }

    void setAnchors(unsigned anchors) { _anchors = anchors; }
    
    unsigned getAnchors() const { return _anchors; }

    struct ControlPointInfo
    {
      Control::Ptr control;
      unsigned edges = Edge::None;
    };

    ControlPointInfo controlAtPoint(int x, int y);

    void setStyle(Graphics::Style *style) { _style = style; }

    Graphics::Style *getStyle(bool recursive = false) const
    {
      if (!_style && recursive && _parent)
        return _parent->getStyle(true);

      return _style;
    }

    void setAlpha(float a)
    {
      if (_alpha != a)
      {
        _alpha = a;
        setDirty();
      }
    }

    float getAlpha() const { return _alpha; }

    void bringToFront();

    virtual void autoSize(bool recursive = false);

    virtual void processEvent(Event &e, bool propagateUp = true, bool propagateDown = false);

  protected:
    virtual ~Control() { }

    void addState(unsigned state) { _state |= state; }

    void removeState(unsigned state) { _state &= ~state; }

    void addChild(Control *child)
    {
      _children.push_back(child);
      setDirty();
    }

    void removeChild(Control *child)
    {
      for (auto iter = _children.begin(); iter != _children.end(); ++iter)
      {
        if ((*iter) == child)
        {
          _children.erase(iter);
          setDirty();
          return;        
        }
      }
    }

    void clampResizeStep(int &dx, int &dy) const;

    void updateContentSize();

    bool arrangeChildren();

    void resizeStep(int dx, int dy);

    void resizeStepChildren(int dx, int dy);

    // Control has been changed and not updated yet
    bool _dirty = true;

    // General control flags
    unsigned _flags = Control::Visible | Control::Enabled | Control::Draw;

    // Text/title string
    std::string _text = "";

    // Icon ID
    int _icon = -1;

    // Parent control
    Control *_parent = nullptr;
    
    // Children controls
    Children _children;

    // State
    unsigned _state = State::None;

    // Title height
    int _titleHeight = 0;

    // Rectagle
    Rect _rect = Rect(0, 0, Graphics::Style::DefaultControlWidth, Graphics::Style::DefaultControlHeight);

    // Minimum size
    Vec2 _minimumSize;

    // Maximum size
    Vec2 _maximumSize;

    // Content rectangle required to fit all children in
    Rect _contentRect;

    // Offset of children which are not docked
    Vec2 _undockedOffset;

    // Margins
    Borders _margins = Borders(Graphics::Style::DefaultMargin);

    // Padding
    Borders _padding = Borders(Graphics::Style::DefaultPadding);

    // Docking mode
    Docking _docking = Docking::None;

    // Anchor flags
    unsigned _anchors = Edge::Top | Edge::Left;
    
    // Primary control color
    Graphics::Style::Ptr _style;

    // Alpha
    float _alpha = 1.0f;
};

}
