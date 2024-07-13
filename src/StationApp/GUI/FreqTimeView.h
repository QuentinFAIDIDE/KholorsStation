#pragma once

#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/FrequencyScale.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "StationApp/GUI/TimeScale.h"
#include "StationApp/GUI/TrackList.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define MAX_SCALE_SAMPLE_PER_PIXEL 600
#define MIN_SCALE_SAMPLE_PER_PIXEL 80
#define PIXEL_SCALE_SPEED 0.01f
#define MAX_IDLE_MS_TIME_BEFORE_CLEAR 500
#define MAX_TIME_SINCE_FFT_UPDATE_TO_CENTER_VIEW_MS 250
#define MAX_TIME_WITHOUT_TRACK_LIST_PAINT_MS 150
#define VIEW_MOVE_TIME_INTERVAL_MS 15
#define FREQUENCY_GRID_WIDTH 110
#define TIME_GRID_HEIGHT 65
#define TRACK_LIST_WIDTH 220

/**
 * @brief Describe a class which displays a timeline, and
 * which own a drawing backend that will draw FFT of signal
 * received. It will draw labels and eventually more info over drawing widgets.
 */
class FreqTimeView : public juce::Component, public TaskListener, public juce::Timer
{
  public:
    FreqTimeView(TrackInfoStore &);
    ~FreqTimeView();

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseMove(const juce::MouseEvent &me) override;
    void mouseExit(const juce::MouseEvent &me) override;

    void timerCallback() override;

    /**
     * @brief Receives tasks from tasking manager and is responsible for
     * handling the one emmited by AudioDataWorker, in particular the ones
     * with Short Time Fast Fourier Transforms or track name/colors updates.
     *
     * @param task the task to be casted, we may or may not be interested by it
     * @return true if we want to stop the dtask from broacasting to further listeners
     * @return false if we don't care if the tasks keep on going to other Listeners.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    NormalizedUnitTransformer frequencyTransformer;    /**< Transformer for the frequency displayed */
    NormalizedUnitTransformer intensityTransformer;    /**< Transformer for the intensity displayed */
    std::shared_ptr<FftDrawingBackend> fftDrawBackend; /**< Juce component that draws FFTs on screen */
    TrackInfoStore &trackInfoStore;         /**< Store track names and color for FftDrawingBackend to access */
    int64_t lastMouseDragX, lastMouseDragY; /**< Last position of the mouse cursor at last drag iteration */
    int64_t lastFftDrawTimeMs;              /**< Last millisecond timestamp at when something was drawn */
    std::mutex lastFftDrawTimeMutex;

    int64_t lastTimerCallMs; /**< time since last timer call */

    bool isViewMoving; /**< tells if user is dragging the view around. Beware not to access outside of juce message
                          thread */

    std::mutex viewMutex; /**< Mutex for view position and scale */
    int64_t viewPosition; /**< View position in samples */
    int64_t viewScale;    /**< View scale in samples per pixels */

    FrequencyScale frequencyScale;
    TimeScale timeScale;

    TrackList trackList;

    juce::Rectangle<int> unpaintedArea1, unpaintedArea2; /**< Area left unpainted that FreqView needs to paint */

    float lastReceivedBpm;
};