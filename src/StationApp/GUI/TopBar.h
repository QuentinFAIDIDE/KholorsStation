#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/IconsLoader.h"
#include "StationApp/GUI/Widgets/ClearButton.h"
#include "StationApp/GUI/Widgets/HelpButton.h"
#include "StationApp/GUI/Widgets/SensitivitySlider.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

inline constexpr int TOP_BAR_HEIGHT = 64;

/**
 * @brief A component that is meant to be at the top of the
 * station window, showing the app title and version, the
 * sensitivity slider and the clear and help buttons.
 */
class TopBar final : public juce::Component
{
  public:
    explicit TopBar(TaskingManager &tm);

    /**
     * @brief Show or hide the notice telling a newer version is available.
     */
    void setUpdateAvailable(bool isUpdateAvailable);

    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    juce::SharedResourcePointer<IconsLoader> sharedSvgs;  /**< Singleton that loads svg files */
    HelpButton helpButton;
    ClearButton clearButton;
    SensitivitySlider volumeSensitivitySlider;
    bool updateAvailable{false};

    // areas computed in resized() and drawn in paint()
    juce::Rectangle<int> mainTitleArea, subTitleArea, versionArea;
    juce::Rectangle<int> lowWavePictogramArea, highWavePictogramArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};
