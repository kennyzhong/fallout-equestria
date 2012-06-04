#ifndef  INTERACT_MENU_HPP
# define INTERACT_MENU_HPP

# include <panda3d/pandaFramework.h>
# include <panda3d/pgButton.h>

class InstanceDynamicObject;

class InteractMenu
{
public:
  InteractMenu(WindowFramework* window, InstanceDynamicObject& object);
  ~InteractMenu();

private:
  static void ButtonClicked(const Event*, void* data);

  struct ButtonStorage
  {
    ButtonStorage(PGButton* ptr, NodePath node) : button(ptr), node(node) {}
    
    PT(PGButton) button;
    NodePath     node;
  };
  
  typedef std::list<ButtonStorage> Buttons;

  Buttons _buttons;
};

#endif