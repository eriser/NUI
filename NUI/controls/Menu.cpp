#include "Menu.h"
#include "Root.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
MenuItem::MenuItem(bool inMenuBar, Control *parent, const std::string &text, int icon)
  : Control(parent, text, Docking::Top, icon)
  , _inMenuBar(inMenuBar)
  , _isSeparator(text == "-")
{
  _rect.height = _isSeparator ? Graphics::Style::DefaultControlHeight / 4 : Graphics::Style::DefaultControlHeight;

  setMargins(0);
  addFlags(CanFocus);
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::draw(Graphics *graphics)
{
  int offset = (_state & (State::Selected | State::Down)) != 0 ? 1 : 0;
  
  if (!_isSeparator)
    graphics->drawBevel(0, 0, _rect.width, _rect.height, _state, Graphics::Bevel::MenuItem);

  if (_inMenuBar)
  {
    graphics->drawText(offset + _rect.width / 2, offset + _rect.height / 2, _text.c_str(), true);
  }
  else
  {
    if (_isSeparator)
    {
      graphics->drawBevel(0, _rect.height / 2, _rect.width, 1, 0, Graphics::Bevel::MenuBar);
    }
    else
    {
      graphics->drawText(offset + graphics->iconAtlasInfo.iconSize.x + graphics->iconAtlasInfo.iconMargin * 2, offset + _rect.height / 2, _text.c_str(), true, Graphics::HAlign::Left);
      graphics->drawIcon(graphics->iconAtlasInfo.iconSize.x / 2 + graphics->iconAtlasInfo.iconMargin, _rect.height / 2, _icon);
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::setText(const std::string &text)
{
  if (_text != text)
  {
    _isSeparator = (text == "-");
    _rect.height = _isSeparator ? Graphics::Style::DefaultControlHeight / 4 : Graphics::Style::DefaultControlHeight;
  }

  Super::setText(text);
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::setSubMenu(Menu *menu)
{
  if (_subMenu != menu)
  {
    if (_subMenu)
      _subMenu->setParent(nullptr);

    _subMenu = menu;

    if (_subMenu)
      _subMenu->setParent(this);
  }
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::openSubMenu(bool open)
{
  if (_subMenu)
  {
    if (open && !_subMenu->isVisible())
    {
      if (_inMenuBar)
        _subMenu->setPosition(-_padding.left, _rect.height - _padding.top);

      _subMenu->show();
      addState(State::Selected);
      getRoot()->setExclusiveControl(_subMenu);
    }
    else if (!open && _subMenu->isVisible())
    {
      getRoot()->setExclusiveControl(nullptr);
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::toggleSubMenu()
{
  if (_subMenu)
    openSubMenu(!_subMenu->isVisible());
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::processEvent(Event &e, bool propagateUp, bool propagateDown)
{
  switch (e.type)
  {
    case Event::Type::ExclusivityChanged:
    {
      if (e.sender == _subMenu && !(e.sender->getState() & State::Exclusive))
      {
        _subMenu->show(false);
        removeState(State::Selected);
      }
    }
    break;
  }

  Super::processEvent(e, propagateUp, propagateDown);
}

//---------------------------------------------------------------------------------------------------------------------
void MenuItem::autoSize(bool recursive)
{
  Vec2 textSize = getRoot()->measureText(13.0f, _text.c_str());
  auto &iai = getRoot()->getIconAtlasInfo();

  if (_inMenuBar)
    setMinimumSize(textSize.x + _padding.getHorizontal() * 2, _rect.height);
  else
    setMinimumSize(textSize.x + iai.iconSize.x + iai.iconMargin * 2 + textSize.x / 3, _rect.height);

  setSize(_minimumSize.x, _minimumSize.y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
void Menu::preDraw(Graphics *graphics)
{
  graphics->drawShadow(0, 8, _rect.width, _rect.height - 8, 6, 0.35f);
}

//---------------------------------------------------------------------------------------------------------------------
void Menu::draw(Graphics *graphics)
{
  graphics->drawBevel(0, 0, _rect.width, _rect.height, 0, Graphics::Bevel::Menu);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
void MenuBar::draw(Graphics *graphics)
{
  graphics->drawBevel(0, 0, _rect.width, _rect.height, 0, Graphics::Bevel::MenuBar);
}

//---------------------------------------------------------------------------------------------------------------------
void MenuBar::processEvent(Event &e, bool propagateUp, bool propagateDown)
{
  switch (e.type)
  {
    case Event::Type::MouseButton:
    {
      if (e.sender->getParent() == this && e.mouseButton.down && e.mouseButton.button == MouseButton::Left)
      {
        MenuItem *mi = reinterpret_cast<MenuItem *>(e.sender.get());
        mi->openSubMenu();
        _isOpened = true;
      }
    }
    break;

    case Event::Type::ExclusivityChanged:
    {
      if (e.sender->hasParent(this))
      {
        if (!(e.sender->getState() & State::Exclusive) && _isOpened)
          _isOpened = false;
        else if ((e.sender->getState() & State::Exclusive) && !_isOpened)
          _isOpened = true;
      }
    }
    break;

    case Event::Type::HotChanged:
    {
      if (e.sender->getParent() == this && (e.sender->getState() & State::Hot) && _isOpened)
      {
        MenuItem *mi = reinterpret_cast<MenuItem *>(e.sender.get());
        mi->openSubMenu();
        _isOpened = true;
      }
    }
    break;
  }

  Super::processEvent(e, propagateUp, propagateDown);
}

}
