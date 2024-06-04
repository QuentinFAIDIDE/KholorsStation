#include "ServerTab.h"
#include "GUIToolkit/Widgets/LabeledLineContainer.h"
#include "StationApp/GUI/EmptyTab.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>

ServerTab::ServerTab(TaskingManager *) : serverStatusStack(TRANS("DATA SERVER STATUS").toStdString())
{
    auto tab = std::make_shared<EmptyTab>();
    auto tab2 = std::make_shared<EmptyTab>();
    auto tab3 = std::make_shared<EmptyTab>();
    auto line1 = std::make_shared<LabeledLineContainer>(TRANS("Server Status").toStdString(), tab, 160, 80);
    auto line2 = std::make_shared<LabeledLineContainer>(TRANS("Server Port").toStdString(), tab, 160, 80);
    auto line3 = std::make_shared<LabeledLineContainer>(TRANS("Listening Address").toStdString(), tab, 160, 80);
    serverStatusStack.addComponent(line1);
    serverStatusStack.addComponent(line2);
    serverStatusStack.addComponent(line3);
    addAndMakeVisible(serverStatusStack);
}

void ServerTab::resized()
{
    serverStatusStack.setBounds(getLocalBounds().removeFromLeft(DEFAULT_LABELED_LINE_WIDTH));
}