#pragma once

#include "../Control.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Root : public Control
{
  public:
    NUI_CONTROL(Root, Control);

    explicit Root(NVGcontext *nvgCtx);

    void tick(double time, double delta) override;

    enum class MouseMode
    {
      Normal = 0,
      Down,
      Moving,
      Resizing
    };

    struct MouseState
    {
      MouseMode mode = MouseMode::Normal;
      Vec2 position;
      unsigned buttonFlags = 0;
      MouseCursor cursor = MouseCursor::Default;
    };

    const MouseState &getMouseState() const { return _mouseState; }

    struct KeyboardState
    {
      static const size_t numPressedKeys = static_cast<size_t>(Key::Last);
      bool pressedKeys[numPressedKeys];
      bool pressedCharacters[256];
      unsigned modKeyFlags = 0;

      void updateModKeyFlags()
      {
        modKeyFlags = 0;

        if (pressedKeys[static_cast<size_t>(Key::ShiftLeft)])
          modKeyFlags |= ModKey::ShiftLeft;
        if (pressedKeys[static_cast<size_t>(Key::ShiftRight)])
          modKeyFlags |= ModKey::ShiftRight;

        if (pressedKeys[static_cast<size_t>(Key::ControlLeft)])
          modKeyFlags |= ModKey::ControlLeft;
        if (pressedKeys[static_cast<size_t>(Key::ControlRight)])
          modKeyFlags |= ModKey::ControlRight;

        if (pressedKeys[static_cast<size_t>(Key::AltLeft)])
          modKeyFlags |= ModKey::AltLeft;
        if (pressedKeys[static_cast<size_t>(Key::AltRight)])
          modKeyFlags |= ModKey::AltRight;
      }

      KeyboardState()
      {
        for (size_t i = 0; i < numPressedKeys; ++i)
          pressedKeys[i] = false;

        for (size_t i = 0; i < 256; ++i)
          pressedCharacters[i] = false;
      }
    };

    const KeyboardState &getKeyboardState() const { return _keyboardState; }

    Control *getGrabbedControl() const { return _grabbedControl; }

    Control *getHotControl() const { return _hotControl; }

    Control *getFocusedControl() const { return _focusedControl; }

    void setExclusiveControl(Control *control);

    Control *getExclusiveControl() const { return _exclusiveControl; }

    void draw();

    bool eventMouseMotion(int x, int y);

    bool eventMouseButtonDown(MouseButton button);

    bool eventMouseButtonUp(MouseButton button);

    bool eventKeyDown(Key key, int character = 0);

    bool eventKeyUp(Key key, int character = 0);

    bool isTextInputRequired() const;

    const Graphics::IconAtlasInfo &getIconAtlasInfo() const { return _iconAtlasInfo; }

    Vec2 measureText(int fontSize, const char *text, const char *endText, bool monospace = false) const;

    Vec2 measureText(int fontSize, const char *text, bool monospace = false) const
    {
      return measureText(fontSize, text, nullptr, monospace);
    }

    int measureIndex(int fontSize, const char *text, int x, int *charX, bool monospace = false) const;

  protected:
    virtual ~Root()
    {
      delete [] _nvgGlyphPositionBuffer;
    }

  private:
    void traverseControl(Graphics *graphics, Control *control);

    void setGrabbedControl(Control *control, unsigned edges);

    void setHotControl(Control *control, unsigned edges);

    void setFocusedControl(Control *control);

    NVGcontext *_nvgContext;

    NVGglyphPosition *_nvgGlyphPositionBuffer;

    MouseState _mouseState;

    KeyboardState _keyboardState;

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

    double _cursorBlinker = 0.0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
