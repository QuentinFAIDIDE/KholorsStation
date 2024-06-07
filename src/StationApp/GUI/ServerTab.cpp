#include "ServerTab.h"
#include "GUIToolkit/Widgets/LabeledLineContainer.h"
#include "GUIToolkit/Widgets/LabeledLineText.h"
#include "GUIToolkit/Widgets/ServerPortText.h"
#include "GUIToolkit/Widgets/ServerStatusText.h"
#include "StationApp/GUI/EmptyTab.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>

ServerTab::ServerTab(TaskingManager &tm) : serverStatusStack(TRANS("DATA SERVER STATUS").toStdString())
{
    auto serverStatusText = std::make_shared<ServerStatusText>(tm);
    auto serverStatusPort = std::make_shared<ServerPortText>(tm);
    auto serverAddr = std::make_shared<LabeledLineText>("127.0.0.1");
    auto line1 =
        std::make_shared<LabeledLineContainer>(TRANS("Server Status").toStdString(), serverStatusText, 160, 80);
    auto line2 = std::make_shared<LabeledLineContainer>(TRANS("Server Port").toStdString(), serverStatusPort, 160, 80);
    auto line3 = std::make_shared<LabeledLineContainer>(TRANS("Listening Address").toStdString(), serverAddr, 160, 80);
    serverStatusStack.addComponent(line1);
    serverStatusStack.addComponent(line2);
    serverStatusStack.addComponent(line3);
    addAndMakeVisible(serverStatusStack);
}

void ServerTab::resized()
{
    serverStatusStack.setBounds(getLocalBounds().removeFromLeft(DEFAULT_LABELED_LINE_WIDTH));
}