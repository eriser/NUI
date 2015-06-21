#pragma once

#include "../Control.h"

namespace nui {
  
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TextBox : public Control
{
  public:
    NUI_CONTROL(TextBox, Control);

    explicit TextBox(Control *parent = nullptr, const std::string &text = std::string(), Docking docking = Docking::None);

    void draw(Graphics *graphics) override;

    void processEvent(Event &e, bool propagateUp /* = true */, bool propagateDown /* = false */) override;

    void setText(const std::string &text) override;

    void loadTextFromFile(const std::string &fileName);

    void setMonospace(bool set = true) { _monospace = set; }

    bool getMonospace() const { return _monospace; }
    
    void setMultiline(bool set = true);

    bool getMultiline() const { return _multiline; }

    size_t getCursorOffset() const;

  private:
    void insertChar(int ch);

    void deleteChar();

    void rebuildLineInfos();

    void updatePositions();

    void updateScrollArea();

    bool _multiline = false;

    bool _monospace = false;

    bool _hasSelection = false;

    Vec2 _cursor;

    Vec2 _cursorDrawPos;

    Vec2 _selectionStart;

    Vec2 _selectionEnd;

    Vec2 _selectionDrawStart;

    Vec2 _selectionDrawEnd;

    struct LineInfo
    {
      size_t index;
      size_t offset;
      size_t length;
    };

    std::vector<LineInfo> _lineInfos;

    Vec2 _scroll;

    int _lineHeight = 0;

    size_t _longestLine = static_cast<size_t>(-1);

    int _longestLineWidth = 0;

    nui::Ptr<ScrollBar> _hScroll;

    nui::Ptr<ScrollBar> _vScroll;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
