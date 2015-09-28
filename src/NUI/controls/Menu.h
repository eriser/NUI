#pragma once

#include "../Control.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MenuItem : public Control
{
  friend class Menu;
  friend class MenuBar;

  public:
    NUI_CONTROL(MenuItem, Control);

    void draw(Graphics *graphics) override;

    void autoSize(bool recursive) override;

    void setText(const std::string &text) override;

    void setSubMenu(Menu *menu);

    Menu *getSubMenu() const { return _subMenu; }

    void openSubMenu(bool open = true);

    void toggleSubMenu();

    void processEvent(Event &e, bool propagateUp, bool propagateDown) override;

    bool isSeparator() const { return _isSeparator; }

  protected:
    explicit MenuItem(bool inMenuBar, Control *parent = nullptr, const std::string &text = std::string(), int icon = -1);

  private:
    bool _inMenuBar;

    bool _isSeparator;

    nui::Ptr<Menu> _subMenu;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Menu : public Control
{
  public:
    NUI_CONTROL(Menu, Control);

    explicit Menu(Control *parent = nullptr)
      : Control(parent)
    {
      addFlags(AlwaysBringToFront | PreDraw);
      removeFlags(Visible);
      setPadding(2);
    }

    void preDraw(Graphics *graphics) override;

    void draw(Graphics *graphics) override;

    MenuItem *addItem(const std::string &text, int icon)
    {
      MenuItem *item = new MenuItem(false, this, text, icon);

      _rect.width = 0;
      _rect.height = 0;
      autoSize(true);

      return item;
    }

    MenuItem *addItem(const std::string &text) { return addItem(text, -1); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MenuBar : public Control
{
  public:
    NUI_CONTROL(MenuBar, Control);

    explicit MenuBar(Control *parent = nullptr)
      : Control(parent, std::string(), Docking::Top)
    {
      setPadding(0);
    }

    void draw(Graphics *graphics) override;

    Menu *addMenu(const std::string &text)
    {
      MenuItem *item = new MenuItem(true, this, text);
      item->setDocking(Docking::Left);

      _rect.width = 0;
      _rect.height = 0;
      autoSize(true);

      Menu *menu = new Menu(item);
      item->setSubMenu(menu);

      return menu;
    }

    void processEvent(Event &e, bool propagateUp /* = true */, bool propagateDown /* = false */) override;

  private:
    bool _isOpened = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
