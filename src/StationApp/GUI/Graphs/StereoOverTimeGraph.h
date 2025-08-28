#pragma once

#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/ClearTrackInfoRange.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_opengl/juce_opengl.h"

class StereoOverTimeGraph : public juce::Component, juce::OpenGLRenderer
{
  public:
    StereoOverTimeGraph(TrackInfoStore &tis);
    ~StereoOverTimeGraph();

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

    /**
     * @brief Move the view so that the position at the component left
     * matches the samplePosition (audio sample offset of the song).
     *
     * @param samplePosition audio sample position (audio sample offset of the song) to match
     */
    void updateViewPosition(uint32_t samplePosition);

    /**
     * @brief Scale the view
     * @param samplesPerPixel number of audio samples per pixel to display in the viewer
     */
    void updateViewScale(uint32_t samplesPerPixel);

    /**
     * @brief Update the bpm.
     *
     * @param newBpm new bpm value to use to draw the grid
     */
    void updateBpm(float newBpm, TaskingManager *tm);

    /**
     * @brief Update the time signature of the beat grid.
     *
     * @param timeSignatureNumerator number at numerator of the time signature fraction.
     */
    void timeSignatureNumeratorUpdate(int timeSignatureNumerator);

    /**
     * @brief clears on screen data.
     * In this openGL version, queue clearing to be done by openGL Thread.
     */
    void clearDisplayedSections();

    /**
     * @brief      Called when opengl context is created.
     */
    void newOpenGLContextCreated() override;

    /**
     * @brief      Renders openGL context
     */
    void renderOpenGL() override;

    /**
     * @brief      Called when opengl context is closed.
     */
    void openGLContextClosing() override;

    /**
     * @brief Return a list of ranges where specific tracks have been
     * cleared from.
     * @return std::vector<ClearTrackInfoRange> vector of ranges to clear with trackIdentifiers.
     */
    std::vector<ClearTrackInfoRange> getClearedTrackRanges();

    /**
     * @brief Set the mouse cursor position on component, as it is intercepted
     * by parent and is not received. We could let the mouse events through
     * but we're with this patterN;
     *
     * @param onComponent is the mouse over this compoennt ?
     * @param x mouse x
     * @param y mouse y
     */
    void setMouseCursor(bool onComponent, int x, int y);

    /**
     * @brief Setting the currently selected track highlighted on screen.
     *
     * @param selectedTrack Optional, being if something is selected the identifier of the track.
     */
    void setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm);

  private:
    TrackInfoStore &trackInfoStore;
};