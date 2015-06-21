#include "TextBox.h"
#include "Root.h"
#include "ScrollBar.h"

#include <fstream>

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
TextBox::TextBox(Control *parent, const std::string &text, Docking docking)
  : Control(parent, text, docking)
{
  addFlags(CanFocus | NeedsTextInput);
  rebuildLineInfos();

  _hScroll = new ScrollBar(this);
  _hScroll->show(false);
  _hScroll->removeFlags(CanFocus);
  _hScroll->setSize(_rect.width, _hScroll->getHeight());
  _hScroll->setPosition(0, _rect.height - _hScroll->getHeight() - _padding.top);
  _hScroll->setAnchors(Edge::Left | Edge::Bottom | Edge::Right);

  _vScroll = new ScrollBar(this);
  _vScroll->show(false);
  _vScroll->removeFlags(CanFocus);
  _vScroll->setSize(_vScroll->getWidth(), _rect.height - _padding.getVertical());
  _vScroll->setPosition(_rect.width - _vScroll->getWidth() - _padding.getHorizontal(), 0);
  _vScroll->setAnchors(Edge::Top | Edge::Right | Edge::Bottom);

  updateScrollArea();
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::draw(Graphics *graphics)
{
  int reducedWidth = _vScroll->isVisible() ? _vScroll->getWidth() + 1 : 0;
  int reducedHeight = _hScroll->isVisible() ? _hScroll->getHeight() + 1 : 0;

  graphics->drawBevel(0, 0, _rect.width, _rect.height, _state, Graphics::Bevel::TextBox);

  if (!_lineHeight)
  {
    _lineHeight = static_cast<int>(graphics->state.style->textSize);
    updateScrollArea();
  }

  graphics->pushState();
  graphics->intersectScissor(_padding.left, _padding.top, _rect.width - reducedWidth - _padding.getHorizontal(), _rect.height - reducedHeight - _padding.getVertical());

  if (_multiline)
  {
    int visibleLines = 1 + (_rect.height - _padding.getVertical() / _lineHeight);

    size_t startLine = _scroll.y / _lineHeight;
    size_t endLine = startLine + visibleLines;
    size_t focusLine = (_state & (State::Focused | State::DeepFocused)) != 0 ? _cursor.y : static_cast<size_t>(-1);

    int y = _padding.top + 6 - _scroll.y % _lineHeight;
    for (size_t i = startLine, S = _lineInfos.size(); i < endLine && i < S; ++i)
    {
      const LineInfo &l = _lineInfos[i];
      const char *text = _text.c_str() + l.offset;
      const char *end = text + l.length;

      graphics->drawText(_padding.left, y, text, end, false, Graphics::HAlign::Left, Graphics::VAlign::Middle, _monospace);

      if (i == focusLine)
        graphics->drawTextCursor(_padding.left + _cursorDrawPos.x, y);

      y += 13;
    }
  }
  else
  {
    graphics->drawText(_padding.left, _rect.height / 2, _text.c_str(), false, Graphics::HAlign::Left, Graphics::VAlign::Middle, _monospace);

    if (_state & State::Focused)
      graphics->drawTextCursor(_padding.left + _cursorDrawPos.x, _rect.height / 2);
  }

  graphics->popState();
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::processEvent(Event &e, bool propagateUp, bool propagateDown)
{
  switch (e.type)
  {
    case Event::Type::ValueChanged:
    {
      if (e.sender == _vScroll)
        _scroll.y = static_cast<int>(_vScroll->getValue());
      else if (e.sender == _hScroll)
        _scroll.x = static_cast<int>(_hScroll->getValue());
    }
    break;

    case Event::Type::SizeChanged:
    {
      if (e.sender == this)
        updateScrollArea();
    }
    break;

    case Event::Type::Key:
    {
      if (e.key.down)
      {
        switch (e.key.key)
        {
          case Key::Backspace:
          case Key::Left:
          {
            bool moved = false;

            if (_cursor.x > 0)
            {
              --_cursor.x;
              moved = true;
            }
            else if (_cursor.y > 0)
            {
              --_cursor.y;
              _cursor.x = _lineInfos[_cursor.y].length;
              moved = true;
            }

            if (moved && e.key.key == Key::Backspace)
              deleteChar();
          }
          break;

          case Key::Right:
          {
            if (_cursor.x < static_cast<int>(_lineInfos[_cursor.y].length))
              ++_cursor.x;
            else if (_cursor.y < static_cast<int>(_lineInfos.size()) - 1)
            {
              ++_cursor.y;
              _cursor.x = 0;
            }
          }
          break;

          case Key::Up:
          {
            if (_cursor.y > 0)
            {
              --_cursor.y;
              _cursor.x = minimum(_cursor.x, static_cast<int>(_lineInfos[_cursor.y].length));
            }
          }
          break;

          case Key::Down:
          {
            if (_cursor.y < static_cast<int>(_lineInfos.size()) - 1)
            {
              ++_cursor.y;
              _cursor.x = minimum(_cursor.x, static_cast<int>(_lineInfos[_cursor.y].length));
            }
          }
          break;

          case Key::Home:
          {
            _cursor.x = 0;
          }
          break;

          case Key::End:
          {
            _cursor.x = static_cast<int>(_lineInfos[_cursor.y].length);
          }
          break;

          case Key::Delete:
          {
            deleteChar();
          }
          break;

          case Key::Enter:
          {
            insertChar('\n');
          }
          break;

          case Key::Character:
          {
            insertChar(e.key.character);
          }
          break;

          default:
            break;
        }
      }

      updatePositions();
      updateScrollArea();
    }
    break;

    case Event::Type::MouseButton:
    {
      const Root *root = getRoot();
      if (root && e.mouseButton.down && (e.mouseButton.button == MouseButton::Left || e.mouseButton.button == MouseButton::Right))
      {
        if (_multiline)
        {
          _cursor.y = 0;
        }
        else
        {
          _cursor.x = root->measureIndex(_lineHeight, _text.c_str(), e.mouseButton.x - _padding.left, &_cursorDrawPos.x, _monospace);
          _cursor.y = 0;
        }
      }
    }
    break;

    default:
      break;
  }

  Super::processEvent(e, propagateUp, propagateDown);
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::setText(const std::string &text)
{
  bool rebuild = (_text != text);
  Super::setText(text);

  if (rebuild)
  {
    rebuildLineInfos();
    updateScrollArea();
  }
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::loadTextFromFile(const std::string &fileName)
{
  std::ifstream ifs(fileName.c_str(), std::ios_base::binary);
  
  ifs.seekg(0, std::ios_base::end);
  size_t size = static_cast<size_t>(ifs.tellg());
  ifs.seekg(0, std::ios_base::beg);

  char *buff = new char[size + 1];
  buff[size] = 0;
  ifs.read(buff, size);

  setText(std::string(buff));

  delete[] buff;
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::setMultiline(bool set)
{
  if (_multiline != set)
  {
    _multiline = set;
    rebuildLineInfos();
    updateScrollArea();
  }
}

//---------------------------------------------------------------------------------------------------------------------
size_t TextBox::getCursorOffset() const
{
  return _lineInfos[_cursor.y].offset + _cursor.x;
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::insertChar(int ch)
{
  if (ch == '\n')
  {
    if (!_multiline)
      return;

    size_t off = getCursorOffset();
    _text.insert(_text.begin() + off, ch);

    _lineInfos.insert(_lineInfos.begin() + _cursor.y + 1, LineInfo());

    LineInfo &li = _lineInfos[_cursor.y];
    LineInfo &nextLI = _lineInfos[_cursor.y + 1];
    
    nextLI.offset = off;
    nextLI.length = li.length - static_cast<size_t>(_cursor.x);

    li.length = _cursor.x;

    for (size_t i = _cursor.y + 1, S = _lineInfos.size(); i < S; ++i)
    {
      _lineInfos[i].index = i;
      ++_lineInfos[i].offset;
    }

    _cursor.x = 0;
    ++_cursor.y;
  }
  else
  {
    size_t off = getCursorOffset();
    _text.insert(_text.begin() + off, ch);

    ++_lineInfos[_cursor.y].length;
    for (size_t i = _cursor.y + 1, S = _lineInfos.size(); i < S; ++i)
      ++_lineInfos[i].offset;

    ++_cursor.x;
  }
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::deleteChar()
{
  size_t off = getCursorOffset();
  size_t charsToDelete = 1;

  if (_cursor.x < static_cast<int>(_lineInfos[_cursor.y].length))
  {
    _text.erase(_text.begin() + off);
    --_lineInfos[_cursor.y].length;
  }
  else if (_cursor.y < (static_cast<int>(_lineInfos.size()) - 1))
  {
    if (_text[off] == '\r')
      ++charsToDelete;
    
    for (size_t i = 0; i < charsToDelete; ++i)
      _text.erase(_text.begin() + off);

    _lineInfos[_cursor.y].length += _lineInfos[_cursor.y + 1].length;
    _lineInfos.erase(_lineInfos.begin() + _cursor.y + 1);
  }

  for (size_t i = _cursor.y + 1, S = _lineInfos.size(); i < S; ++i)
    --_lineInfos[i].offset;

  updatePositions();
  updateScrollArea();
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::rebuildLineInfos()
{
  _lineInfos.clear();

  if (_multiline)
  {
    splitString(_text, "\n", [&](const std::string &str, size_t index, size_t offset, size_t length)
    {
      LineInfo li;
      li.index = index;
      li.offset = offset;
      li.length = length;

      _lineInfos.push_back(li);
    });
  }
  else
  {
    LineInfo li;
    li.offset = 0;
    li.length = _text.length();
    li.index = 0;

    _lineInfos.push_back(li);
  }

  if (_lineInfos.empty())
    _lineInfos.push_back(LineInfo());

  _longestLine = static_cast<size_t>(-1);
  _longestLineWidth = -1;

  Root::Ptr root = getRoot();
  Graphics::Style::Ptr style = getStyle(true);

  if (style && root)
  {
    for (auto &li : _lineInfos)
    {
      int lineWidth = root->measureText(style->textSize, _text.c_str() + li.offset, _text.c_str() + li.offset + li.length, _monospace).x;
      if (lineWidth > _longestLineWidth)
      {
        _longestLine = li.index;
        _longestLineWidth = lineWidth;
      }
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::updatePositions()
{
  const Root *root = getRoot();

  if (root)
  {
    const Graphics::Style *style = _style ? _style : root->getStyle();
    const char *text = _text.c_str() + _lineInfos[_cursor.y].offset;
    Vec2 size = root->measureText(style->textSize, text, text + _cursor.x + 1, _monospace);

    _cursorDrawPos.x = size.x;
  }
}

//---------------------------------------------------------------------------------------------------------------------
void TextBox::updateScrollArea()
{
  if (!_lineHeight)
    return;
  
  int availableWidth = _rect.width;
  int totalHeight = _multiline ? _lineInfos.size() * _lineHeight : _lineHeight;

  if (totalHeight > _rect.height - _padding.getVertical())
  {
    _vScroll->show(true);
    _vScroll->setMaximum(totalHeight);
    _vScroll->setSize(_vScroll->getWidth(), _rect.height - _padding.getVertical());
    availableWidth -= _vScroll->getWidth() + 1;
  }
  else
  {
    _vScroll->show(false);
  }

  if (_longestLineWidth > availableWidth - _padding.getHorizontal())
  {
    _hScroll->show(true);
    _hScroll->setMaximum(_longestLineWidth);
    _hScroll->setSize(availableWidth, _hScroll->getHeight());

    if (_vScroll->isVisible())
      _vScroll->setSize(_vScroll->getWidth(), _rect.height - _padding.getVertical() - _hScroll->getHeight());
  }
  else
  {
    _hScroll->show(false);
  }
}

}
