#pragma once

#include "FftDrawingBackend.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

/**
 * @brief Describe a class which displays a timeline, and
 * which own a drawing backend that will draw FFT of signal
 * received. It will draw labels and eventually more info over drawing widgets.
 */
class FreqTimeView : public juce::Component, public TaskListener
{
  public:
    FreqTimeView(TrackInfoStore &);
    ~FreqTimeView();

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;

    /**
     * @brief Receives tasks from tasking manager and is responsible for
     * handling the one emmited by AudioDataWorker, in particular the ones
     * with Short Time Fast Fourier Transforms or track name/colors updates.
     *
     * @param task the task to be casted, we may or may not be interested by it
     * @return true if we want to stop the task from broacasting to further listeners
     * @return false if we don't care if the tasks keep on going to other Listeners.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    std::shared_ptr<FftDrawingBackend> fftDrawBackend; /**< Juce component that draws FFTs on screen */
    TrackInfoStore &trackInfoStore;         /**< Store track names and color for FftDrawingBackend to access */
    int64_t viewPosition;                   /**< View position in samples */
    int64_t viewScale;                      /**< View scale in samples per pixels */
    int64_t lastMouseDragX, lastMouseDragY; /**< Last position of the mouse cursor at last drag iteration */
};