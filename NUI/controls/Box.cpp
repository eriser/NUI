#include "Box.h"

namespace nui {

//---------------------------------------------------------------------------------------------------------------------
Box::Box(Control *parent, Docking docking)
  : Control(parent, std::string(), docking, -1)
{
  removeFlags(Draw);
  setPadding(0);
}

}
