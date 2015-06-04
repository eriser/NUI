#pragma once

#include "../Control.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Root : public Control
{
  public:
    NUI_CONTROL(Root, Control);

    explicit Root(NVGcontext *nvgCtx);

    enum class MouseMode
    {
      Normal = 0,
      Down,
      Moving,
      Resizing
    };

    MouseMode getMouseState() const { return _mouseMode; }

    MouseCursor getMouseCursor() const { return _mouseCursor; }

    Control *getGrabbedControl() const { return _grabbedControl; }

    Control *getHotControl() const { return _hotControl; }

    Control *getFocusedControl() const { return _focusedControl; }

    void setExclusiveControl(Control *control);

    Control *getExclusiveControl() const { return _exclusiveControl; }

    void draw();

    bool eventMouseMotion(int x, int y);

    bool eventMouseButtonDown(MouseButton button);

    bool eventMouseButtonUp(MouseButton button);

    const Graphics::IconAtlasInfo &getIconAtlasInfo() const { return _iconAtlasInfo; }

    Vec2 measureText(float fontSize, const char *text) const;

  protected:
    virtual ~Root() { }

  private:
    void traverseControl(Graphics *graphics, Control *control);

    void setGrabbedControl(Control *control, unsigned edges);

    void setHotControl(Control *control, unsigned edges);

    void setFocusedControl(Control *control);

    NVGcontext *_nvgContext;

    MouseMode _mouseMode = MouseMode::Normal;

    MouseCursor _mouseCursor = MouseCursor::Default;

    Vec2 _mousePosition;
    
    unsigned _mouseButtonFlags = 0;

    Control *_grabbedControl = nullptr;

    unsigned _grabbedEdges = Edge::None;

    Rect _grabbedRect;

    Vec2 _grabbedPosition;

    Control *_hotControl = nullptr;

    unsigned _hotEdges = Edge::None;

    Control *_focusedControl = nullptr;

    int _normalFontID;

    int _monospaceFontID;

    Graphics::IconAtlasInfo _iconAtlasInfo;

    Control *_exclusiveControl = nullptr;

    Control::Ptr _exclusiveOldParent;

    Vec2 _exclusiveOldPosition;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
