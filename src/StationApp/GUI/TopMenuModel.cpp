#include "TopMenuModel.h"
#include "TaskManagement/TaskingManager.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

#define RESULT_ID_EXIT 1
#define RESULT_ID_ABOUT 2

TopMenuModel::TopMenuModel(TaskingManager &tm) : taskManager(tm)
{
    topLevelMenus = {TRANS("File"), TRANS("Help")};
    menuItems.resize(2);
    menuItems[0].addItem(RESULT_ID_EXIT, TRANS("Exit"), true, false);
    menuItems[1].addItem(RESULT_ID_ABOUT, TRANS("About"), true, false);
}

juce::StringArray TopMenuModel::getMenuBarNames()
{
    return topLevelMenus;
}

juce::PopupMenu TopMenuModel::getMenuForIndex(int topLevelMenuIndex, const juce::String &)
{
    if (topLevelMenuIndex < 0 || (size_t)topLevelMenuIndex >= menuItems.size())
    {
        throw std::invalid_argument("getMenuForIndex topLevelMenuIndex out of range");
    }
    return menuItems[(size_t)topLevelMenuIndex];
}

void TopMenuModel::menuBarActivated(bool)
{
}

void TopMenuModel::menuItemSelected(int menuItemId, int)
{
    switch (menuItemId)
    {
    case RESULT_ID_ABOUT:
        spdlog::info("about clicked");
        break;

    case RESULT_ID_EXIT:
        spdlog::info("exit clicked");
        break;

    default:
        spdlog::warn("Received menu click with unknown id: " + std::to_string(menuItemId));
        break;
    }
}
