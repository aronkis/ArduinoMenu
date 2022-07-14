#include "Menu.h" 

// define text to display
const char DateTime[] = {"1. Time & Date"};
const char Time[] = {"15:00"};
const char Date[] = {"07/14/2022"};
const char History[] = {"2. History"};
const char History1[] = {"Pet feed at: @Time"};
const char Settings[] = {"3. Settings"};
const char Settings1[] = {"Time"};
const char Settings2[] = {"Date"};
const char Settings3[] = {"Treat Amount"};
const char Settings4[] = {"Food Amount"};
const char Settings5[] = {"Feed Time"};

// define function IDs
enum MenuFID {
  MenuDummy,
  DateTimeID,
  TimeID,
  DateID,
  HistoryID,
  History1ID,
  SettingsID,
  Settings1ID,
  Settings2ID,
  Settings3ID,
  Settings4ID,
  Settings5ID
};

// define key types
enum KeyType {
  KeyNone, // no key is pressed
  KeyLeft,
  KeyRight,
  KeyEnter,
  KeyExit
};

Menu<11> ArdMenu;
int main()
{
  // ** menu **
  // add nodes to menu (layer, string, function ID)
  ArdMenu.addNode(0, DateTime , DateTimeID);
  ArdMenu.addNode(1, Time, TimeID);
  ArdMenu.addNode(1, Date, DateID);

  ArdMenu.addNode(0, History, HistoryID);
  ArdMenu.addNode(1, History1, History1ID);
  
  ArdMenu.addNode(0, Settings, SettingsID);
  ArdMenu.addNode(1, Settings1, Settings1ID);
  ArdMenu.addNode(1, Settings2, Settings2ID);
  ArdMenu.addNode(1, Settings3, Settings3ID);
  ArdMenu.addNode(1, Settings4, Settings4ID);
  ArdMenu.addNode(1, Settings5, Settings5ID);
  


  // ** menu **
  // build menu and print menu
  // (see terminal for output)
  const char* info;
  ArdMenu.buildMenu(info);
  ArdMenu.printMenu();
}
