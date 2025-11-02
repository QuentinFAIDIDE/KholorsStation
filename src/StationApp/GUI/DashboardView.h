#pragma once

#include "StationApp/Audio/BpmUpdateTask.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include "StationApp/Audio/TimeSignatureUpdateTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/Audio/VolumeSensitivityTask.h"
#include "StationApp/GUI/ClearTask.h"
#include "StationApp/GUI/Graphs/FftOverTimeGraph.h"
#include "StationApp/GUI/Graphs/FrequencyScale.h"
#include "StationApp/GUI/Graphs/TimeScale.h"
#include "StationApp/GUI/Graphs/VolumeOverTimeGraph.h"
#include "StationApp/GUI/Graphs/VolumeScale.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "StationApp/GUI/TrackList.h"
#include "StationApp/GUI/TrackSelectionTask.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define MAX_SCALE_SAMPLE_PER_PIXEL 400
#define MIN_SCALE_SAMPLE_PER_PIXEL 80
#define PIXEL_SCALE_SPEED 0.01f
#define MAX_IDLE_MS_TIME_BEFORE_CLEAR 2000
#define MAX_TIME_SINCE_FFT_UPDATE_TO_CENTER_VIEW_MS 250
#define MAX_TIME_WITHOUT_TRACK_LIST_PAINT_MS 150
#define VIEW_MOVE_TIME_INTERVAL_MS 15
#define FREQUENCY_GRID_WIDTH 90
#define TIME_GRID_HEIGHT 55
#define VOLUME_GRAPH_HEIGHT 220
#define TRACK_LIST_WIDTH 220
#define TIME_GRAPHS_PADDING 20

/**
 * @brief Describe a class which displays a timeline, and
 * which own a drawing backend that will draw FFT of signal
 * received. It will draw labels and eventually more info over drawing widgets.
 */
class DashboardView : public juce::Component, public TaskListener, public juce::Timer
{
  public:
    DashboardView(TrackInfoStore &, TaskingManager &);
    ~DashboardView();

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
    /**
     * @brief Compute frequency and time under cursor and pass data to freqtime view and tip bar.
     *
     * @param me mouse event on drag or move.
     */
    void broadcastMouseEventInfo(const juce::MouseEvent &me);

    /**
     * @brief called to broadcast the mouse position relative to the fft view to the tip bar
     * through a position info task.
     *
     * @param mouseOverFreqTimeGraph if the mouse cross is inside the FFT and the position tip should be shown.
     * @param x the x position relative to the dashbaord view
     * @param y the y position relative to the dashbaord view
     */
    void emitMousePositionInfoTask(bool mouseOverFreqTimeGraph, int x, int y);

    void updateGraphsViewPositions(int64_t newPosition);

    void updateGraphsViewScale(int64_t newScale);

    bool handleZoom(int dragY, int mouseX);
    bool handlePan(int dragX);

    void updateViewMouseDrag(const juce::MouseEvent &e);

    void updateAutoscroll(int64_t currentTime, int64_t elapsedSinceLastCallMs, int64_t lastFftDrawTimeMsCopy);

    void propagateClearedFft();

    bool handleNewFftDataTask(std::shared_ptr<NewFftDataTask> task);
    bool handleNewVolumeDataTask(std::shared_ptr<NewTrackVolumeDataTask> task);
    bool handleTrackColorUpdateTask(std::shared_ptr<TrackColorUpdateTask> task);
    bool handleBpmUpdateTask(std::shared_ptr<BpmUpdateTask> task);
    bool handleTimeSignatureUpdateTask(std::shared_ptr<TimeSignatureUpdateTask> task);
    bool handleTrackSelectionTask(std::shared_ptr<TrackSelectionTask> task);
    bool handleClearTask(std::shared_ptr<ClearTask> task);
    bool handleVolumeSensitivityTask(std::shared_ptr<VolumeSensitivityTask> task);

    TaskingManager &taskingManager;
    ProcessingTimer processingTimer;

    NormalizedUnitTransformer frequencyTransformer;           /**< Transformer for the frequency displayed */
    NormalizedUnitTransformer intensityTransformer;           /**< Transformer for the intensity displayed */
    std::shared_ptr<FftOverTimeGraph> freqOverTimeGraph;      /**< Juce component that draws FFTs on screen */
    std::shared_ptr<VolumeOverTimeGraph> volumeOverTimeGraph; /**< Juce component that draws volumes on screen */
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
    VolumeScale volumeScale;
    TimeScale timeScale;

    TrackList trackList;

    juce::Rectangle<int> unpaintedArea1, unpaintedArea2,
        unpaintedArea3; /**< Area left unpainted that FreqView needs to paint */

    juce::Rectangle<int> timeTicksAxisNameArea;

    int lastFftMousePosX;
    int lastFftMousePosY;
    bool lastCursorShowStatus;

    float lastReceivedBpm;
};