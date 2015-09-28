#include "Window.h"

namespace nui {
  
//---------------------------------------------------------------------------------------------------------------------
Window::Window(Control *parent, const std::string &text, int icon)
  : Control(parent, text, Docking::None, icon)
{
  addFlags(CanFocus | CanMove | CanResize | CanMaximize | CanMinimize | AlwaysBringToFront | PreserveDeepFocus | ParentClip | PreDraw);
  setTitleHeight(Graphics::Style::DefaultTitleHeight);
  setSize(Graphics::Style::DefaultWindowWidth, Graphics::Style::DefaultWindowHeight);
  setMinimumSize(Graphics::Style::DefaultTitleHeight * 2, Graphics::Style::DefaultTitleHeight);

  _buttonClose = new Button(this, std::string(), Docking::None, NUI_ICON_WINDOW_CLOSE);
  _buttonClose->addFlags(ParentClip);
  _buttonClose->removeFlags(CanFocus);
  _buttonClose->setSize(Graphics::Style::DefaultTitleHeight - 4, Graphics::Style::DefaultTitleHeight - 4);
  _buttonClose->setPosition(_rect.width - _padding.right - _buttonClose->getWidth() - 2, -_padding.top - _titleHeight + 2);
  _buttonClose->setAnchors(Edge::TopRight);
}

//---------------------------------------------------------------------------------------------------------------------
void Window::preDraw(Graphics *graphics)
{
  graphics->drawShadow(0, 8, _rect.width, _rect.height - 8, 6, 0.35f);
}

//---------------------------------------------------------------------------------------------------------------------
void Window::draw(Graphics *graphics)
{
  graphics->drawBevel(0, 0, _rect.width, _rect.height, _state, Graphics::Bevel::Window);
  graphics->drawBevel(0, 0, _rect.width, _titleHeight, _state, Graphics::Bevel::Title);
  graphics->drawText(_rect.width / 2, _titleHeight / 2, _text.c_str(), true);
  graphics->drawIcon(_titleHeight / 2, _titleHeight / 2, _icon);
}

}
