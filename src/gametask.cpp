#include "gametask.hpp"
#include <iostream>

using namespace std;

GameTask* GameTask::CurrentGameTask = 0;

Buff::~Buff()
{
  if (_task)
    _tm.DelTask(_task);
}

Buff::Buff(const string& name, StatController* stats, Data data, TimeManager& tm) : _tm(tm)
{
  unsigned short refresh_time = 1;

  _buff.Duplicate(data);
  InitScripts();
  _target_name  = name;
  _target_stats = stats;
  //_looping    = _buff["loop"].Value() == "1";
  _task       = tm.AddTask(TASK_LVL_WORLDMAP, _buff["loop"].Value() == "1", data["duration"].Nil() ? 1 : (int)data["duration"]);
  _task->Interval.Connect(*this, &Buff::Refresh);
}

Buff::Buff(Utils::Packet& packet, TimeManager& tm, function<StatController* (const string&)> get_controller) : _tm(tm)
{
  string    data_json;
  DataTree* data_tree;

  packet >> _target_name;
  packet >> data_json;
  data_tree = DataTree::Factory::StringJSON(data_json);
  if (data_tree)
  {
    _buff.Duplicate(data_tree);
    delete data_tree;
  }
  _target_stats = get_controller(_target_name);
  _looping      = _buff["loop"].Value() == "1";
  _task         = _tm.AddTask(TASK_LVL_WORLDMAP, _looping, 0);
  packet >> _task->lastS >> _task->lastM >> _task->lastH >> _task->lastD >> _task->lastMo >> _task->lastY;
  packet >> _task->timeS >> _task->timeM >> _task->timeH >> _task->timeD >> _task->timeMo >> _task->timeY;
  _task->Interval.Connect(*this, &Buff::Refresh);
  InitScripts();
}

void Buff::Save(Utils::Packet& packet)
{
  string json;
  Data   safeDataCopy;

  safeDataCopy.Duplicate(_buff);
  DataTree::Writers::StringJSON(safeDataCopy, json);
  packet << _target_name << json;
  packet << _task->lastS << _task->lastM << _task->lastH << _task->lastD << _task->lastMo << _task->lastY;
  packet << _task->timeS << _task->timeM << _task->timeH << _task->timeD << _task->timeMo << _task->timeY;
}

void Buff::InitScripts(void)
{
  Data           data_script  = _buff["script"];
  string         script_src, script_func, script_decl;

  if (data_script.Nil())
    return ;
  script_src  = data_script["src"].Value();
  script_func = data_script["hook"].Value();
  script_decl = "bool " + script_func + "(Data, Data)";
  _context    = Script::Engine::Get()->CreateContext();
  _module     = Script::ModuleManager::Require(script_src, "scripts/buffs/" + script_src);
  if (!_module || !_context)
    return ;
  _refresh    = _module->GetFunctionByDecl(script_decl.c_str());  
  if (_refresh == 0)
    cout << "[Fatal Error] Buff refresh function '" << script_decl << "' doesn't exist" << endl;
  cout << "Successfully loaded scripts" << endl;
}

void Buff::Refresh(void)
{
  bool  keep_going = false;
  bool  looping    = _buff["loop"].Value() == "1";

  cout << "Refreshing Buff" << endl;
  if (_target_stats)
  {
    cout << "Executing scripts" << endl;
    Data  stats        = _target_stats->Model().GetAll();

    InitScripts();
    _context->Prepare(_refresh);
    _context->SetArgObject(0, &_buff);
    _context->SetArgObject(1, &stats);
    _context->Execute();
    keep_going = _context->GetReturnByte();
    cout << "Returned " << (keep_going ? "true" : "false") << endl;
    cout << "Looping is " << (looping ? "true" : "false") << endl;
  }
  if (!keep_going || !looping)
  {
    _buff.Output();
    cout << "Buff is over" << endl;
    Over.Emit(this);
  }
}

void BuffManager::Cleanup(Buff* to_del)
{
  auto it = find(buffs.begin(), buffs.end(), to_del);
  
  if (it != buffs.end())
    buffs.erase(it);
  garbage.push_back(to_del);
}

void BuffManager::CollectGarbage(void)
{
  for_each(garbage.begin(), garbage.end(), [](Buff* to_del) { delete to_del; } );
  garbage.clear();
}

void BuffManager::Load(Utils::Packet& packet, function<StatController* (const string&)> get_controller)
{
  unsigned short n_buffs;

  packet >> n_buffs;
  for (unsigned short i = 0 ; i < n_buffs ; ++i)
  {
    Buff* buff = new Buff(packet, tm, get_controller);

    buff->Over.Connect(*this, &BuffManager::Cleanup);
    buffs.push_back(buff);
  }
}

void BuffManager::Save(Utils::Packet& packet, function<bool (const string&)> callback)
{
  unsigned short n_buff = 0;
  auto           it     = buffs.begin();
  auto           end    = buffs.end();

  // How much buffs need to be saved ?
  while (it != end)
  {
    Buff* buff = *it;
    
    if (callback(buff->GetTargetName()))
      n_buff++;
  }
  packet << n_buff;

  // Now save them
  it  = buffs.begin();
  end = buffs.end();
  while (it != end)
  {
    Buff* buff = *it;

    if (callback(buff->GetTargetName()))
    {
      buff->Save(packet);
      it = buffs.erase(it);
    }
    else
      ++it;
  }
}

void GameTask::LoadLevelBuffs(Utils::Packet& packet)
{
  _buff_manager.Load(packet, [this](const string&name) -> StatController*
  {
    ObjectCharacter* character = _level->GetCharacter(name);

    return (character ? character->GetStatController() : 0);
  });
}

void GameTask::SaveLevelBuffs(Utils::Packet& packet)
{
  _buff_manager.Save(packet, _is_level_buff);
}

void GameTask::SavePartyBuffs(Utils::Packet&)
{
  auto it  = _buff_manager.buffs.begin();
  auto end = _buff_manager.buffs.end();

  while (it != end)
  {
    Buff* buff = *it;

    if (_is_level_buff(buff->GetTargetName()))
    {
      it = _buff_manager.buffs.erase(it);
      delete buff;
    }
    else
      ++it;
  }
}

void GameTask::PushBuff(ObjectCharacter* character, Data data)
{
  StatController* controller = character->GetStatController();
  Buff*           buff       = new Buff(character->GetName(), controller, data, _timeManager);

  _buff_manager.buffs.push_back(buff);
}

void GameTask::PushBuff(const std::string& name, Data data)
{
  // If Level is open, do it with level
  if (_level)
  {
    ObjectCharacter* character = _level->GetCharacter(name);

    if (character)
    {
      PushBuff(character, data);
      return ;
    }
  }

  // If target hasn't been found, or if level isn't open, search for target in PlayerParty
  auto it  = _playerParty->GetObjects().begin();
  auto end = _playerParty->GetObjects().end();
 
  for (; it != end ; ++it)
  {
    DynamicObject* object = *it;
    
    if (object->nodePath.get_name() == name)
    {
      StatController* controller = 0;
      Buff*           buff       = new Buff(name, controller, data, _timeManager);

      _buff_manager.buffs.push_back(buff);
      break ;
    }
  }
}

GameTask::GameTask(WindowFramework* window, GeneralUi& generalUi) : _gameUi(window, generalUi.GetRocketRegion()), _buff_manager(_timeManager)
{
  CurrentGameTask  = this;
  _continue        = true;
  _window          = window;
  _level           = 0;
  _savePath        = "saves";
  _worldMap        = 0;
  _charSheet       = 0;
  _playerParty     = 0;
  _playerStats     = 0;
  _playerInventory = 0;
  _uiSaveGame      = 0;
  _uiLoadGame      = 0;
  _gameUi.GetMenu().SaveClicked.Connect(*this, &GameTask::SaveClicked);
  _gameUi.GetMenu().LoadClicked.Connect(*this, &GameTask::LoadClicked);
  _gameUi.GetMenu().ExitClicked.Connect(*this, &GameTask::Exit);
  _gameUi.GetMenu().OptionsClicked.Connect(generalUi.GetOptions(), &UiBase::FireShow);
  
  _is_level_buff   = [this](const string& name) -> bool
  {
    ObjectCharacter* character   = _level->GetCharacter(name);
    auto             party_chars = _playerParty->ConstGetObjects();
    auto             it = party_chars.begin(), end = party_chars.end();
    
    for (; it != end ; ++it)
    {
      DynamicObject* party_character = *it;
      
      if (character->GetDynamicObject() == party_character)
	return (false);
    }
    return (character != 0);
  };
}

GameTask::~GameTask()
{
  if (_playerParty) { delete _playerParty; }
  if (_playerStats) { delete _playerStats; }
  if (_charSheet)   { delete _charSheet;   }
  if (_uiSaveGame)  { _uiSaveGame->Destroy(); delete _uiSaveGame; }
  if (_uiLoadGame)  { _uiLoadGame->Destroy(); delete _uiLoadGame; }
  if (_worldMap)    { _worldMap->Destroy();   delete _worldMap;   }
  if (_level)       { delete _level;       }
  CurrentGameTask = 0;
}

void                  GameTask::SaveClicked(Rocket::Core::Event&)
{
  if (_uiSaveGame)
    delete _uiSaveGame;
  _uiSaveGame = new UiSave(_window, _gameUi.GetContext(), _savePath);
  _uiSaveGame->SaveToSlot.Connect(*this, &GameTask::SaveToSlot);
  _uiSaveGame->Show();
}

void                  GameTask::LoadClicked(Rocket::Core::Event&)
{
  if (_uiLoadGame)
    delete _uiLoadGame;
  _uiLoadGame = new UiLoad(_window, _gameUi.GetContext(), _savePath);
  _uiLoadGame->LoadSlot.Connect(*this, &GameTask::LoadSlot);
  _uiLoadGame->Show();
}

void                  GameTask::MapOpenLevel(std::string name)
{
  OpenLevel(_savePath, name);
  _loadLevelParams.entry_zone = "worldmap";
}

void                  GameTask::SetLevel(Level* level)
{
  _level = level;
}

void                  GameTask::GameOver(void)
{
  _continue = false;
}

AsyncTask::DoneStatus GameTask::do_task()
{
  if (!_continue)
    return (AsyncTask::DS_done);
  if (_loadLevelParams.doLoad)
    DoLoadLevel();
  {
    Data charsheet(_charSheet);
    if ((int)charsheet["Variables"]["Hit Points"] <= 0)
      GameOver();
  }
  if (_level)
  {
    if (_level->do_task() == AsyncTask::DoneStatus::DS_done)
    {
      const string nextZone  = _level->GetNextZone();
      const string exitPoint = _level->GetExitZone();

      ExitLevel(_savePath);
      if (nextZone != "")
      {
	OpenLevel(_savePath, nextZone);
	_loadLevelParams.entry_zone = exitPoint;
      }
    }
    if (!_level && _worldMap)
      _worldMap->Show();
  }
  else if (_worldMap)
    _worldMap->Run();
  _buff_manager.CollectGarbage();
  return (AsyncTask::DoneStatus::DS_cont);
}

bool GameTask::SaveGame(const std::string& savepath)
{
  bool success = true;

  _worldMap->Save(savepath);
  if (_level)
  {
    _dataEngine["system"]["current-level"] = _levelName;
    success = success && SaveLevel(_level, savepath + "/" + _levelName + ".blob");
  }
  else
  {
    _dataEngine["system"]["current-level"] = 0;
    _playerParty->Save(savepath);
  }

  {
    Data player_inventory = _dataEngine["player"]["inventory"];
    
    if (player_inventory.Count() != 0)
    {
      player_inventory.CutBranch();
      player_inventory = _dataEngine["player"]["inventory"];
      player_inventory.Output();
    }
    _playerInventory->SaveInventory(player_inventory);
  }
  _dataEngine["time"]["seconds"] = _timeManager.GetSecond();
  _dataEngine["time"]["minutes"] = _timeManager.GetMinute();
  _dataEngine["time"]["hours"]   = _timeManager.GetHour();
  _dataEngine["time"]["days"]    = _timeManager.GetDay();
  _dataEngine["time"]["month"]   = _timeManager.GetMonth();
  _dataEngine["time"]["year"]    = _timeManager.GetYear();
  _dataEngine.Save(savepath + "/dataengine.json");

  DataTree::Writers::JSON(_charSheet, savepath + "/stats-self.json");

  return (success);
}

bool GameTask::LoadGame(const std::string& savepath)
{
  Data currentLevel, time;

  if (_worldMap)    delete _worldMap;
  if (_playerParty) delete _playerParty;
  if (_playerStats) delete _playerStats;
  if (_charSheet)   delete _charSheet;
  if (_playerInventory)
  {
    delete _playerInventory;
    _playerInventory = 0;
  }

  _dataEngine.Load(savepath + "/dataengine.json");
  currentLevel     = _dataEngine["system"]["current-level"];
  time             = _dataEngine["time"];
  _timeManager.ClearTasks(0);
  _timeManager.SetTime(time["seconds"], time["minutes"], time["hours"], time["days"], time["month"], time["year"]);
  _charSheet       = DataTree::Factory::JSON(savepath + "/stats-self.json");
  if (!_charSheet)  return (false);
  _playerParty     = new PlayerParty(savepath);
  _playerStats     = new StatController(_charSheet);
  _playerStats->SetView(&(_gameUi.GetPers()));
  if (_dataEngine["player"]["inventory"].NotNil())
  {
    _playerInventory = new Inventory;
    _playerInventory->LoadInventory(_dataEngine["player"]["inventory"]);
    _gameUi.GetInventory().SetInventory(*_playerInventory);
  }

  _worldMap    = new WorldMap(_window, &_gameUi, _dataEngine, _timeManager);
  _worldMap->GoToPlace.Connect(*this, &GameTask::MapOpenLevel);

  if (!(currentLevel.Nil()) && currentLevel.Value() != "0")
  {
    _worldMap->Hide();
    LoadLevel(_window, _gameUi, savepath + "/" + currentLevel.Value() + ".blob", currentLevel.Value(), true);
  }
  else
    _worldMap->Show();
  return (true);
}

void GameTask::OpenLevel(const std::string& savepath, const std::string& level)
{
  std::ifstream fileTest;

  _worldMap->Hide();
  fileTest.open((savepath + "/" + level + ".blob").c_str());
  if (fileTest.is_open())
  {
    fileTest.close();
    _level = LoadLevel(_window, _gameUi, savepath + "/" + level + ".blob", level, true);
  }
  else
    _level = LoadLevel(_window, _gameUi, "maps/" + level + ".blob", level, false);
}

void GameTask::ExitLevel(const std::string& savepath)
{
  _level->StripParty(*_playerParty);
  if (!(SaveGame(savepath)))
  {
    cerr << "!! Couldn't save level state on ExitLevel !!" << endl;
  }
  delete _level;
  _level = 0;
  cout << "Exited Level" << endl;
}

bool GameTask::CopySave(const std::string& savepath, const std::string& slotPath)
{
  // Copy the savepath directory to the slotpath directory
  Directory                          dir;
  Directory::Entries::const_iterator it, end;

  Directory::MakeDir(slotPath);
  dir.OpenDir(savepath);
  it  = dir.GetEntries().begin();
  end = dir.GetEntries().end();
  
  for (; it != end ; ++it)
  {
    const Dirent& entry = *it;

    if (entry.d_type == DT_REG)
    {
      // Copy file to the slot
      std::string   ifilePath, ofilePath;
      std::ifstream ifile;
      std::ofstream ofile;

      ifilePath = savepath + "/" + entry.d_name;
      ofilePath = slotPath + "/" + entry.d_name;
      ifile.open(ifilePath.c_str());
      ofile.open(ofilePath.c_str());
      if (ifile.is_open() && ofile.is_open())
      {
	long  begin, end;
	long  size;
	char* raw;

	begin     = ifile.tellg();
	ifile.seekg (0, std::ios::end);
	end       = ifile.tellg();
	ifile.seekg(0, std::ios::beg);
	size      = end - begin;
	raw       = new char[size + 1];
	raw[size] = 0;
	ifile.read(raw, size);
	ifile.close();

	ofile.write(raw, size);
	ofile.close();

	delete[] raw;
      }
      else
      {
	// Can't copy file...
	return (false);
      }
    }
  }
  return (true);
}

void GameTask::EraseSlot(unsigned char slot)
{
  std::stringstream stream;
  Directory         dir;
  
  stream << _savePath << "/slot-" << (int)slot;
  dir.OpenDir(stream.str());
  
  std::for_each(dir.GetEntries().begin(), dir.GetEntries().end(), [](const Dirent& entry)
  {
	std::string to_remove = entry.d_name;
    remove(to_remove.c_str());
  });
}

void GameTask::SaveToSlot(unsigned char slot)
{
  if (SaveGame(_savePath))
  {
    std::stringstream stream;

    stream << _savePath << "/slot-" << (int)slot;
    EraseSlot(slot);
    CopySave(_savePath, stream.str());
  }
  if (_uiSaveGame) _uiSaveGame->Hide();
}

void GameTask::LoadLastState(void)
{
  if (_level)    delete _level;
  if (_worldMap) { _worldMap->Hide(); delete _worldMap; }
  FinishLoad();
}

void GameTask::LoadSlot(unsigned char slot)
{
  if (_level)    { delete _level; _level = 0; }
  if (_worldMap) { _worldMap->Hide(); delete _worldMap; _worldMap = 0; }
  
  // Clear original directory
  {
    Directory         dir;

    dir.OpenDir(_savePath);

    std::for_each(dir.GetEntries().begin(), dir.GetEntries().end(), [](const Dirent& entry)
    {
      if (entry.d_type == DT_REG)
	  {
		std::string dname = entry.d_name;
        remove(dname.c_str());
	  }
    });
  }
  
  // Copy the saved files in the original directory
  {
    std::stringstream stream;

    stream << _savePath << "/slot-" << (unsigned int)slot;
    CopySave(stream.str(), _savePath);
  }
  
  if (_uiLoadGame) _uiLoadGame->Hide();
  FinishLoad();
}

void GameTask::FinishLoad(void)
{
  if (!(LoadGame(_savePath)))
  {
    // TODO Handle error while loading the game
    cout << "[NOT IMPLEMENTED] GameTask::FinishLoad -> LoadGame Failure" << endl;
  }
}

// LEVEL EVENTS
void GameTask::UiSaveGame(const std::string& slotPath)
{
  SaveGame(_savePath);
  CopySave(_savePath, slotPath);
}

void GameTask::UiLoadGame(const std::string& slotPath)
{
  CopySave(_savePath, slotPath);
  LoadGame(_savePath);
}

bool GameTask::SaveLevel(Level* level, const std::string& name)
{
  Utils::Packet packet;
  std::ofstream file;

  level->SaveUpdateWorld();
  level->UnprocessAllCollisions();
  level->GetWorld()->Serialize(packet);
  level->Save(packet);
  level->ProcessAllCollisions();
  file.open(name.c_str(), std::ios::binary);
  if (file.is_open())
  {
    file.write(packet.raw(), packet.size());
    file.close();
  }
  else
  {
    std::cerr << "�� Failed to open file '" << name << "', save failed !!" << std::endl;
    return (false);
  }
  return (true);
}

void   GameTask::SetPlayerInventory(void)
{
  if (!_playerInventory)
  {
    Inventory& inventory = _level->GetPlayer()->GetInventory();
    Data       save      = _dataEngine["player"]["inventory"];

    inventory.SaveInventory(save);
    save.Output();
    _playerInventory = new Inventory;
    _playerInventory->LoadInventory(save);
    _gameUi.GetInventory().SetInventory(*_playerInventory);
  }
  _level->SetPlayerInventory(_playerInventory);
}

Level* GameTask::DoLoadLevel(void)
{
  std::cout << "DoLoadLevel" << std::endl;
  Level*        level = 0;
  std::ifstream file;
  std::string   name  = _loadLevelParams.path;
  
  file.open(name.c_str(), std::ios::binary);
  if (file.is_open())
  {
    Utils::Packet packet(file);

    try
    {
      level = new Level(_window, _gameUi, packet, _timeManager);
      if (_loadLevelParams.isSaveFile)
	level->Load(packet);
      SetLevel(level);
      file.close();
    }
    catch (const char* error)
    {
      std::cerr << "�� Failed to load file !! (" << error << ")" << std::endl;
      level = 0;
    }
  }
  else
    std::cerr << "�� File not found !!" << std::endl;
  if (_level)
  {
    _levelName = _loadLevelParams.name;
    _level->SetDataEngine(&_dataEngine);
    if (_loadLevelParams.entry_zone == "worldmap")
      _level->InsertParty(*_playerParty);
    _level->InitPlayer();
    _level->GetPlayer()->SetStatistics(_charSheet, _playerStats);
    SetPlayerInventory();
    _level->SetEntryZone(*_playerParty, _loadLevelParams.entry_zone);
    SetLevel(_level);
  }
  else
  {
    cerr << "�� Can't open level !!" << endl;
    _worldMap->Show();
  }
  _loadLevelParams.entry_zone = "";
  _loadLevelParams.doLoad     = false;
  return (level);  
}

Level* GameTask::LoadLevel(WindowFramework* window, GameUi& gameUi, const std::string& path, const std::string& name, bool isSaveFile)
{
  _loadLevelParams.name       = name;
  _loadLevelParams.path       = path;
  _loadLevelParams.isSaveFile = isSaveFile;
  _loadLevelParams.doLoad     = true;
  return (0);
}