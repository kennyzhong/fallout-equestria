#include "level/inventory.hpp"
#include "level/level.hpp"
#include <level/objects/locker.hpp>
#include <algorithm>

InventoryObject::InventoryObject(Data data) : Data(&_dataTree), _object("scripts/objects/" + data["script"]["file"].Value())
{
  std::cout << "LOADING OBJECT " << data.Key() << " WITH SCRIPT " << "scripts/objects/" << data["script"]["file"].Value() << endl;
  
  // Duplicate the DataBranch into the InventoryObject
  Duplicate(data);

  // Set the default values
  (*this)["weight"]   = (data["weight"].Nil()) ? "0" : data["weight"].Value();
  (*this)["interactions"]["use"] = "1";
  
  if (data["mode-mouth"].Nil())
    (*this)["mode-mouth"] = "1";
  if (data["mode-magic"].Nil())
    (*this)["mode-magic"] = "1";

  _equiped = false;

  Data script = data["script"];

  if (!(script.Nil()))
  {
    if (script["file"].Value() != "")
    {
      _object.asDefineMethod("CanWeild", "bool CanWeild(Item@, Character@, string, int)");
      _object.asDefineMethod("SetEquiped", "void SetEquiped(Item@, Character@, bool)");
    }
  }

  ForEach(data["actions"], [this](Data action)
  {
    AngelScript::Object hooks(_object.GetContext(), _object.GetModule());
    Data                action_data = (*this)["actions"][action.Key()];

    action_data.Duplicate(action);
    if (action_data["targeted"].Nil())
      action_data["targeted"] = 1;
    if (action["hookUse"].NotNil())
      hooks.asDefineMethod("Use", "string " + action["hookUse"].Value() + "(Item@, Character@)");
    if (action["hookCharacters"].NotNil())
      hooks.asDefineMethod("UseOnCharacter", "string " + action["hookCharacters"].Value() + "(Item@, Character@, Character@");
    if (action["hookDoors"].NotNil())
      hooks.asDefineMethod("UseOnDoor", "string " + action["hookDoors"].Value() + "(Item@, Character@, Door@)");
    if (action["hookOthers"].NotNil())
      hooks.asDefineMethod("UseOnOthers", "string " + action["hookOthers"].Value() + "(Item@, Character@, DynamicObject@");
    if (action["hookWeapon"].NotNil())
      hooks.asDefineMethod("UseAsWeapon", "string " + action["hookWeapon"].Value() + "(Item@, Character@, Character@)");
    if (action["hookHitChances"].NotNil())
      hooks.asDefineMethod("HitChances", "int " + action["hookHitChances"].Value() + "(Item@, Character@, Character@)");
    _actionHooks.push_back(hooks);
  });
}

InventoryObject::~InventoryObject()
{
}

bool InventoryObject::IsGroupableWith(const InventoryObject* other) const
{
  return (GetName() == other->GetName());
}

void InventoryObject::SetEquiped(ObjectCharacter* character, bool set)
{
  if (_equiped != set && _object.IsDefined("SetEquiped"))
  {
    AngelScript::Type<InventoryObject*> this_param(this);
    AngelScript::Type<ObjectCharacter*> character_param(character);
    AngelScript::Type<bool>             set_param(set);

    _object.Call("SetEquiped", 3, &this_param, &character_param, &set_param);
  }
  _equiped = set;
}

bool InventoryObject::CanWeild(ObjectCharacter* character, std::string slot, unsigned char mode)
{
  cout << "CALLING HOOK CAN WEILD" << endl;
  if (_object.IsDefined("CanWeild"))
  {
    AngelScript::Type<InventoryObject*> this_param(this);
    AngelScript::Type<ObjectCharacter*> character_param(character);
    AngelScript::Type<std::string*>     slot_param(&slot);
    AngelScript::Type<unsigned int>     mode_param(mode);

    return (_object.Call("CanWeild", 4, &this_param, &character_param, &slot_param, &mode_param));
  }
  return (false);
}

/*
 * DEPRECATED Needs to disapear asap
 */
bool InventoryObject::CanWeild(ObjectCharacter* character, EquipedMode mode)
{
  return (CanWeild(character, "equiped", mode));
}

InventoryObject::EquipedModel* InventoryObject::CreateEquipedModel(World* world)
{
  if ((*this)["model"].Value() != "")
  {
    EquipedModel* equipedModel = new EquipedModel(world->window, this);

    return (equipedModel);
  }
  return (0);
}

InventoryObject::EquipedModel::EquipedModel(WindowFramework* window, InventoryObject* p) : AnimatedObject(window), object(*p)
{
  Data        anims   = object["animations"];
  std::string texture = object["texture"].Value();

  _modelName   = object["model"].Value();
  for (unsigned short i = 0 ; i < _modelName.size() ; ++i)
  {
    if (_modelName[i] == '/' || _modelName[i] == '\\')
    {
      _modelName.erase(0, i);
      i = 0;
    }
  }
  for (short i = _modelName.size() - 1 ; i >= 0 ; --i)
  {
    if (_modelName[i] == '.')
    {
      _modelName.erase(i);
      break ;
    }
  }
  _modelName   = "items/" + _modelName;

  np = window->load_model(window->get_panda_framework()->get_models(), MODEL_ROOT + object["model"].Value());
  {
    float scale = object["scale"];

    if (object["scale"].NotNil())
      np.set_scale(scale);
  }
  {
    Data offset   = object["offset"];
    Data rotation = object["rotation"];

    if (offset.NotNil())
    {
      LPoint3f position(0, 0, 0);

      if (offset["x"].NotNil())
        position.set_x(offset["x"]);
      if (offset["y"].NotNil())
        position.set_y(offset["y"]);
      if (offset["z"].NotNil())
        position.set_z(offset["z"]);
      np.set_pos(position);
    }
    if (rotation.NotNil())
    {
      LPoint3f position(0, 0, 0);

      rotation.Output();
      if (rotation["x"].NotNil())
        position.set_x(rotation["x"]);
      if (rotation["y"].NotNil())
        position.set_y(rotation["y"]);
      if (rotation["z"].NotNil())
        position.set_z(rotation["z"]);
      np.set_hpr(position);
    }
  }
  if (texture != "")
  {
    Texture*    texfile = TexturePool::load_texture(TEXT_ROOT + texture);

    if (texfile)
      np.set_texture(texfile);
  }
  if (!(anims.Nil()))
  {
    std::for_each(anims.begin(), anims.end(), [this](Data animation)
    {
      if (animation.Key() == "player")
	return ;
      LoadAnimation(animation.Value());
    });
  }
  np.set_name("Equiped" + object.GetName());
}

InventoryObject::EquipedModel::~EquipedModel()
{
  np.detach_node();
}

DynamicObject* InventoryObject::CreateDynamicObject(World* world) const
{
  DynamicObject* object;
  Data           self = *this;

  object               = world->AddDynamicObject("item" + Key(), DynamicObject::Item, self["model"], self["texture"]);
  object->interactions = Interactions::Use;
  object->waypoint     = 0;
  object->type         = DynamicObject::Item;
  return (object);
}

int               InventoryObject::HitSuccessRate(ObjectCharacter* user, ObjectCharacter* target, unsigned char use_type)
{
  AngelScript::Object& hooks = _actionHooks[use_type];

  if (hooks.IsDefined("HitChances"))
  {
    AngelScript::Type<InventoryObject*> this_param(this);
    AngelScript::Type<ObjectCharacter*> user_param(user);
    AngelScript::Type<ObjectCharacter*> target_param(target);

    return (hooks.Call("HitChances", 3, &this_param, &user_param, &target_param));
  }
  return (0);
}

const std::string InventoryObject::UseAsWeapon(ObjectCharacter* user, ObjectCharacter* target, unsigned char useType)
{
  cout << "CALLED USEASWEAPON" << endl;
  return (ExecuteHook("UseAsWeapon", user, target, useType));
}

const std::string InventoryObject::UseOn(ObjectCharacter* user, InstanceDynamicObject* target, unsigned char useType)
{
  cout << "CALLED USEON" << endl;
  ObjectCharacter*     charTarget;
  Lockable*            lockTarget;
  AngelScript::Object& hooks = _actionHooks[useType];

  if (hooks.IsDefined("UseOnCharacter") && (charTarget = target->Get<ObjectCharacter>()) != 0)
    return (ExecuteHook("UseOnCharacter", user, charTarget, useType));
  if (hooks.IsDefined("UseOnDoor")      &&
     (((lockTarget = target->Get<ObjectDoor>()) != 0) ||
       (lockTarget = target->Get<ObjectLocker>()) != 0))
    return (ExecuteHook("UseOnDoor", user, lockTarget, useType));
  if (hooks.IsDefined("UseOnOthers"))
    return (ExecuteHook("UseOnOthers", user, target, useType));
  return ("That does nothing");
}

const std::string InventoryObject::Use(ObjectCharacter* user, unsigned char useType)
{
  return (ExecuteHook("Use", user, (ObjectCharacter*)0, useType));
}

template<class C>
const std::string InventoryObject::ExecuteHook(const std::string& hook, ObjectCharacter* user, C* target, unsigned char useType)
{
  AngelScript::Object& handle = _actionHooks[useType];

  cout << GetName() << " object has methods:" << endl;
  handle.OutputMethods(cout);
  if (handle.IsDefined(hook))
  {
    AngelScript::Type<InventoryObject*> this_param(this);
    AngelScript::Type<ObjectCharacter*> user_param(user);
    AngelScript::Type<C*>               target_param(target);

    if (target == nullptr)
      return (*(std::string*)(handle.Call(hook, 2, &this_param, &user_param)));
    return (*(std::string*)(handle.Call(hook, 3, &this_param, &user_param, &target_param)));
  }
  cout << "Method " << hook << " undefined" << endl;
  return ("That does nothing");
}

/*
 * Inventory
 */
// WARNING TODO: Implement the new equiping system for this LoadInventory overload.
void Inventory::LoadInventory(DynamicObject* object)
{
  ForEach(object->inventory, [this](std::pair<std::string, int> data)
  {
    Data      objectTree = Level::CurrentLevel->GetItems();
    DataTree  objectTmp;
    DataTree* dataTree = DataTree::Factory::StringJSON(data.first);
    {
      Data      data_(dataTree);
      Data*     objectBuilder = new Data(dataTree);

      if (!data_["type"].Nil())
      {
	Data      object(&objectTmp);

	object.Duplicate(objectTree[data_["type"].Value()]);
	if (!(data_["ammo"].Nil()))
	{
	  if (!(data_["ammo"]["ammount"].Nil()))
	    object["ammo"]["ammount"] = data_["ammo"]["ammount"];
	  if (!(data_["ammo"]["current"].Nil()))
	    object["ammo"]["current"] = data_["ammo"]["current"];
	}
	delete objectBuilder;
	objectBuilder = new Data(object);
      }
      else
	objectBuilder->SetKey(data_["Name"]);
      for (int i = 0 ; i < data.second ; ++i)
      {
	InventoryObject* newObject = new InventoryObject(*objectBuilder);

	AddObject(newObject);
      }
      delete objectBuilder;
    }
    delete dataTree;
  });
  InitializeSlots();
}

void Inventory::LoadInventory(Data items)
{
  _content.clear();
  for_each(items.begin(), items.end(), [this](Data item)
  {
    unsigned int     quantity;

    quantity = (item["quantity"].Nil() ? 1 : (unsigned int)item["quantity"]);
    for (unsigned short i = 0 ; i < quantity ; ++i)
    {
      InventoryObject* newObject    = new InventoryObject(item);
      Data             equiped_data = item["equiped-save"];

      AddObject(newObject);
      if (equiped_data.NotNil() && i < equiped_data.Count())
      {
        Data equiped = (*newObject)["equiped"];

        equiped.Duplicate(equiped_data[i]);
        equiped.SetKey("equiped");
      }
      (*newObject)["quantity"].Remove();
      (*newObject)["equiped-save"].Remove();
    }
  });
  InitializeSlots();
}

void Inventory::InitializeSlots(void)
{
  _slots.clear();
  for_each(_content.begin(), _content.end(), [this](InventoryObject* ptr)
  {
    InventoryObject& item = *ptr;
    Data             slot = item["equiped"];

    if (slot.NotNil())
    {  SetEquipedItem(slot["target"], slot["slot"], ptr, (unsigned short)slot["mode"]); }
  });
}

// WARNING TODO: Implement the new equiping system for this SaveInventory overload.
void Inventory::SaveInventory(DynamicObject* object)
{
  Content::iterator it  = _content.begin();
  Content::iterator end = _content.end();

  object->inventory.clear();
  for (; it != end ; ++it)
  {
    Content::iterator groupIt  = _content.begin();
    InventoryObject&  item     = **it;
    bool              ignore   = true;
    int               quantity = 0;

    for (; groupIt != end ; ++groupIt)
    {
      if (groupIt == it && quantity == 0)
	ignore = false;
      if (item.IsGroupableWith(*groupIt))
	quantity++;
    }
    if (quantity == 0) quantity = 1;
    if (!ignore)
    {
      std::string str;

      item["Name"] = item.Key();
      DataTree::Writers::StringJSON(item, str);
      object->inventory.push_back(std::pair<std::string, int>(str, quantity));
    }
  }
}

void Inventory::SaveInventory(Data items)
{
  Content::iterator it  = _content.begin();
  Content::iterator end = _content.end();

  for (; it != end ; ++it)
  {
    Content::iterator groupIt  = _content.begin();
    InventoryObject&  item     = **it;
    bool              ignore   = true;
    int               quantity = 0;
    std::vector<Data> equip_data;

    for (; groupIt != end ; ++groupIt)
    {
      if (groupIt == it && quantity == 0)
	ignore = false;
      if (item.IsGroupableWith(*groupIt))
	quantity++;
      if (item["equiped"].NotNil())
        equip_data.push_back(item["equiped"]);
    }
    if (quantity == 0) quantity = 1;
    if (!ignore)
    {
      for (unsigned int i = 0 ; i < equip_data.size() ; ++i)
      {
        item["equiped-save"]["new"].Duplicate(equip_data[i]);
        item["equiped-save"]["new"].SetKey("");
      }
      item["quantity"] = quantity;
      items[item.Key()].Duplicate(item);
      item["quantity"].Remove();
      item["equiped-save"].Remove();
    }
  }
}

void Inventory::AddObject(InventoryObject* toAdd)
{
  Data weight = (*toAdd)["weight"];

  if (!(weight.Nil()))
    _currentWeight += (unsigned short)((*toAdd)["weight"]);
  (*toAdd)["equiped"].Remove();
  _content.push_back(toAdd);
  ContentChanged.Emit();
}

void Inventory::DelObject(InventoryObject* toDel)
{
  Content::iterator it = std::find(_content.begin(), _content.end(), toDel);

  if (it != _content.end())
  {
    Data weight = (*toDel)["weight"];

    if (!(weight.Nil()))
      _currentWeight -= (unsigned short)((*toDel)["weight"]);
    (*toDel)["equiped"].Remove();
    _content.erase(it);
    ContentChanged.Emit();
  }
}

bool Inventory::IncludesObject(InventoryObject* obj) const
{
  Content::const_iterator it = std::find(_content.begin(), _content.end(), obj);

  return (it != _content.end());
}

int              Inventory::GetObjectIterator(InventoryObject* object) const
{
  auto it = _content.begin();
  auto end = _content.end();

  for (int i = 0 ; it != end ; ++it, ++i)
  {
    InventoryObject* comp = *it;

    if (object == comp)
      return (i);
  }
  return (-1);
}

InventoryObject* Inventory::GetObject(const std::string& name)
{
  Content::iterator it  = _content.begin();
  Content::iterator end = _content.end();
  
  for (; it != end ; ++it)
  {
    if ((*(*it)).GetName() == name)
      return (*it);
  }
  return (0);
}

unsigned short Inventory::ContainsHowMany(const std::string& name) const
{
  Content::const_iterator it    = _content.begin();
  Content::const_iterator end   = _content.end();
  unsigned short          count = 0;

  for (; it != end ; ++it)
  {
    if ((*(*it)).GetName() == name)
      count++;
  }
  return (count);
}

bool Inventory::CanCarry(InventoryObject* object, unsigned short quantity)
{
  return ((_capacity >= _currentWeight + (unsigned short)((*object)["weight"]) * quantity) || (_capacity == 0));
}

//
// Slots
//

// This here contraption allows to duplicate a single method to generate a const
// and a non-const getter at the same time. Might be worth moving into Boots.
#define DECLARE_GETTER(return_type, name, parameters, code) \
const return_type name parameters const \
code \
return_type name parameters \
code

Inventory::Slot& Inventory::GetItemSlot(const std::string& type_slot, unsigned int slot)
{
  auto it = std::find(_slots.begin(), _slots.end(), type_slot);

  if (it == _slots.end())
  {
    _slots.push_back(Slots(type_slot));
    return (GetItemSlot(type_slot, slot));
  }
  /*if (slot >= it->size())
    it->resize(slot + 1);*/
  {
    Inventory::Slot& obj = (*it)[slot];

    if (obj.object)
      std::cout << "Lick My Balls" << std::endl;
    return (obj);
  }
}

const Inventory::Slot& Inventory::GetConstItemSlot(const std::string& type_slot, unsigned int slot) const
{
  auto it = std::find(_slots.begin(), _slots.end(), type_slot);

  if (it == _slots.end())
    throw 0; // TODO find some kind of exception for that
  return ((*it)[slot]);
}

InventoryObject*       Inventory::GetEquipedItem(const std::string& type_slot, unsigned int slot)
{
  try
  {
    return (GetConstItemSlot(type_slot, slot).object);
  }
  catch (...)
  {
    return (0);
  }
}

unsigned char          Inventory::GetEquipedMode(const std::string& type_slot, unsigned int slot) const
{
  try
  {
    return (GetConstItemSlot(type_slot, slot).mode);
  }
  catch (...)
  {
    return (0);
  }
}

bool                   Inventory::SlotHasEquipedItem(const std::string& type_slot, unsigned int slot) const
{
  try
  {
    return (!(GetConstItemSlot(type_slot, slot).empty));
  }
  catch (...)
  {
    return (false);
  }
}

void                   Inventory::SetEquipedItem(const std::string& type_slot, unsigned int it_slot, InventoryObject* object, unsigned char equip_mode)
{
  Inventory::Slot& slot = GetItemSlot(type_slot, it_slot);

  if (slot.object)
  {
    Data data = *slot.object;

    data["equiped"].Remove();
    UnequipedItem.Emit(type_slot, it_slot, slot.object);
  }
  if (object)
  {
    Data data = *object;

    data["equiped"]["target"] = type_slot;
    data["equiped"]["slot"]   = it_slot;
    data["equiped"]["mode"]   = (unsigned short)equip_mode;
  }
  slot.mode   = equip_mode;
  slot.object = object;
  slot.empty  = object == 0;
  EquipedItem.Emit(type_slot, it_slot, object);
}
